// SPA con routing hash: #/ (dashboard), #/area/<id>[/<ancora>], #/project/<id>,
// #/errori[/<area>], #/turni, #/turno/<n>. Nessuna build: ES modules caricati
// direttamente dal browser.

import { AREAS, STATUSES, REVIEW_AFTER_DAYS, REVIEW_LIMIT } from './areas.js';
import * as store from './store.js';
import { loadAllAreas, loadNote, loadProjectNote, loadRepoFiles, loadTurnIndex, loadTurn, loadCorrections, LoadError } from './data.js';
import { configure, codeRefElement, resolveRef, linkCodeRefs } from './codelinks.js';
import { renderDocument, renderMarkdown, renderInline, libsReady } from './md.js';
import { configureTurns, linkTurnRefs, turnLink, turnInfo, turnCount, turnHref, validTurn } from './turns.js';
import * as corr from './corrections.js';

const main = document.getElementById('main');
let site = null; // dati di tutte le aree, caricati una volta
let current = { key: null }; // pagina mostrata

// ---------------------------------------------------------------------------
// Utilità DOM

function h(tag, props, ...kids) {
  const el = document.createElement(tag);
  if (props) {
    for (const [k, v] of Object.entries(props)) {
      if (v == null || v === false) continue;
      if (k === 'class') el.className = v;
      else if (k === 'dataset') Object.assign(el.dataset, v);
      else if (k.startsWith('on')) el.addEventListener(k.slice(2), v);
      else if (['checked', 'open', 'hidden', 'disabled', 'value'].includes(k)) el[k] = v;
      else el.setAttribute(k, v === true ? '' : v);
    }
  }
  for (const c of kids.flat(Infinity)) {
    if (c == null || c === false) continue;
    el.append(c instanceof Node ? c : String(c));
  }
  return el;
}

const plural = (n, one, many) => `${n} ${n === 1 ? one : many}`;

const MONTHS = ['gen', 'feb', 'mar', 'apr', 'mag', 'giu', 'lug', 'ago', 'set', 'ott', 'nov', 'dic'];
function fmtDate(iso, withYear = true) {
  const m = /^(\d{4})-(\d{2})-(\d{2})/.exec(iso || '');
  if (!m) return iso || '';
  return `${Number(m[3])} ${MONTHS[Number(m[2]) - 1]}` + (withYear ? ` ${m[1]}` : '');
}
function fmtPeriod(p) {
  const [a, b] = String(p || '').split('/');
  if (!a) return '';
  if (!b) return fmtDate(a);
  const sameYear = a.slice(0, 4) === b.slice(0, 4);
  return `${fmtDate(a, !sameYear)} → ${fmtDate(b)}`;
}
function fmtAgo(iso) {
  const d = store.daysSince(iso);
  if (d <= 0) return 'oggi';
  if (d === 1) return 'ieri';
  return `${d} giorni fa`;
}

const statusLabel = (s) => (STATUSES.find((x) => x.id === s) || { label: s || '?' }).label;
const statusRank = (s) => {
  const i = STATUSES.findIndex((x) => x.id === s);
  return i < 0 ? STATUSES.length : i;
};
const badge = (status) => h('span', { class: `badge badge-${status || 'unknown'}` }, statusLabel(status));

function difficulty(n) {
  if (!n) return null;
  const v = Math.max(1, Math.min(3, Number(n)));
  return h(
    'span',
    { class: 'difficulty', title: `Difficoltà ${v} su 3` },
    h('span', { 'aria-hidden': 'true' }, '●'.repeat(v) + '○'.repeat(3 - v)),
    h('span', { class: 'sr-only' }, `difficoltà ${v} su 3`),
  );
}

function progressBar(done, total, label) {
  const pct = total ? Math.round((done / total) * 100) : 0;
  return h(
    'div',
    { class: 'progress' },
    h('div', {
      class: 'progress-track',
      role: 'progressbar',
      'aria-valuemin': '0',
      'aria-valuemax': String(total),
      'aria-valuenow': String(done),
      'aria-label': label,
    }, h('div', { class: 'progress-fill', style: `width:${pct}%` })),
    h('span', { class: 'progress-text' }, `${done}/${total} ${label}`),
  );
}

// Testo semplice dai JSON: i riferimenti al codice dentro il testo diventano link.
const INLINE_REF = /(?:[\w.\-]+\/)*[\w.\-]+\.(?:c|h|ts|js|py|sh|txt|md|csv|json|html|css|s)(?::\d+(?:-\d+)?)?/g;
function richText(str, ctx) {
  const s = String(str ?? '');
  if (s.includes('`') || /\*\*|\[.+\]\(/.test(s)) {
    const span = renderInline(s);
    linkCodeRefs(span, ctx);
    linkTurnRefs(span);
    return span;
  }
  const frag = document.createDocumentFragment();
  let last = 0;
  for (const m of s.matchAll(INLINE_REF)) {
    const text = m[0].replace(/[.,;:]+$/, '');
    const r = resolveRef(text, ctx);
    if (!r) continue;
    frag.append(s.slice(last, m.index));
    frag.append(codeRefElement(text, ctx));
    last = m.index + text.length;
  }
  frag.append(s.slice(last));
  linkTurnRefs(frag);
  return frag;
}

// Contesto per risolvere i nomi di file senza cartella.
function projectDirs(path) {
  return String(path || '')
    .split(/,\s*/)
    .map((p) => p.trim().replace(/^antirez\//, ''))
    .filter((p) => p && !p.includes('*'))
    .map((p) => {
      if (p.endsWith('/')) return p;
      const last = p.split('/').pop();
      if (!last.includes('.')) return p + '/';
      return p.includes('/') ? p.slice(0, p.lastIndexOf('/') + 1) : '';
    })
    .filter(Boolean);
}
function areaCtx(area) {
  const refs = new Set(area.concepts.flatMap((c) => c.code_refs.map((r) => r.split(':')[0].replace(/^antirez\//, ''))));
  return { refs, dirs: area.projects.flatMap((p) => projectDirs(p.path)) };
}

// ---------------------------------------------------------------------------
// Tema

const themeBtn = document.getElementById('theme-toggle');
function applyTheme() {
  const t = store.getTheme();
  const root = document.documentElement;
  if (t) root.dataset.theme = t;
  else delete root.dataset.theme;
  const light = document.getElementById('hljs-light');
  const dark = document.getElementById('hljs-dark');
  if (light && dark) {
    light.media = t ? (t === 'light' ? 'all' : 'not all') : '(prefers-color-scheme: light)';
    dark.media = t ? (t === 'dark' ? 'all' : 'not all') : '(prefers-color-scheme: dark)';
  }
  const label = { light: 'chiaro', dark: 'scuro' }[t] || 'automatico';
  if (themeBtn) {
    themeBtn.querySelector('.theme-label').textContent = label;
    themeBtn.setAttribute('aria-label', `Tema: ${label}. Cambia tema`);
  }
}
themeBtn?.addEventListener('click', () => {
  const order = [null, 'light', 'dark'];
  const next = order[(order.indexOf(store.getTheme()) + 1) % order.length];
  store.setTheme(next);
  applyTheme();
});
applyTheme();

// ---------------------------------------------------------------------------
// Errori

function errorBox(err) {
  const box = h('div', { class: 'callout callout-error', role: 'alert' });
  if (location.protocol === 'file:') {
    box.append(
      h('h2', null, 'Il sito va servito via HTTP'),
      h('p', null, 'Hai aperto la pagina con ', h('code', null, 'file://'), ': il browser blocca il caricamento delle note e dei dati. Avvia un server locale dalla cartella ', h('code', null, 'antirez/study'), ':'),
      h('pre', null, h('code', null, 'cd antirez/study\npython3 -m http.server 8000')),
      h('p', null, 'poi apri ', h('a', { href: 'http://localhost:8000/' }, 'http://localhost:8000/'), '.'),
    );
    return box;
  }
  box.append(
    h('h2', null, 'Qualcosa non si è caricato'),
    h('p', null, err instanceof LoadError ? err.message : String(err && err.message ? err.message : err)),
  );
  if (err instanceof LoadError && /HTTP 404/.test(err.detail)) {
    box.append(h('p', null, 'Il file non esiste: controlla il nome in ', h('code', null, 'assets/areas.js'), ' o nella cartella.'));
  } else {
    box.append(h('p', null, 'Se stai provando in locale, verifica che il server sia avviato (', h('code', null, 'python3 -m http.server'), ' dentro ', h('code', null, 'antirez/study'), ').'));
  }
  box.append(h('p', null, h('button', { type: 'button', class: 'btn', onclick: () => location.reload() }, 'Riprova')));
  return box;
}

function libsWarning() {
  if (libsReady()) return null;
  return h('div', { class: 'callout callout-warn', role: 'status' },
    'Le librerie da cdnjs (marked, highlight.js) non si sono caricate: le note sono mostrate come testo semplice. Controlla la connessione.');
}

function storageWarning() {
  if (store.isPersistent()) return null;
  return h('div', { class: 'callout callout-warn', role: 'status' },
    'localStorage non è disponibile (navigazione privata o cookie bloccati): i progressi valgono solo finché la pagina resta aperta. Puoi comunque esportarli in JSON.');
}

// ---------------------------------------------------------------------------
// Dati

async function ensureSite() {
  if (site) return site;
  const [repoFiles, { areas, errors }, turnIndex, corrections] = await Promise.all([
    loadRepoFiles(),
    loadAllAreas(),
    // l'indice dei turni è facoltativo: senza, i link ai turni non hanno anteprima
    loadTurnIndex().catch((e) => (console.warn(e.message), null)),
    // anche le correzioni: senza, ogni turno resta "non rivisto"
    loadCorrections(),
  ]);
  if (!areas.length && errors.length) throw errors[0];
  const byId = new Map(areas.map((a) => [a.id, a]));
  const projects = new Map();
  const concepts = new Map();
  for (const a of areas) {
    for (const p of a.projects) if (!projects.has(p.id)) projects.set(p.id, p);
    for (const c of a.concepts) concepts.set(c.id, { concept: c, area: a });
  }
  configure({
    repoFiles,
    areaIds: AREAS,
    projectIds: [...projects.keys()],
    codeRefs: areas.flatMap((a) => a.concepts.flatMap((c) => c.code_refs)),
  });
  configureTurns(turnIndex);
  corr.configureCorrections(corrections);
  site = { areas, errors, byId, projects, concepts, turnIndex };
  return site;
}

// Link a un concetto a partire dall'id JSON ("parsing/recursive-descent").
function conceptHref(id) {
  const [area, ...rest] = String(id).split('/');
  return `#/area/${area}/${rest.join('/') || ''}`;
}
// "related" può contenere id di concetti, id di aree o testo libero.
function relatedLink(r) {
  if (site.concepts.has(r)) return h('a', { href: conceptHref(r) }, conceptTitle(r));
  if (site.byId.has(r)) return h('a', { href: `#/area/${r}` }, site.byId.get(r).title);
  return h('span', null, r);
}
function conceptTitle(id) {
  const c = site && site.concepts.get(id);
  return c ? c.concept.title : id;
}

function areaQuizStats(area) {
  const s = { so: 0, incerto: 0, non: 0, total: area.self_check.length };
  area.self_check.forEach((q, i) => {
    const r = store.getQuiz(area.id, i, q.q);
    if (r) s[r.s]++;
  });
  return s;
}
const reviewedCount = (area) => area.concepts.filter((c) => store.isReviewed(c.id)).length;

// ---------------------------------------------------------------------------
// Quiz di autoverifica (usato in area e dashboard)

const QUIZ_CHOICES = [
  { s: 'so', label: 'So' },
  { s: 'incerto', label: 'Incerto' },
  { s: 'non', label: 'Non so' },
];
const QUIZ_TEXT = { so: 'so', incerto: 'incerto', non: 'non so' };

function quizItem(area, i, item, { showArea = false, onChange } = {}) {
  const ctx = areaCtx(area);
  const li = h('li', { class: 'quiz-item' });
  const last = h('span', { class: 'quiz-last' });
  const buttons = QUIZ_CHOICES.map((c) =>
    h('button', {
      type: 'button',
      class: `quiz-btn quiz-${c.s}`,
      'aria-pressed': 'false',
      onclick: () => {
        store.setQuiz(area.id, i, item.q, c.s);
        sync();
        onChange && onChange();
      },
    }, c.label),
  );
  function sync() {
    const r = store.getQuiz(area.id, i, item.q);
    li.dataset.state = r ? r.s : '';
    buttons.forEach((b, k) => b.setAttribute('aria-pressed', String(Boolean(r && r.s === QUIZ_CHOICES[k].s))));
    last.textContent = r ? `Ultima risposta: ${QUIZ_TEXT[r.s]}, ${fmtAgo(r.d)}` : 'Mai valutata';
  }
  const meta = h('div', { class: 'quiz-meta' },
    showArea ? h('a', { href: `#/area/${area.id}`, class: 'quiz-area' }, area.title) : null,
    item.concept ? relatedLink(item.concept) : null,
    difficulty(item.difficulty),
    last,
  );
  li.append(
    h('p', { class: 'quiz-q' }, showArea ? null : h('span', { class: 'quiz-n' }, `${i + 1}.`), ' ', richText(item.q, ctx)),
    meta,
    h('div', { class: 'quiz-actions', role: 'group', 'aria-label': 'Quanto la sai?' }, buttons),
  );
  if (item.a) {
    const body = renderMarkdown(String(item.a));
    linkCodeRefs(body, ctx);
    linkTurnRefs(body);
    li.append(h('details', { class: 'quiz-answer' }, h('summary', null, 'Mostra la risposta'), body));
  }
  sync();
  return li;
}

// ---------------------------------------------------------------------------
// Dashboard

function dueQuestions() {
  const out = [];
  site.areas.forEach((area, ai) => {
    area.self_check.forEach((item, qi) => {
      const r = store.getQuiz(area.id, qi, item.q);
      let p;
      if (!r) p = 2;
      else if (r.s === 'non') p = 0;
      else if (r.s === 'incerto') p = 1;
      else if (store.daysSince(r.d) >= REVIEW_AFTER_DAYS) p = 3;
      else return;
      out.push({ area, qi, item, p, d: r ? r.d : '', ai });
    });
  });
  out.sort((a, b) => a.p - b.p || a.d.localeCompare(b.d) || a.ai - b.ai || a.qi - b.qi);
  return out;
}

function viewDashboard() {
  const { areas } = site;
  const totalConcepts = areas.reduce((n, a) => n + a.concepts.length, 0);
  const totalQ = areas.reduce((n, a) => n + a.self_check.length, 0);

  const statsEl = h('p', { class: 'lead-stats' });
  const cards = new Map();
  function refresh() {
    const rev = areas.reduce((n, a) => n + reviewedCount(a), 0);
    const q = areas.map(areaQuizStats).reduce((acc, s) => ({ so: acc.so + s.so, incerto: acc.incerto + s.incerto, non: acc.non + s.non }), { so: 0, incerto: 0, non: 0 });
    statsEl.replaceChildren(
      h('span', null, h('strong', null, `${rev}/${totalConcepts}`), ' concetti ripassati'),
      h('span', null, h('strong', null, `${q.so + q.incerto + q.non}/${totalQ}`), ' domande valutate'),
      h('span', { class: 'mini-legend' },
        h('span', { class: 'dot dot-so' }), `${q.so} so `,
        h('span', { class: 'dot dot-incerto' }), `${q.incerto} incerto `,
        h('span', { class: 'dot dot-non' }), `${q.non} non so`),
    );
    for (const [id, slot] of cards) {
      const a = site.byId.get(id);
      const s = areaQuizStats(a);
      slot.replaceChildren(
        progressBar(reviewedCount(a), a.concepts.length, 'concetti ripassati'),
        h('p', { class: 'card-quiz' }, s.so + s.incerto + s.non
          ? [
            h('span', { class: 'dot dot-so', title: 'so' }), `${s.so}`, h('span', { class: 'sr-only' }, ' so,'), ' ',
            h('span', { class: 'dot dot-incerto', title: 'incerto' }), `${s.incerto}`, h('span', { class: 'sr-only' }, ' incerto,'), ' ',
            h('span', { class: 'dot dot-non', title: 'non so' }), `${s.non}`, h('span', { class: 'sr-only' }, ' non so'), ' ',
            h('span', { class: 'muted' }, `su ${s.total} domande`),
          ]
          : h('span', { class: 'muted' }, `${s.total} domande di autoverifica`)),
      );
    }
  }

  // Percorso
  const path = h('ol', { class: 'path' });
  AREAS.forEach((id, i) => {
    const a = site.byId.get(id);
    if (!a) {
      path.append(h('li', { class: 'area-card area-card-missing' }, h('span', { class: 'step' }, i + 1), h('div', null, h('h3', null, id), h('p', { class: 'muted' }, `data/${id}.json non caricato`))));
      return;
    }
    const slot = h('div', { class: 'card-progress' });
    cards.set(id, slot);
    path.append(
      h('li', { class: 'area-card' },
        h('span', { class: 'step', 'aria-hidden': 'true' }, i + 1),
        h('div', { class: 'card-body' },
          h('h3', null, h('a', { href: `#/area/${id}`, class: 'stretched' }, a.title)),
          h('p', { class: 'card-meta' }, fmtPeriod(a.period), ' · ', plural(a.concepts.length, 'concetto', 'concetti'), a.projects.length ? ` · ${plural(a.projects.length, 'progetto', 'progetti')}` : ''),
          slot,
        ),
      ),
    );
  });

  // Da ripassare oggi
  const due = dueQuestions();
  const dueList = h('ol', { class: 'quiz-list quiz-list-due' });
  let shown = 0;
  const more = h('button', { type: 'button', class: 'btn btn-quiet' });
  function showMore() {
    const next = due.slice(shown, shown + REVIEW_LIMIT);
    next.forEach((d) => dueList.append(quizItem(d.area, d.qi, d.item, { showArea: true, onChange: refresh })));
    shown += next.length;
    more.hidden = shown >= due.length;
    more.textContent = `Mostra altre (${due.length - shown})`;
  }
  more.addEventListener('click', showMore);
  showMore();
  const counts = { 0: 0, 1: 0, 2: 0, 3: 0 };
  due.forEach((d) => counts[d.p]++);
  const dueSummary = [
    counts[0] && `${counts[0]} "non so"`,
    counts[1] && `${counts[1]} ${counts[1] === 1 ? 'incerta' : 'incerte'}`,
    counts[3] && `${counts[3]} "so" da più di ${REVIEW_AFTER_DAYS} giorni`,
    counts[2] && `${counts[2]} mai viste`,
  ].filter(Boolean).join(', ');

  // Progetti
  const projects = [...site.projects.values()].sort((a, b) => statusRank(a.status) - statusRank(b.status) || String(b.last_modified).localeCompare(String(a.last_modified)));
  const table = h('table', { class: 'data-table projects-table' },
    h('thead', null, h('tr', null, ['Progetto', 'Stato', 'Ultima modifica', 'Area', 'Prossimo passo'].map((t) => h('th', { scope: 'col' }, t)))),
    h('tbody', null, projects.map((p) => {
      const a = site.byId.get(p.area);
      return h('tr', null,
        h('th', { scope: 'row', 'data-label': 'Progetto' }, h('a', { href: `#/project/${p.id}` }, p.id)),
        h('td', { 'data-label': 'Stato' }, badge(p.status)),
        h('td', { 'data-label': 'Ultima modifica', class: 'nowrap' }, p.last_modified ? h('time', { datetime: p.last_modified }, fmtDate(p.last_modified)) : '—'),
        h('td', { 'data-label': 'Area', class: 'nowrap' }, h('a', { href: `#/area/${p.area}` }, p.area)),
        h('td', { 'data-label': 'Prossimo passo', class: 'next-step' }, richText(p.next_step || '', { refs: a ? areaCtx(a).refs : new Set(), dirs: projectDirs(p.path) })),
      );
    })),
  );

  refresh();
  document.title = 'Study — percorso';
  return h('div', { class: 'page page-dashboard' },
    storageWarning(),
    site.errors.length ? h('div', { class: 'callout callout-warn', role: 'status' }, `Alcune aree non si sono caricate: ${site.errors.map((e) => e.path || e.message).join(', ')}`) : null,
    h('header', { class: 'hero' },
      h('h1', { tabindex: '-1' }, 'Percorso di studio'),
      h('p', { class: 'lead' }, 'C, parsing, concorrenza, networking, actor model, SICP e logica combinatoria: le note sistematizzate dalla conversazione con Gemini e lo stato dei progetti in ', h('code', null, 'antirez/'), '.'),
      statsEl,
    ),
    h('section', { 'aria-labelledby': 'h-path' }, h('h2', { id: 'h-path' }, 'Le aree, in ordine'), path),
    h('section', { 'aria-labelledby': 'h-due', class: 'due' },
      h('h2', { id: 'h-due' }, 'Da ripassare oggi'),
      due.length
        ? h('p', { class: 'muted' }, `${plural(due.length, 'domanda', 'domande')} in coda: ${dueSummary}. Prima le "non so", poi le incerte, poi quelle mai viste.`)
        : h('p', null, 'Niente da ripassare oggi: hai valutato tutte le domande di recente.'),
      dueList,
      more,
    ),
    h('section', { 'aria-labelledby': 'h-proj', id: 'progetti' },
      h('h2', { id: 'h-proj' }, 'Stato dei progetti'),
      h('div', { class: 'table-wrap', tabindex: '0', role: 'region', 'aria-label': 'Stato dei progetti' }, table),
    ),
    progressTools(),
  );
}

function progressTools() {
  const status = h('p', { class: 'muted', role: 'status', 'aria-live': 'polite' });
  const file = h('input', { type: 'file', accept: 'application/json,.json', class: 'sr-only', id: 'import-file' });
  file.addEventListener('change', async () => {
    const f = file.files && file.files[0];
    if (!f) return;
    try {
      const text = await f.text();
      if (!confirm('Importare i progressi da questo file? Sostituiscono quelli attuali su questo dispositivo.')) return;
      const r = store.importJSON(text);
      status.textContent = `Importati ${r.reviewed} concetti ripassati e ${r.quiz} risposte.`;
      rerender();
    } catch (e) {
      status.textContent = `Import non riuscito: ${e.message}`;
    } finally {
      file.value = '';
    }
  });
  return h('section', { class: 'tools', 'aria-labelledby': 'h-tools' },
    h('h2', { id: 'h-tools' }, 'I tuoi progressi'),
    h('p', { class: 'muted' }, store.isPersistent()
      ? 'Sono salvati solo in questo browser (localStorage, chiave study.v1). Per spostarli su un altro dispositivo esportali e importali di là.'
      : 'localStorage non disponibile: i progressi si perdono alla chiusura della pagina, esportali se vuoi tenerli.'),
    h('div', { class: 'tool-buttons' },
      h('button', {
        type: 'button', class: 'btn', onclick: () => {
          const blob = new Blob([store.exportJSON()], { type: 'application/json' });
          const a = h('a', { href: URL.createObjectURL(blob), download: `study-progressi-${store.today()}.json` });
          document.body.append(a);
          a.click();
          a.remove();
          setTimeout(() => URL.revokeObjectURL(a.href), 1000);
          const s = store.stats();
          status.textContent = `Esportati ${s.reviewed} concetti ripassati e ${s.quiz} risposte.`;
        },
      }, 'Esporta JSON'),
      h('button', { type: 'button', class: 'btn', onclick: () => file.click() }, 'Importa JSON…'),
      file,
      h('button', {
        type: 'button', class: 'btn btn-danger', onclick: () => {
          if (!confirm('Azzerare tutti i progressi (concetti ripassati e risposte ai quiz)? Non si può annullare: esportali prima se vuoi tenerli.')) return;
          store.resetProgress();
          rerender();
        },
      }, 'Azzera progressi'),
    ),
    status,
  );
}

// ---------------------------------------------------------------------------
// Pagina di un'area

const norm = (s) => String(s).toLowerCase().replace(/[`"'«»“”]/g, '').replace(/\s+/g, ' ').trim();

// Raccoglie gli elementi fra `start` (escluso) e il prossimo titolo di livello <= `level`.
function sectionAfter(start, stopTags) {
  const out = [];
  let el = start.nextElementSibling;
  while (el && !stopTags.includes(el.tagName)) {
    out.push(el);
    el = el.nextElementSibling;
  }
  return out;
}

async function viewArea(id) {
  if (!AREAS.includes(id)) return viewNotFound(`L'area "${id}" non è nel percorso (assets/areas.js).`);
  const area = site.byId.get(id);
  const md = await loadNote(id);
  const ctx = area ? areaCtx(area) : null;
  const doc = renderDocument(md, { docPath: `notes/${id}.md`, route: `area/${id}`, ctx });

  // Titolo e riga "Periodo" della nota vanno nell'intestazione.
  const h1 = doc.querySelector('h1');
  const title = h1 ? h1.textContent : area ? area.title : id;
  let metaLine = null;
  if (h1) {
    const next = h1.nextElementSibling;
    if (next && next.tagName === 'P' && /^Periodo/i.test(next.textContent)) {
      metaLine = next;
      next.remove();
    }
    h1.remove();
  }

  const concepts = area ? area.concepts : [];
  const tocConcepts = [];
  const progressSlot = h('div', { class: 'area-progress' });
  const refreshProgress = () => {
    if (!area) return;
    progressSlot.replaceChildren(progressBar(reviewedCount(area), area.concepts.length, 'concetti ripassati'));
    for (const t of tocConcepts) t.li.classList.toggle('is-done', store.isReviewed(t.key));
  };

  // Concetti: ogni ### sotto "## Concetti" diventa una sezione con il controllo "ripassato".
  const h2s = [...doc.querySelectorAll('h2')];
  const conceptsH2 = h2s.find((x) => /^concetti/i.test(x.textContent.trim()));
  if (conceptsH2) {
    const block = sectionAfter(conceptsH2, ['H2']);
    const h3s = block.filter((el) => el.tagName === 'H3');
    h3s.forEach((h3, k) => {
      const t = norm(h3.textContent);
      let c = concepts.find((x) => norm(x.title) === t);
      if (!c && concepts.length === h3s.length) c = concepts[k];
      const key = c ? c.id : `${id}/#${h3.id}`;
      const local = c ? c.id.split('/').slice(1).join('/') : h3.id;
      const body = sectionAfter(h3, ['H2', 'H3']);
      const sec = h('section', { class: 'concept', dataset: { concept: local } });
      h3.replaceWith(sec);
      sec.append(h3, conceptBar(c, key, area, ctx, refreshProgress), ...body);
      tocConcepts.push({ key, li: null, h3 });
    });
  }

  // Domande di autoverifica: quiz dal JSON al posto dell'elenco markdown.
  const quizH2 = h2s.find((x) => /^domande di autoverifica/i.test(x.textContent.trim()));
  if (quizH2 && area && area.self_check.length) {
    const old = sectionAfter(quizH2, ['H2']);
    const list = h('ol', { class: 'quiz-list' }, area.self_check.map((q, i) => quizItem(area, i, q)));
    old.forEach((el) => el.remove());
    quizH2.after(h('p', { class: 'muted' }, 'Rispondi a voce o su carta, poi segna quanto eri sicuro. Le "non so" e le incerte tornano nella dashboard in "Da ripassare oggi".'), list);
  }

  // Errori di Gemini: link alla tabella aggregata filtrata.
  const errH2 = h2s.find((x) => /errori.*gemini/i.test(x.textContent));
  if (errH2) errH2.after(h('p', { class: 'muted small' }, h('a', { href: `#/errori/${id}` }, 'Vedi come tabella filtrabile')));

  // Indice laterale
  const tocList = h('ol', { class: 'toc-list' });
  for (const h2 of h2s) {
    const li = h('li', { class: 'toc-h2' }, h('a', { href: `#/area/${id}/${h2.id}` }, h2.textContent));
    tocList.append(li);
    if (h2 === conceptsH2) {
      const sub = h('ol', { class: 'toc-sub' });
      for (const t of tocConcepts) {
        t.li = h('li', null, h('a', { href: `#/area/${id}/${t.h3.id}` }, h('span', { class: 'toc-check', 'aria-hidden': 'true' }), h('span', null, t.h3.textContent)));
        sub.append(t.li);
      }
      li.append(sub);
    }
  }
  const toc = h('details', { class: 'toc', id: 'toc' }, h('summary', null, 'Indice'), h('nav', { 'aria-label': 'Indice della nota' }, tocList));

  // Navigazione nel percorso
  const idx = AREAS.indexOf(id);
  const prev = AREAS[idx - 1];
  const next = AREAS[idx + 1];
  const nameOf = (x) => (site.byId.get(x) || { title: x }).title;

  const header = h('header', { class: 'page-head' },
    h('p', { class: 'eyebrow' }, h('a', { href: '#/' }, 'Percorso'), ` › area ${idx + 1} di ${AREAS.length}`),
    h('h1', { tabindex: '-1' }, title),
    metaLine ? h('p', { class: 'doc-meta' }, ...metaLine.childNodes) : area ? h('p', { class: 'doc-meta' }, fmtPeriod(area.period)) : null,
    area && area.summary ? h('p', { class: 'lead' }, area.summary) : null,
    progressSlot,
    area && area.projects.length
      ? h('div', { class: 'chips' }, h('span', { class: 'chips-label' }, 'Progetti:'),
        area.projects.map((p) => h('a', { href: `#/project/${p.id}`, class: 'chip' }, p.id, ' ', badge(p.status))))
      : null,
  );

  refreshProgress();
  document.title = `${title} — Study`;
  return h('div', { class: 'page page-area' },
    storageWarning(),
    area ? null : h('div', { class: 'callout callout-warn', role: 'status' }, `data/${id}.json non caricato: niente quiz né progressi per quest'area.`),
    h('div', { class: 'area-layout' },
      h('aside', { class: 'toc-col' }, toc),
      h('article', { class: 'doc' }, header, h('div', { class: 'prose' }, ...doc.childNodes),
        h('nav', { class: 'prev-next', 'aria-label': 'Aree vicine' },
          prev ? h('a', { href: `#/area/${prev}`, class: 'pn-prev' }, h('span', { class: 'muted small' }, '← Area precedente'), h('span', null, nameOf(prev))) : h('span'),
          next ? h('a', { href: `#/area/${next}`, class: 'pn-next' }, h('span', { class: 'muted small' }, 'Area successiva →'), h('span', null, nameOf(next))) : h('span'),
        ),
      ),
    ),
  );
}

function conceptBar(c, key, area, ctx, onChange) {
  const date = h('span', { class: 'muted small' });
  const input = h('input', { type: 'checkbox', checked: store.isReviewed(key) });
  const syncDate = () => {
    const d = store.reviewedDate(key);
    date.textContent = d ? ` (${fmtAgo(d)})` : '';
  };
  input.addEventListener('change', () => {
    store.setReviewed(key, input.checked);
    syncDate();
    onChange();
  });
  syncDate();
  const bar = h('div', { class: 'concept-bar' },
    h('label', { class: 'check' }, input, h('span', null, 'Ripassato'), date),
    c ? difficulty(c.difficulty) : null,
  );
  if (c) {
    const extra = [];
    if (c.projects.length) {
      extra.push(h('div', { class: 'concept-row' }, h('span', { class: 'row-label' }, 'Progetti'),
        c.projects.map((p) => {
          const proj = site.projects.get(p);
          return h('a', { href: `#/project/${p}`, class: 'chip' }, p, proj ? [' ', badge(proj.status)] : null);
        })));
    }
    if (c.related.length) {
      extra.push(h('div', { class: 'concept-row' }, h('span', { class: 'row-label' }, 'Collegati'),
        c.related.map((r, i) => [i ? ' · ' : '', relatedLink(r)])));
    }
    if (c.code_refs.length) {
      extra.push(h('div', { class: 'concept-row' }, h('span', { class: 'row-label' }, 'Codice'),
        h('span', { class: 'refs' }, c.code_refs.map((r) => codeRefElement(r, ctx)))));
    }
    if (c.turns.length) {
      extra.push(h('div', { class: 'concept-row' }, h('span', { class: 'row-label' }, 'Turni'),
        h('span', { class: 'turn-refs' }, c.turns.map((n, i) => [i ? ', ' : '', turnLink(n)]))));
    }
    if (extra.length) {
      bar.append(h('details', { class: 'concept-more' }, h('summary', null, 'Progetti, collegamenti e codice'), ...extra));
    }
  }
  return bar;
}

// ---------------------------------------------------------------------------
// Pagina di un progetto

async function viewProject(id) {
  const p = site.projects.get(id);
  const area = p ? site.byId.get(p.area) : null;
  let md;
  try {
    md = await loadProjectNote(id);
  } catch (e) {
    if (!p && e instanceof LoadError && /404/.test(e.detail)) return viewNotFound(`Il progetto "${id}" non esiste.`);
    throw e;
  }
  const ctx = {
    refs: area ? areaCtx(area).refs : new Set(),
    dirs: p ? projectDirs(p.path) : [],
  };
  const doc = renderDocument(md, { docPath: `projects/${id}.md`, route: `project/${id}`, ctx });
  const h1 = doc.querySelector('h1');
  let title = p ? p.title : id;
  let status = p ? p.status : null;
  if (h1) {
    // "# Titolo  — stato: broken": lo stato diventa un badge.
    const m = /^(.*?)\s*[—–-]\s*stato:\s*([\w-]+)\s*$/i.exec(h1.textContent);
    title = m ? m[1] : h1.textContent;
    status = status || (m ? m[2].toLowerCase() : null);
    h1.remove();
  }
  let pathLine = null;
  const first = doc.firstElementChild;
  if (first && first.tagName === 'P' && /^Path:/i.test(first.textContent)) {
    pathLine = first;
    first.remove();
    pathLine.classList.add('doc-meta');
  }
  document.title = `${title} — Study`;
  return h('div', { class: 'page page-project' },
    h('article', { class: 'doc doc-single' },
      h('header', { class: 'page-head' },
        h('p', { class: 'eyebrow' }, h('a', { href: '#/' }, 'Percorso'), ' › ',
          area ? h('a', { href: `#/area/${area.id}` }, area.title) : 'progetto'),
        h('div', { class: 'title-row' }, h('h1', { tabindex: '-1' }, title), status ? badge(status) : null),
        pathLine,
        p && p.next_step ? h('div', { class: 'callout callout-next' }, h('strong', null, 'Prossimo passo: '), richText(p.next_step, ctx)) : null,
      ),
      h('div', { class: 'prose' }, ...doc.childNodes),
    ),
  );
}

// ---------------------------------------------------------------------------
// Errori di Gemini

function viewErrors(filterArea) {
  const rows = site.areas.flatMap((a) => a.gemini_errors.map((e) => ({ ...e, area: a })));
  const select = h('select', { id: 'f-area' },
    h('option', { value: '' }, `Tutte le aree (${rows.length})`),
    site.areas.map((a) => h('option', { value: a.id }, `${a.title} (${a.gemini_errors.length})`)),
  );
  select.value = site.byId.has(filterArea) ? filterArea : '';
  const search = h('input', { type: 'search', id: 'f-q', placeholder: 'es. SDL3, Y, monoide', autocomplete: 'off' });
  const count = h('p', { class: 'muted', role: 'status', 'aria-live': 'polite' });
  const tbody = h('tbody');
  const table = h('table', { class: 'data-table errors-table' },
    h('thead', null, h('tr', null, ['Area', 'Turno', 'Affermazione di Gemini', 'Correzione'].map((t) => h('th', { scope: 'col' }, t)))),
    tbody,
  );
  const ctxCache = new Map();
  const ctxOf = (a) => ctxCache.get(a.id) || (ctxCache.set(a.id, areaCtx(a)), ctxCache.get(a.id));

  function update() {
    const fa = select.value;
    const q = norm(search.value);
    const shown = rows.filter((r) => (!fa || r.area.id === fa) && (!q || norm(`${r.claim} ${r.correction} ${r.turn}`).includes(q)));
    tbody.replaceChildren(...shown.map((r) => h('tr', null,
      h('td', { 'data-label': 'Area' }, h('a', { href: `#/area/${r.area.id}` }, r.area.id)),
      h('td', { 'data-label': 'Turno', class: 'nowrap' }, r.turn == null ? '' : turnLink(Number(r.turn), String(r.turn))),
      h('td', { 'data-label': 'Gemini' }, richText(r.claim, ctxOf(r.area))),
      h('td', { 'data-label': 'Correzione' }, richText(r.correction, ctxOf(r.area))),
    )));
    count.textContent = `${plural(shown.length, 'voce', 'voci')} su ${rows.length}`;
    const target = fa ? `#/errori/${fa}` : '#/errori';
    if (location.hash !== target) {
      history.replaceState(history.state, '', target);
      current.key = routeKey(parseRoute());
    }
  }
  select.addEventListener('change', update);
  search.addEventListener('input', update);
  update();
  document.title = 'Errori di Gemini — Study';
  return h('div', { class: 'page page-errors' },
    h('header', { class: 'page-head' },
      h('p', { class: 'eyebrow' }, h('a', { href: '#/' }, 'Percorso'), ' › pagina secondaria'),
      h('h1', { tabindex: '-1' }, 'Errori e imprecisioni di Gemini'),
      h('p', { class: 'lead' }, 'Tutte le voci "Possibili errori o imprecisioni di Gemini" dei file ', h('code', null, 'data/*.json'), ', verificate sul codice o su server reali. Il turno è quello della ', h('a', { href: '#/turni' }, 'conversazione originale'), ': il numero apre il testo di Gemini.'),
      corr.correctionsLoaded()
        ? h('p', { class: 'errors-corr-link' }, h('a', { href: '#/turni/correzioni' }, 'Turni con correzioni puntuali →'),
          h('span', { class: 'muted small' }, ' ogni correzione evidenziata nel testo originale di Gemini'))
        : null,
    ),
    h('div', { class: 'filters' },
      h('div', { class: 'field' }, h('label', { for: 'f-area' }, 'Area'), select),
      h('div', { class: 'field' }, h('label', { for: 'f-q' }, 'Cerca'), search),
    ),
    count,
    h('div', { class: 'table-wrap', tabindex: '0', role: 'region', 'aria-label': 'Errori di Gemini' }, table),
  );
}

// ---------------------------------------------------------------------------
// Turni della conversazione con Gemini

// Chi cita il turno `n`: concetti (concepts[].turns), errori di Gemini (turn) e aree
// (area.turns, più quelle dei concetti e degli errori).
function turnCitations(n) {
  const concepts = [];
  const errors = [];
  const areas = new Set();
  for (const a of site.areas) {
    if (a.turns.includes(n)) areas.add(a);
    for (const c of a.concepts) if (c.turns.includes(n)) (concepts.push({ c, a }), areas.add(a));
    for (const e of a.gemini_errors) if (Number(e.turn) === n) (errors.push({ e, a }), areas.add(a));
  }
  return { concepts, errors, areas: site.areas.filter((a) => areas.has(a)) };
}

// Un turno esportato: "## User:\n\n> data\n\ndomanda…\n## Gemini:\n\n> data\n\nrisposta…".
function splitTurn(md) {
  const take = (part, head) => {
    const lines = part.replace(head, '').replace(/^\s*\n/, '').split('\n');
    let time = '';
    const m = /^>\s*(\d+\/\d+\/\d{4})\s+(\d+:\d+)/.exec(lines[0] || '');
    if (m) {
      time = m[2];
      lines.shift();
    }
    return { time, text: lines.join('\n').replace(/^\s*\n/, '').trimEnd() };
  };
  const g = /^## Gemini:.*$/m.exec(md);
  const user = take(g ? md.slice(0, g.index) : md, /^## User:.*$/m);
  const gemini = g ? take(md.slice(g.index), /^## Gemini:.*$/m) : null;
  return { user, gemini };
}

const LONG_QUESTION = 40; // righe oltre le quali la domanda parte compressa

// La domanda è testo semplice (spesso codice incollato senza ```): niente markdown,
// a capo e rientri preservati.
function turnQuestion(text) {
  const lines = text.split('\n').length;
  const code = /^\s*(#include|#define|typedef |struct |static |int |void |char |\}|\/\/|\/\*)/m.test(text);
  const box = h('div', { class: `turn-question${code ? ' is-code' : ''}`, id: 'domanda-testo' }, text);
  if (lines <= LONG_QUESTION) return box;
  box.classList.add('is-collapsed');
  const btn = h('button', { type: 'button', class: 'btn btn-quiet turn-expand', 'aria-expanded': 'false', 'aria-controls': 'domanda-testo' });
  const sync = () => {
    const open = !box.classList.contains('is-collapsed');
    btn.setAttribute('aria-expanded', String(open));
    btn.textContent = open ? 'Comprimi la domanda' : `Mostra tutta la domanda (${lines} righe)`;
  };
  btn.addEventListener('click', () => {
    box.classList.toggle('is-collapsed');
    sync();
  });
  sync();
  return h('div', null, box, btn);
}

// I titoli della risposta scendono di un livello: h2 sono "Domanda" e "Risposta".
function demoteHeadings(root) {
  for (const el of root.querySelectorAll('h1, h2, h3, h4, h5, h6')) {
    const level = Math.min(6, Number(el.tagName[1]) + 1);
    const nh = h(`h${level}`, { id: el.id || null });
    nh.append(...el.childNodes);
    el.replaceWith(nh);
  }
}

// "Indietro": alla pagina da cui si è arrivati (nota, errori, indice), altrimenti al percorso.
function goBack(e) {
  e.preventDefault();
  if (navDepth > 0) history.back();
  else location.hash = '#/';
}
// Turno precedente/successivo: sostituisce la voce della cronologia, così "Indietro"
// riporta alla pagina di partenza anche dopo aver sfogliato più turni.
function replaceNav(e) {
  if (e.button !== 0 || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) return;
  e.preventDefault();
  replacingNav = true;
  location.replace(e.currentTarget.getAttribute('href'));
}

// ---------------------------------------------------------------------------
// Correzioni puntuali ai turni (turns/corrections.json, vedi assets/corrections.js)

const SEVERITY = { errore: 'Errore', imprecisione: 'Imprecisione' };
const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
const plainClick = (e) => e.button === 0 && !e.metaKey && !e.ctrlKey && !e.shiftKey && !e.altKey;

function reviewSummary(r) {
  const { total, errors, imprecisions } = corr.reviewCounts(r);
  const when = r.date ? ` il ${fmtDate(r.date)}` : '';
  if (!total && r.verdict === 'senza-risposta') return `Rivisto${when}: non c'è una risposta di Gemini da verificare.`;
  if (!total) return `Rivisto${when}: nessun errore rilevante.`;
  const parts = [errors ? plural(errors, 'errore', 'errori') : null, imprecisions ? plural(imprecisions, 'imprecisione', 'imprecisioni') : null];
  return `Rivisto${when}: ${plural(total, 'correzione', 'correzioni')} (${parts.filter(Boolean).join(', ')}).`;
}

// Il marcatore va alla fine del passaggio, ma fuori da link e codice inline (resta
// cliccabile e non sembra parte del codice).
function insertMarker(range, marker, root) {
  let host = null;
  const end = range.endContainer;
  for (let el = end.nodeType === Node.TEXT_NODE ? end.parentElement : end; el && el !== root; el = el.parentElement) {
    if (el.matches('a, code, mark') && !el.closest('pre')) host = el;
  }
  if (host) host.after(marker);
  else {
    const r = range.cloneRange();
    r.collapse(false);
    r.insertNode(marker);
  }
}

// Cerca ogni citazione nella risposta, la evidenzia (Highlight API, altrimenti <mark>)
// e mette un marcatore numerato che rimanda alla voce del riquadro.
function markCorrections(root, n, items) {
  const api = corr.hasHighlightApi();
  return items.map((item, i) => {
    const k = i + 1;
    let range = item.quote ? corr.locateQuote(root, item.quote) : null;
    let marks = [];
    if (range && !api) ({ range, marks } = corr.wrapRange(range, item.severity));
    let marker = null;
    if (range) {
      marker = h('a', {
        class: `corr-marker corr-marker-${item.severity}`, id: `corr-punto-${k}`, href: `#/turno/${n}/corr-${k}`,
        title: `${SEVERITY[item.severity]} ${k}: ${item.claim || item.correction}`,
        'aria-label': `Correzione ${k} (${SEVERITY[item.severity].toLowerCase()}): vai alla voce`,
        onclick: (e) => {
          if (!plainClick(e)) return;
          e.preventDefault();
          const li = document.getElementById(`corr-${k}`);
          if (!li) return;
          li.scrollIntoView({ block: 'start', behavior: reducedMotion.matches ? 'auto' : 'smooth' });
          li.focus({ preventScroll: true });
          history.replaceState(history.state, '', e.currentTarget.getAttribute('href'));
        },
      }, String(k));
      insertMarker(range, marker, root);
    }
    return { item, k, range, marks, marker };
  });
}

// "Vai al punto": apre le parti compresse che contengono il passaggio, ci scorre e lo
// mette in risalto.
function gotoCorrection(e, entry, entries) {
  if (!plainClick(e)) return;
  e.preventDefault();
  const { range, marks, marker } = entry;
  const start = range.startContainer;
  for (let el = start.nodeType === Node.TEXT_NODE ? start.parentElement : start; el; el = el.parentElement) {
    if (el.tagName === 'DETAILS' && !el.open) el.open = true;
    if (el.classList.contains('is-collapsed')) {
      const btn = el.id && document.querySelector(`[aria-controls="${CSS.escape(el.id)}"]`);
      if (btn) btn.click();
      else el.classList.remove('is-collapsed');
    }
  }
  const rect = range.getBoundingClientRect();
  const header = document.querySelector('.site-header');
  const offset = Math.max((header ? header.offsetHeight : 0) + 16, (window.innerHeight - rect.height) / 3);
  window.scrollTo({ top: window.scrollY + rect.top - offset, behavior: reducedMotion.matches ? 'auto' : 'smooth' });
  if (marks.length) {
    for (const x of entries) for (const m of x.marks) m.classList.toggle('is-active', x === entry);
  } else corr.setActiveHighlight(range);
  if (marker) marker.focus({ preventScroll: true });
  history.replaceState(history.state, '', e.currentTarget.getAttribute('href'));
}

// Testi delle correzioni: sempre markdown inline (anche *corsivo*), con link a codice e turni.
function inlineMd(str, ctx) {
  const span = renderInline(str);
  linkCodeRefs(span, ctx);
  linkTurnRefs(span);
  return span;
}

function correctionsBox(n, entries) {
  return h('section', { class: 'corrections', id: 'correzioni', 'aria-labelledby': 'h-corr' },
    h('h2', { id: 'h-corr' }, 'Correzioni a questo turno'),
    h('ol', { class: 'corr-list' }, entries.map((entry) => {
      const { item, k, range } = entry;
      const c = item.concept ? site.concepts.get(item.concept) : null;
      const ctx = c ? areaCtx(c.area) : null;
      return h('li', { class: `corr-item corr-item-${item.severity}`, id: `corr-${k}`, tabindex: '-1' },
        h('div', { class: 'corr-head' },
          h('span', { class: `corr-num corr-marker-${item.severity}`, 'aria-hidden': 'true' }, String(k)),
          h('span', { class: `badge badge-${item.severity}` }, SEVERITY[item.severity]),
          range
            ? h('a', { href: `#/turno/${n}/corr-punto-${k}`, class: 'corr-goto', onclick: (e) => gotoCorrection(e, entry, entries) }, 'vai al punto ↓')
            : h('span', { class: 'corr-missing' }, 'passaggio non localizzato'),
        ),
        item.claim ? h('p', null, h('span', { class: 'corr-label' }, 'Gemini dice:'), ' ', inlineMd(item.claim, ctx)) : null,
        item.correction ? h('p', null, h('span', { class: 'corr-label' }, 'Corretto:'), ' ', inlineMd(item.correction, ctx)) : null,
        item.evidence ? h('p', { class: 'corr-evidence' }, h('span', { class: 'corr-label' }, 'Verifica:'), ' ', inlineMd(item.evidence, ctx)) : null,
        c ? h('p', { class: 'corr-concept' }, 'Concetto: ', h('a', { href: conceptHref(item.concept) }, c.concept.title), h('span', { class: 'muted' }, ` — ${c.area.title}`)) : null,
      );
    })),
  );
}

// Indicatore per l'indice dei turni (solo se corrections.json c'è).
function reviewStatus(n) {
  const r = corr.turnReview(n);
  if (!r) return h('span', { class: 'turn-status is-none' }, 'non rivisto');
  const { total, errors } = corr.reviewCounts(r);
  if (!total) return h('span', { class: 'turn-status is-ok' }, r.verdict === 'senza-risposta' ? 'rivisto, senza risposta' : 'rivisto ✓');
  return h('span', { class: `turn-status ${errors ? 'is-errore' : 'is-imprecisione'}`, title: `${plural(errors, 'errore', 'errori')}, ${plural(total - errors, 'imprecisione', 'imprecisioni')}` },
    plural(total, 'correzione', 'correzioni'));
}

async function viewTurn(raw) {
  const n = /^\d+$/.test(raw || '') ? Number(raw) : NaN;
  const total = turnCount();
  const outOfRange = !validTurn(n);
  let md = null;
  if (!outOfRange) {
    try {
      md = await loadTurn(n);
    } catch (e) {
      if (!(e instanceof LoadError && /404/.test(e.detail))) throw e;
    }
  }
  if (md == null) {
    document.title = 'Turno non trovato — Study';
    return h('div', { class: 'page' },
      h('h1', { tabindex: '-1' }, 'Turno non trovato'),
      h('p', null, outOfRange
        ? `"${raw}" non è un turno della conversazione${total ? `: i turni vanno da 1 a ${total}` : ''}.`
        : [`Il file del turno ${n} non c'è: rigenera i turni con `, h('code', null, 'python3 antirez/study/tools/split_turns.py'), '.']),
      h('p', null, h('a', { href: '#/turni' }, 'Tutti i turni'), ' · ', h('a', { href: '#/' }, 'Torna al percorso')),
    );
  }

  const info = turnInfo(n);
  // Nell'export i delimitatori dei blocchi di codice hanno spazi a larghezza zero fra i
  // backtick (`\u200B`\u200B`): senza ripulirli marked non vede i blocchi di codice.
  const { user, gemini } = splitTurn(md.replace(/`\u200B`\u200B`/g, '```'));
  const cites = turnCitations(n);
  const areaLinks = (list) => list.map((a, i) => [i ? (i === list.length - 1 ? ' e ' : ', ') : '', h('a', { href: `#/area/${a.id}` }, a.title)]);

  let answer;
  if (gemini && gemini.text) {
    answer = renderDocument(gemini.text, { docPath: `turns/${String(n).padStart(3, '0')}.md`, route: `turno/${n}`, ctx: null, turns: false });
    demoteHeadings(answer);
    answer = h('div', { class: 'turn-answer' }, ...answer.childNodes);
  } else answer = h('p', { class: 'muted' }, "Nell'export non c'è la risposta di Gemini a questo turno.");

  // Correzioni: le citazioni si cercano solo nella risposta di Gemini, già nel suo
  // contenitore definitivo (spostare i nodi dopo invaliderebbe i Range).
  const review = corr.turnReview(n);
  const entries = review ? markCorrections(answer, n, review.items) : [];
  const notes = cites.areas.length ? [' (', areaLinks(cites.areas), ')'] : [' del ', h('a', { href: '#/' }, 'percorso')];

  const prev = n > 1 ? n - 1 : null;
  const next = !total || n < total ? n + 1 : null;
  const pnLink = (m, cls, label) => h('a', { href: turnHref(m), class: cls, onclick: replaceNav, title: turnInfo(m) ? turnInfo(m).preview : null },
    h('span', { class: 'muted small' }, label), h('span', null, `Turno ${m}`));

  const cited = [];
  for (const { c, a } of cites.concepts) {
    cited.push(h('li', null, h('a', { href: conceptHref(c.id) }, c.title), h('span', { class: 'muted' }, ` — ${a.title}`)));
  }
  for (const { e, a } of cites.errors) {
    cited.push(h('li', { class: 'cite-error' },
      h('a', { href: `#/errori/${a.id}` }, 'Errore di Gemini'), h('span', { class: 'muted' }, ` (${a.title}): `),
      richText(e.claim, areaCtx(a))));
  }

  document.title = `Turno ${n} — Study`;
  const view = h('div', { class: 'page page-turn' },
    h('article', { class: 'doc doc-single' },
      h('header', { class: 'page-head' },
        h('p', { class: 'eyebrow' }, h('a', { href: '#/' }, 'Percorso'), ' › ', h('a', { href: '#/turni' }, 'Conversazione con Gemini')),
        h('h1', { tabindex: '-1' }, `Turno ${n}`),
        h('p', { class: 'doc-meta' },
          info && info.date ? h('time', { datetime: info.date }, fmtDate(info.date)) : null,
          user.time ? `, ore ${user.time}` : null,
          total ? ` · ${n} di ${total}` : null),
        h('nav', { class: 'turn-nav', 'aria-label': 'Navigazione fra i turni' },
          h('a', { href: '#/', class: 'turn-back', onclick: goBack }, '← Indietro'),
          h('span', { class: 'turn-steps' },
            prev ? h('a', { href: turnHref(prev), onclick: replaceNav, title: `Turno ${prev}` }, '‹ precedente') : null,
            next ? h('a', { href: turnHref(next), onclick: replaceNav, title: `Turno ${next}` }, 'successivo ›') : null),
        ),
        review
          ? h('div', { class: `callout callout-source${entries.length ? '' : ' is-clean'}`, role: 'note' },
            h('p', null, h('strong', null, 'Testo originale di Gemini.'), ' ', reviewSummary(review), ' Per i concetti corretti valgono le note', notes, '.'))
          : h('div', { class: 'callout callout-source', role: 'note' },
            h('p', null, h('strong', null, 'Testo originale di Gemini, non rivisto:'), ' può contenere errori. Per i concetti corretti valgono le note', notes, '.')),
        entries.length ? correctionsBox(n, entries) : null,
        cited.length
          ? h('section', { class: 'cited', 'aria-labelledby': 'h-cited' },
            h('h2', { id: 'h-cited' }, 'Citato in'),
            h('ul', null, cited))
          : h('p', { class: 'muted small' }, 'Nessun concetto o errore delle note cita questo turno.'),
      ),
      h('div', { class: 'prose turn-body' },
        h('h2', { class: 'turn-role', id: 'domanda' }, 'Domanda di Stefano'),
        user.text ? turnQuestion(user.text) : h('p', { class: 'muted' }, '(domanda vuota)'),
        h('h2', { class: 'turn-role', id: 'risposta' }, 'Risposta di Gemini', gemini && gemini.time ? h('span', { class: 'muted small' }, ` · ${gemini.time}`) : null),
        answer,
      ),
      h('nav', { class: 'prev-next', 'aria-label': 'Turni vicini' },
        prev ? pnLink(prev, 'pn-prev', '← Turno precedente') : h('span'),
        next ? pnLink(next, 'pn-next', 'Turno successivo →') : h('span'),
      ),
    ),
  );
  // Gli highlight sono globali (CSS.highlights): si registrano solo quando la pagina
  // viene davvero mostrata (vedi render).
  view.onMount = () => corr.registerHighlights(entries.filter((x) => x.range && !x.marks.length).map((x) => ({ range: x.range, severity: x.item.severity })));
  return view;
}

function viewTurnIndex(filter) {
  const turns = site.turnIndex && Array.isArray(site.turnIndex.turns) ? site.turnIndex.turns : null;
  document.title = 'Conversazione con Gemini — Study';
  const head = h('header', { class: 'page-head' },
    h('p', { class: 'eyebrow' }, h('a', { href: '#/' }, 'Percorso'), ' › pagina secondaria'),
    h('h1', { tabindex: '-1' }, 'Conversazione con Gemini'),
    h('p', { class: 'lead' }, 'I turni originali, uno per pagina: la domanda e la risposta di Gemini così come sono state esportate. Dove il turno è stato rivisto, le correzioni sono mostrate a parte ed evidenziate nel testo; le note sono la versione rivista.'),
  );
  if (!turns) {
    return h('div', { class: 'page page-turns' }, head,
      h('div', { class: 'callout callout-error', role: 'alert' }, h('p', null, 'turns/index.json non si è caricato: rigenera i turni con ', h('code', null, 'python3 antirez/study/tools/split_turns.py'), '.')));
  }
  // Aree di ogni turno (area.turns), mostrate accanto alla data.
  const areasOf = new Map();
  for (const a of site.areas) for (const t of a.turns) areasOf.set(t, [...(areasOf.get(t) || []), a]);

  const reviewed = corr.correctionsLoaded();
  const items = turns.map((t) => {
    const areas = areasOf.get(t.n) || [];
    const r = corr.turnReview(t.n);
    const li = h('li', { class: 'turn-item' },
      h('a', { href: turnHref(t.n), class: 'turn-item-link' },
        h('span', { class: 'turn-n' }, `Turno ${t.n}`),
        h('span', { class: 'turn-date' }, t.date ? fmtDate(t.date) : ''),
        reviewed ? reviewStatus(t.n) : null,
        areas.length ? h('span', { class: 'turn-areas' }, areas.map((a) => a.title).join(' · ')) : null,
        h('span', { class: 'turn-preview' }, t.preview || ''),
      ),
    );
    return { li, corrected: Boolean(r && r.items.length), text: norm(`turno ${t.n} ${t.date || ''} ${fmtDate(t.date)} ${areas.map((a) => a.title).join(' ')} ${t.preview || ''}`) };
  });
  const list = h('ol', { class: 'turn-list' }, items.map((x) => x.li));
  const search = h('input', { type: 'search', id: 'f-turn', placeholder: 'es. refcount, pthread, 111, giu', autocomplete: 'off' });
  const count = h('p', { class: 'muted', role: 'status', 'aria-live': 'polite' });
  const nCorrected = items.filter((x) => x.corrected).length;
  const only = h('input', { type: 'checkbox', id: 'f-corr', checked: reviewed && filter === 'correzioni' });
  function update() {
    const q = norm(search.value);
    let shown = 0;
    for (const x of items) {
      const on = (!q || x.text.includes(q)) && (!only.checked || x.corrected);
      x.li.hidden = !on;
      if (on) shown++;
    }
    count.textContent = `${plural(shown, 'turno', 'turni')} su ${items.length}`;
    const target = only.checked ? '#/turni/correzioni' : '#/turni';
    if (location.hash !== target) {
      history.replaceState(history.state, '', target);
      current.key = routeKey(parseRoute());
    }
  }
  search.addEventListener('input', update);
  only.addEventListener('change', update);
  update();
  const nReviewed = corr.reviewedTurns().length;
  return h('div', { class: 'page page-turns' }, head,
    h('div', { class: 'filters' },
      h('div', { class: 'field' }, h('label', { for: 'f-turn' }, 'Cerca nelle domande'), search),
      reviewed ? h('div', { class: 'field field-check' }, h('label', { for: 'f-corr' }, only, ` Solo turni con correzioni (${nCorrected})`)) : null),
    reviewed ? h('p', { class: 'muted small' }, `Rivisti ${nReviewed} turni su ${items.length}: ${plural(nCorrected, 'turno', 'turni')} con correzioni.`) : null,
    count,
    list,
  );
}

function viewNotFound(msg) {
  document.title = 'Pagina non trovata — Study';
  return h('div', { class: 'page' },
    h('h1', { tabindex: '-1' }, 'Pagina non trovata'),
    h('p', null, msg || 'Questo indirizzo non corrisponde a nessuna pagina.'),
    h('p', null, h('a', { href: '#/' }, 'Torna al percorso')),
  );
}

// ---------------------------------------------------------------------------
// Router

function parseRoute() {
  const raw = decodeURIComponent(location.hash.replace(/^#\/?/, ''));
  const parts = raw.split('/').filter(Boolean);
  const [page, id, ...rest] = parts;
  if (!page) return { page: 'dashboard' };
  if (page === 'area' && id) return { page: 'area', id, anchor: rest.join('/') };
  if (page === 'project' && id) return { page: 'project', id };
  if (page === 'errori') return { page: 'errori', id };
  if (page === 'turni' && (!id || id === 'correzioni')) return { page: 'turni', id };
  if (page === 'turno' && id) return { page: 'turno', id, anchor: rest.join('/') };
  return { page: 'notfound' };
}
const routeKey = (r) => `${r.page}/${r.id || ''}`;

function scrollToAnchor(anchor) {
  if (!anchor) return false;
  const el = document.getElementById(anchor) || main.querySelector(`[data-concept="${CSS.escape(anchor)}"]`);
  if (!el) return false;
  el.scrollIntoView({ block: 'start' });
  const target = el.matches('h1,h2,h3,h4') ? el : el.querySelector('h3') || el;
  target.setAttribute('tabindex', '-1');
  target.focus({ preventScroll: true });
  return true;
}

function markNav(route) {
  for (const a of document.querySelectorAll('[data-nav]')) {
    const group = route.page === 'errori' ? 'errori' : 'dashboard';
    const on = a.dataset.nav === group && !['notfound', 'turni', 'turno'].includes(route.page);
    if (on) a.setAttribute('aria-current', 'page');
    else a.removeAttribute('aria-current');
  }
}

async function render() {
  const route = parseRoute();
  const key = routeKey(route);
  const savedScroll = trackHistory();
  markNav(route);
  if (key === current.key && (route.page === 'area' || route.page === 'turno')) {
    if (savedScroll !== undefined) window.scrollTo(0, savedScroll);
    else scrollToAnchor(route.anchor) || window.scrollTo(0, 0);
    return;
  }
  if (key === current.key && (route.page === 'errori' || route.page === 'turni')) return;
  current.key = key;
  const token = Symbol('render');
  current.token = token;
  main.setAttribute('aria-busy', 'true');
  let view;
  try {
    await ensureSite();
    if (route.page === 'dashboard') view = viewDashboard();
    else if (route.page === 'area') view = await viewArea(route.id);
    else if (route.page === 'project') view = await viewProject(route.id);
    else if (route.page === 'errori') view = viewErrors(route.id);
    else if (route.page === 'turni') view = viewTurnIndex(route.id);
    else if (route.page === 'turno') view = await viewTurn(route.id);
    else view = viewNotFound();
  } catch (e) {
    console.error(e);
    view = h('div', { class: 'page' }, errorBox(e));
    current.key = null;
  }
  if (current.token !== token) return; // nel frattempo è cambiata pagina
  main.replaceChildren(...[libsWarning(), view].filter(Boolean));
  corr.clearHighlights();
  if (view.onMount) view.onMount();
  main.removeAttribute('aria-busy');
  setupToc();
  if (savedScroll !== undefined) window.scrollTo(0, savedScroll);
  else if (!scrollToAnchor(route.anchor)) {
    window.scrollTo(0, 0);
    // Dopo una navigazione il focus va al titolo (utile con lettori di schermo).
    const h1 = main.querySelector('h1');
    if (h1 && !firstRender) h1.focus({ preventScroll: true });
  }
  firstRender = false;
}
let firstRender = true;

// Cronologia: ogni voce del sito ha la sua profondità in history.state (0 = prima pagina
// aperta), così "Indietro" nei turni sa se c'è una pagina del sito a cui tornare. Le
// posizioni di scorrimento delle pagine lasciate vengono ricordate: tornando indietro
// (bottone o tasto del browser) la nota riappare al punto in cui la si era lasciata.
let navDepth = 0;
let replacingNav = false; // location.replace (turno precedente/successivo)
let lastEntry = null; // profondità della pagina mostrata (chiave di scrollPos)
const scrollPos = new Map();
try {
  history.scrollRestoration = 'manual';
} catch {
  /* non supportato: pazienza */
}
function trackHistory() {
  if (lastEntry !== null) scrollPos.set(lastEntry, window.scrollY);
  const st = history.state;
  let traversal = false;
  if (st && Number.isInteger(st.depth)) {
    navDepth = st.depth;
    traversal = !firstRender;
  } else {
    if (!firstRender && !replacingNav) navDepth++;
    try {
      history.replaceState({ ...(st && typeof st === 'object' ? st : {}), depth: navDepth }, '');
    } catch {
      /* ignora */
    }
  }
  replacingNav = false;
  lastEntry = navDepth;
  return traversal ? scrollPos.get(lastEntry) : undefined;
}

function rerender() {
  current.key = null;
  render();
}

// Indice: aperto su desktop, chiuso (a scomparsa) su mobile.
const desktop = window.matchMedia('(min-width: 60rem)');
function setupToc() {
  const toc = document.getElementById('toc');
  if (!toc) return;
  // Desktop: colonna laterale sticky. Mobile: a scomparsa, subito sotto l'intestazione.
  const col = main.querySelector('.toc-col');
  const head = main.querySelector('.page-area .page-head');
  if (desktop.matches && col && toc.parentElement !== col) col.append(toc);
  else if (!desktop.matches && head && toc.previousElementSibling !== head) head.after(toc);
  toc.open = desktop.matches;
  if (!toc.dataset.bound) {
    toc.dataset.bound = '1';
    toc.addEventListener('click', (e) => {
      if (!desktop.matches && e.target.closest('a')) toc.open = false;
    });
  }
}
desktop.addEventListener('change', setupToc);

document.querySelector('.skip-link')?.addEventListener('click', (e) => {
  e.preventDefault();
  const h1 = main.querySelector('h1') || main;
  h1.setAttribute('tabindex', '-1');
  h1.focus();
});

window.addEventListener('hashchange', render);
render();
