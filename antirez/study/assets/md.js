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

// --- formule LaTeX ($...$ e $$...$$) con KaTeX ---
// Le formule vanno tolte dal sorgente PRIMA di marked (che altrimenti trasforma `_` in
// corsivo e mangia i `\`), sostituite da un segnaposto e rimesse come HTML di KaTeX dopo.
// Il codice (blocchi ``` e `inline`) non viene toccato: lì `$` è quasi sempre shell.
// Regole per `$...$` come Pandoc: niente spazio subito dopo il `$` di apertura né subito
// prima di quello di chiusura, che non deve essere seguito da una cifra ("da $5 a $10" resta testo).
const MATH_INLINE = /\$(?!\s)((?:\\\$|[^$\n])+?)(?<!\s)\$(?!\d)/g;
const MATH_DISPLAY = /\$\$([\s\S]+?)\$\$/g;
const CODE_SPAN = /(`+)[\s\S]*?\1/g;

function protectMath(src) {
  const math = [];
  const stash = (tex, display) => `\uE000M${math.push({ tex, display }) - 1}\uE000`;
  const outsideCode = (text) =>
    text.replace(CODE_SPAN, (m) => `\u0001${m}\u0001`).split('\u0001').map((part, i) =>
      i % 2 ? part : part.replace(MATH_DISPLAY, (_, t) => stash(t, true)).replace(MATH_INLINE, (_, t) => stash(t, false)),
    ).join('');
  // le formule display possono andare a capo: lavoro a blocchi separati dai fence
  const blocks = [];
  let buf = [];
  let inFence = false;
  for (const line of src.split('\n')) {
    const fence = /^\s*(```|~~~)/.test(line);
    if (fence && !inFence) { blocks.push({ code: false, text: buf.join('\n') }); buf = [line]; inFence = true; continue; }
    if (fence && inFence) { buf.push(line); blocks.push({ code: true, text: buf.join('\n') }); buf = []; inFence = false; continue; }
    buf.push(line);
  }
  blocks.push({ code: inFence, text: buf.join('\n') });
  const text = blocks.map((b) => (b.code ? b.text : outsideCode(b.text))).join('\n');
  return { text, math };
}

function restoreMath(html, math) {
  if (!math.length) return html;
  const katex = window.katex;
  return html.replace(/\uE000M(\d+)\uE000/g, (_, i) => {
    const { tex, display } = math[Number(i)];
    if (!katex) return escapeHtml(display ? `$$${tex}$$` : `$${tex}$`);
    return katex.renderToString(tex, { displayMode: display, throwOnError: false, output: 'html' });
  });
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
  const { text, math } = protectMath(src);
  div.innerHTML = restoreMath(marked.parse(text), math);
  return div;
}

export function renderInline(src) {
  const span = document.createElement('span');
  if (!marked) {
    span.textContent = src;
    return span;
  }
  const { text, math } = protectMath(src);
  span.innerHTML = restoreMath(marked.parseInline(text), math);
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
