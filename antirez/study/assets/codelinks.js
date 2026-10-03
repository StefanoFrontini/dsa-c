// Trasforma i riferimenti al codice (`chord-pratt/chord-pratt.c:606-607`, `tac.c:44`,
// `networking/Makefile`) in link al file su GitHub, alla riga giusta.
//
// Regole:
// - i percorsi sono relativi ad `antirez/`; un prefisso `antirez/` non viene duplicato;
// - `:86` senza file resta testo;
// - con l'elenco dei file del repo (assets/repo-files.json) un nome senza cartella
//   (`tac.c:44`) o un percorso parziale (`actor/poll_input.c`) viene risolto se il
//   file è uno solo; i file che non esistono (segmenti .ts di esempio, cartella di
//   build) restano testo;
// - `notes/<area>.md` e `projects/<id>.md` diventano link interni al sito.

import { REPO } from './areas.js';

const EXT = 'c|h|ts|tsx|js|mjs|py|sh|txt|html|css|json|md|s|csv|toml|ya?ml|jpe?g|png|pdf';
// path[:riga[-riga][,altre righe]]
const REF_RE = new RegExp(
  String.raw`^((?:[\w.\-]+/)*(?:[\w.\-]+\.(?:${EXT})|Makefile|CMakeLists\.txt))` +
    String.raw`(?::(\d+)(?:-(\d+))?(?:,\s*\d+(?:-\d+)?)*)?$`,
);
const DIR_RE = /^((?:[\w.\-]+\/)+)$/;

let files = null; // Set dei file, se disponibile
let dirs = null; // Set delle cartelle
let byBase = new Map(); // basename -> Set di percorsi completi
let internal = { areas: new Set(), projects: new Set() };

export function configure({ repoFiles, areaIds = [], projectIds = [], codeRefs = [] }) {
  internal = { areas: new Set(areaIds), projects: new Set(projectIds) };
  byBase = new Map();
  if (repoFiles) {
    files = new Set(repoFiles);
    dirs = new Set();
    for (const f of repoFiles) {
      const parts = f.split('/');
      for (let i = 1; i < parts.length; i++) dirs.add(parts.slice(0, i).join('/') + '/');
    }
  } else {
    files = dirs = null;
    // Senza elenco dei file, l'indice dei nomi si costruisce dai code_refs dei JSON.
    for (const r of codeRefs) {
      const m = REF_RE.exec(r.trim());
      if (!m) continue;
      const p = stripBase(m[1]);
      if (!p.includes('/')) continue;
      const base = p.split('/').pop();
      if (!byBase.has(base)) byBase.set(base, new Set());
      byBase.get(base).add(p);
    }
  }
}

const stripBase = (p) => (p.startsWith(REPO.base) ? p.slice(REPO.base.length) : p.replace(/^\.\//, ''));

export function githubUrl(path, a, b) {
  const isDir = path.endsWith('/');
  const kind = isDir ? 'tree' : 'blob';
  let url = `${REPO.url}/${kind}/${REPO.branch}/${REPO.base}${path.replace(/\/$/, '')}`;
  if (a && !isDir) {
    // GitHub mostra i numeri di riga dei .md solo nella vista "plain".
    if (/\.md$/i.test(path)) url += '?plain=1';
    url += `#L${a}` + (b && b !== a ? `-L${b}` : '');
  }
  return url;
}

// Sceglie il file fra più candidati usando il contesto della pagina
// (cartelle dei progetti dell'area, code_refs già noti).
function pick(cands, ctx) {
  if (cands.length === 1) return cands[0];
  if (!ctx) return null;
  const known = cands.filter((c) => ctx.refs && ctx.refs.has(c));
  if (known.length === 1) return known[0];
  const inDir = cands.filter((c) => (ctx.dirs || []).some((d) => c.startsWith(d)));
  if (inDir.length === 1) return inDir[0];
  return null;
}

function resolvePath(p, ctx) {
  if (files) {
    if (files.has(p)) return p;
    const suffix = '/' + p;
    const cands = [...files].filter((f) => f.endsWith(suffix));
    return pick(cands, ctx);
  }
  if (p.includes('/')) return p;
  return pick([...(byBase.get(p) || [])], ctx);
}

// Restituisce { href, internal } oppure null se il testo non è un riferimento.
export function resolveRef(text, ctx) {
  const t = text.trim();

  const dm = DIR_RE.exec(t);
  if (dm) {
    const p = stripBase(dm[1]);
    if (dirs && !dirs.has(p)) return null;
    return { href: githubUrl(p), internal: false };
  }

  const m = REF_RE.exec(t);
  if (!m) return null;
  let p = stripBase(m[1]);
  const a = m[2];
  const b = m[3];

  // Note e schede del sito stesso.
  const im = /^(?:study\/)?(?:(notes|projects)\/)?([\w-]+)\.md$/.exec(p);
  if (im) {
    const [, dir, id] = im;
    if ((dir === 'notes' || !dir) && internal.areas.has(id)) return { href: `#/area/${id}`, internal: true };
    if ((dir === 'projects' || !dir) && internal.projects.has(id)) return { href: `#/project/${id}`, internal: true };
  }

  const resolved = resolvePath(p, ctx);
  if (!resolved) return null;
  return { href: githubUrl(resolved, a, b), internal: false };
}

// Applica i link a tutti i <code> fuori dai blocchi <pre> e fuori dai link esistenti.
export function linkCodeRefs(root, ctx) {
  for (const code of root.querySelectorAll('code')) {
    if (code.closest('pre, a')) continue;
    const r = resolveRef(code.textContent, ctx);
    if (!r) continue;
    const a = document.createElement('a');
    a.href = r.href;
    a.className = 'code-link';
    if (!r.internal) {
      a.target = '_blank';
      a.rel = 'noopener';
      a.title = 'Apri su GitHub';
    }
    code.replaceWith(a);
    a.append(code);
  }
}

// Crea un elemento per un code_ref del JSON: link se risolvibile, altrimenti <code>.
export function codeRefElement(ref, ctx) {
  const code = document.createElement('code');
  code.textContent = ref;
  const r = resolveRef(ref, ctx);
  if (!r) return code;
  const a = document.createElement('a');
  a.href = r.href;
  a.className = 'code-link';
  if (!r.internal) {
    a.target = '_blank';
    a.rel = 'noopener';
    a.title = 'Apri su GitHub';
  }
  a.append(code);
  return a;
}
