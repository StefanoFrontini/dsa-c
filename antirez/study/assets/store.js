// Progressi dell'utente in un'unica chiave versionata di localStorage.
// Ogni accesso è in try/catch: se localStorage non c'è (navigazione privata,
// cookie bloccati, file://) il sito funziona lo stesso con lo stato in memoria.

const KEY = 'study.v1';
const VERSION = 1;

const empty = () => ({ v: VERSION, theme: null, reviewed: {}, quiz: {} });

let state = empty();
let persistent = false;

function sanitize(raw) {
  const s = empty();
  if (!raw || typeof raw !== 'object') return s;
  if (raw.theme === 'light' || raw.theme === 'dark') s.theme = raw.theme;
  if (raw.reviewed && typeof raw.reviewed === 'object') {
    for (const [k, v] of Object.entries(raw.reviewed)) {
      if (typeof v === 'string') s.reviewed[k] = v;
    }
  }
  if (raw.quiz && typeof raw.quiz === 'object') {
    for (const [k, r] of Object.entries(raw.quiz)) {
      if (r && ['so', 'incerto', 'non'].includes(r.s) && typeof r.d === 'string') {
        s.quiz[k] = { s: r.s, d: r.d, q: typeof r.q === 'string' ? r.q : '', n: Number(r.n) || 1 };
      }
    }
  }
  return s;
}

function load() {
  try {
    const txt = window.localStorage.getItem(KEY);
    state = txt ? sanitize(JSON.parse(txt)) : empty();
    // Verifica che si possa anche scrivere.
    window.localStorage.setItem(KEY, JSON.stringify(state));
    persistent = true;
  } catch {
    persistent = false;
  }
}

function save() {
  if (!persistent) return;
  try {
    window.localStorage.setItem(KEY, JSON.stringify(state));
  } catch {
    persistent = false;
  }
}

load();

export const isPersistent = () => persistent;

export function today() {
  const d = new Date();
  const p = (n) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${p(d.getMonth() + 1)}-${p(d.getDate())}`;
}

export function daysSince(isoDate) {
  const a = new Date(isoDate + 'T00:00:00');
  const b = new Date(today() + 'T00:00:00');
  return Math.round((b - a) / 86400000);
}

// --- tema ---
export const getTheme = () => state.theme;
export function setTheme(t) {
  state.theme = t === 'light' || t === 'dark' ? t : null;
  save();
}

// --- concetti ripassati ---
export const isReviewed = (key) => Boolean(state.reviewed[key]);
export const reviewedDate = (key) => state.reviewed[key] || null;
export function setReviewed(key, on) {
  if (on) state.reviewed[key] = today();
  else delete state.reviewed[key];
  save();
}

// --- quiz di autoverifica ---
// Chiave = `<area>:<indice>`. Salviamo anche il testo della domanda: se le domande
// vengono riordinate, il record si ritrova per testo; se il testo viene ritoccato,
// vale l'indice.
export const quizKey = (area, i) => `${area}:${i}`;

export function getQuiz(area, i, q) {
  const direct = state.quiz[quizKey(area, i)];
  if (direct && direct.q === q) return direct;
  for (const [k, r] of Object.entries(state.quiz)) {
    if (k.startsWith(area + ':') && r.q === q) return r;
  }
  return direct || null;
}

export function setQuiz(area, i, q, s) {
  const key = quizKey(area, i);
  // Se il record era sotto un'altra chiave (domande riordinate) lo spostiamo.
  for (const [k, r] of Object.entries(state.quiz)) {
    if (k !== key && k.startsWith(area + ':') && r.q === q) delete state.quiz[k];
  }
  const prev = state.quiz[key];
  state.quiz[key] = { s, d: today(), q, n: (prev && prev.q === q ? prev.n : 0) + 1 };
  save();
}

// --- export / import / reset ---
export function exportJSON() {
  return JSON.stringify({ ...state, exported: new Date().toISOString() }, null, 2);
}

export function importJSON(text) {
  const raw = JSON.parse(text); // lancia se il file non è JSON
  if (!raw || typeof raw !== 'object' || (raw.v !== undefined && raw.v !== VERSION)) {
    throw new Error('formato non riconosciuto (attesa versione ' + VERSION + ')');
  }
  const next = sanitize(raw);
  next.theme = state.theme; // il tema resta quello del dispositivo
  state = next;
  save();
  return {
    reviewed: Object.keys(state.reviewed).length,
    quiz: Object.keys(state.quiz).length,
  };
}

export function resetProgress() {
  const theme = state.theme;
  state = empty();
  state.theme = theme;
  save();
}

export const stats = () => ({
  reviewed: Object.keys(state.reviewed).length,
  quiz: Object.keys(state.quiz).length,
});
