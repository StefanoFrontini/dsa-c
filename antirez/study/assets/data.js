// Caricamento dei file di contenuto (markdown e JSON) con cache in memoria.
// I file vengono sempre richiesti con `cache: 'no-cache'`: il browser rivalida con
// l'ETag, così le correzioni alle note si vedono al primo ricaricamento.

import { AREAS } from './areas.js';

export class LoadError extends Error {
  constructor(path, detail) {
    super(`Impossibile caricare ${path}: ${detail}`);
    this.path = path;
    this.detail = detail;
  }
}

const cache = new Map();

async function get(path, kind) {
  if (location.protocol === 'file:') {
    throw new LoadError(path, 'la pagina è aperta da file://');
  }
  let res;
  try {
    res = await fetch(path, { cache: 'no-cache' });
  } catch (e) {
    throw new LoadError(path, 'errore di rete (' + e.message + ')');
  }
  if (!res.ok) throw new LoadError(path, `HTTP ${res.status}`);
  if (kind === 'json') {
    const text = await res.text();
    try {
      return JSON.parse(text);
    } catch (e) {
      throw new LoadError(path, 'JSON non valido (' + e.message + ')');
    }
  }
  return res.text();
}

function cached(path, kind) {
  if (!cache.has(path)) {
    const p = get(path, kind);
    cache.set(path, p);
    p.catch(() => cache.delete(path)); // un errore non resta in cache
  }
  return cache.get(path);
}

export const loadAreaData = (id) => cached(`data/${id}.json`, 'json');
export const loadNote = (id) => cached(`notes/${id}.md`, 'text');
export const loadProjectNote = (id) => cached(`projects/${id}.md`, 'text');
// Turni della conversazione con Gemini (generati da tools/split_turns.py).
export const loadTurnIndex = () => cached('turns/index.json', 'json');
// Correzioni puntuali ai turni (facoltativo): un 404 vale "nessun turno rivisto".
export const loadCorrections = () =>
  get('turns/corrections.json', 'json').catch((e) => {
    if (!(e instanceof LoadError && /404/.test(e.detail))) console.warn(e.message);
    return null;
  });
export const loadTurn = (n) => cached(`turns/${String(n).padStart(3, '0')}.md`, 'text');

// Carica tutti i JSON delle aree. Le aree che non si caricano compaiono in `errors`
// invece di bloccare la pagina.
export async function loadAllAreas() {
  const results = await Promise.allSettled(AREAS.map(loadAreaData));
  const areas = [];
  const errors = [];
  results.forEach((r, i) => {
    if (r.status === 'fulfilled') areas.push(normalizeArea(AREAS[i], r.value));
    else errors.push(r.reason);
  });
  return { areas, errors };
}

// Rende robusti i campi mancanti, così le viste non devono controllare ogni cosa.
function normalizeArea(id, d) {
  const arr = (x) => (Array.isArray(x) ? x : []);
  const area = d && d.area ? d.area : {};
  return {
    id,
    title: area.title || id,
    summary: area.summary || '',
    period: area.period || '',
    turns: arr(area.turns),
    concepts: arr(d.concepts).map((c, i) => ({
      id: c.id || `${id}/${i}`,
      title: c.title || '',
      summary: c.summary || '',
      turns: arr(c.turns),
      projects: arr(c.projects),
      code_refs: arr(c.code_refs),
      related: arr(c.related),
      difficulty: c.difficulty || null,
    })),
    projects: arr(d.projects).map((p) => ({ ...p, area: id, works: arr(p.works), missing: arr(p.missing), concepts: arr(p.concepts) })),
    gemini_errors: arr(d.gemini_errors),
    open_questions: arr(d.open_questions),
    side_topics: arr(d.side_topics),
    self_check: arr(d.self_check),
  };
}

// Elenco dei file del repo (assets/repo-files.json), opzionale: se manca i link al
// codice usano solo le regole sul percorso.
let repoFiles;
export async function loadRepoFiles() {
  if (repoFiles !== undefined) return repoFiles;
  try {
    const d = await get('assets/repo-files.json', 'json');
    repoFiles = Array.isArray(d.files) ? d.files : null;
  } catch {
    repoFiles = null;
  }
  return repoFiles;
}
