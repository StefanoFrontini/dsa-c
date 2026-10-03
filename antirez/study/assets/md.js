// Rendering del markdown con `marked` (caricato da cdnjs come script classico),
// evidenziazione con `highlight.js`, id dei titoli compatibili con le ancore di GitHub
// e riscrittura dei link fra note/schede in rotte del sito.

import { REPO } from './areas.js';
import { githubUrl, linkCodeRefs } from './codelinks.js';
import { linkTurnRefs } from './turns.js';

const marked = window.marked;
const hljs = window.hljs;

if (marked) {
  // Le note sono scritte da te, ma l'HTML grezzo viene comunque mostrato come testo
  // (es. un `<ruby>` citato fuori dai backtick non deve sparire).
  marked.use({
    gfm: true,
    breaks: false,
    renderer: {
      html(token) {
        return escapeHtml(typeof token === 'string' ? token : token.text || token.raw || '');
      },
    },
  });
}
if (hljs) {
  hljs.configure({
    ignoreUnescapedHTML: true,
    // per i blocchi senza linguaggio l'auto-rilevamento sceglie solo fra questi
    languages: ['c', 'bash', 'typescript', 'javascript', 'python', 'makefile', 'json', 'plaintext'],
  });
}

export const libsReady = () => Boolean(marked);

// Slug in stile GitHub: minuscole, via la punteggiatura (tranne - e _), spazi -> '-'.
export function slugify(text) {
  return text
    .trim()
    .toLowerCase()
    .replace(/[^\p{L}\p{M}\p{N}\p{Pc}\- ]/gu, '')
    .replace(/ /g, '-');
}

function escapeHtml(s) {
  return s.replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[c]);
}

// Markdown -> frammento DOM.
export function renderMarkdown(src) {
  const div = document.createElement('div');
  if (!marked) {
    const pre = document.createElement('pre');
    pre.className = 'md-fallback';
    pre.textContent = src;
    div.append(pre);
    return div;
  }
  div.innerHTML = marked.parse(src);
  return div;
}

export function renderInline(src) {
  const span = document.createElement('span');
  if (!marked) {
    span.textContent = src;
    return span;
  }
  span.innerHTML = marked.parseInline(src);
  return span;
}

// Assegna id ai titoli (con suffisso -1, -2 per i duplicati, come GitHub).
export function addHeadingIds(root) {
  const seen = new Map();
  for (const h of root.querySelectorAll('h1, h2, h3, h4, h5, h6')) {
    const base = slugify(h.textContent);
    const n = seen.get(base) || 0;
    seen.set(base, n + 1);
    h.id = n ? `${base}-${n}` : base;
  }
}

// `docPath` è il percorso del documento dentro study/ (es. "notes/parsing.md"):
// serve a risolvere i link relativi.
export function rewriteLinks(root, docPath, known) {
  const base = new URL(`https://repo.invalid/${REPO.base}study/${docPath}`);
  for (const a of root.querySelectorAll('a[href]')) {
    const raw = a.getAttribute('href');
    if (/^[a-z][a-z0-9+.-]*:/i.test(raw)) {
      if (/^https?:/i.test(raw)) {
        a.target = '_blank';
        a.rel = 'noopener';
      }
      continue;
    }
    if (raw.startsWith('#/')) continue;
    let url;
    try {
      url = new URL(raw, base);
    } catch {
      continue;
    }
    const hash = url.hash ? decodeURIComponent(url.hash.slice(1)) : '';
    if (raw.startsWith('#')) {
      // ancora nella stessa pagina
      a.href = `#/${known.route}${hash ? '/' + hash : ''}`;
      continue;
    }
    const path = url.pathname.replace(/^\//, '');
    const m = new RegExp(`^${REPO.base}study/(notes|projects)/([\\w-]+)\\.md$`).exec(path);
    if (m) {
      const [, dir, id] = m;
      const route = dir === 'notes' ? `area/${id}` : `project/${id}`;
      a.href = `#/${route}${hash ? '/' + hash : ''}`;
      continue;
    }
    if (path.startsWith(REPO.base)) {
      const rel = path.slice(REPO.base.length);
      a.href = githubUrl(rel);
      a.target = '_blank';
      a.rel = 'noopener';
    }
  }
}

export function highlight(root) {
  if (!hljs) return;
  for (const code of root.querySelectorAll('pre code')) {
    try {
      hljs.highlightElement(code);
    } catch {
      /* il blocco resta in chiaro */
    }
  }
}

// Avvolge le tabelle in un contenitore scrollabile orizzontalmente.
export function wrapTables(root) {
  for (const t of root.querySelectorAll('table')) {
    if (t.parentElement.classList.contains('table-wrap')) continue;
    const w = document.createElement('div');
    w.className = 'table-wrap';
    w.tabIndex = 0;
    w.setAttribute('role', 'region');
    w.setAttribute('aria-label', 'Tabella (scorre orizzontalmente)');
    t.replaceWith(w);
    w.append(t);
  }
}

// Pipeline completa per una nota o una scheda. `turns: false` lascia come testo i
// riferimenti "turno N" (nei turni stessi, dove sono parole di Gemini).
export function renderDocument(src, { docPath, route, ctx, turns = true }) {
  const root = renderMarkdown(src);
  addHeadingIds(root);
  rewriteLinks(root, docPath, { route });
  linkCodeRefs(root, ctx);
  if (turns) linkTurnRefs(root);
  highlight(root);
  wrapTables(root);
  return root;
}
