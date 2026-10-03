// Correzioni puntuali alle risposte di Gemini (turns/corrections.json, facoltativo).
//
// Formato: { generated: "AAAA-MM-GG", turns: { "<n>": { verdict, date?, items?: [
//   { quote, severity: "errore" | "imprecisione", claim, correction, evidence?, concept? } ] } } }
//
// - `configureCorrections(json)` riceve il file (o null se manca: ogni turno resta "non rivisto");
// - `turnReview(n)` restituisce la revisione del turno normalizzata, o null;
// - `locateQuote(root, quote)` cerca la citazione nel testo renderizzato della risposta e
//   restituisce un Range (o null): spazi, maiuscole e spazi a larghezza zero non contano, e
//   la citazione può attraversare <strong>, <code>, link dentro lo stesso blocco;
// - `highlightRange` / `registerHighlights` evidenziano con la CSS Custom Highlight API,
//   oppure, se manca, avvolgendo i nodi di testo in <mark>.

let byN = new Map();
let loaded = false;
let generated = '';

const str = (x) => (x == null ? '' : String(x).trim());

export function configureCorrections(data) {
  byN = new Map();
  loaded = false;
  generated = '';
  if (!data || typeof data !== 'object' || !data.turns || typeof data.turns !== 'object') return;
  loaded = true;
  generated = str(data.generated);
  for (const [k, v] of Object.entries(data.turns)) {
    const n = Number(k);
    if (!Number.isInteger(n) || !v || typeof v !== 'object') continue;
    const items = (Array.isArray(v.items) ? v.items : [])
      .filter((o) => o && typeof o === 'object' && (o.claim || o.correction))
      .map((o) => ({
        quote: str(o.quote),
        severity: o.severity === 'errore' ? 'errore' : 'imprecisione',
        claim: str(o.claim),
        correction: str(o.correction),
        evidence: str(o.evidence),
        concept: str(o.concept),
      }));
    byN.set(n, { n, verdict: str(v.verdict), date: str(v.date || v.reviewed) || generated, items });
  }
}

export const correctionsLoaded = () => loaded;
export const turnReview = (n) => byN.get(n) || null;
export const reviewedTurns = () => [...byN.values()];

export function reviewCounts(r) {
  const errors = r ? r.items.filter((i) => i.severity === 'errore').length : 0;
  const total = r ? r.items.length : 0;
  return { total, errors, imprecisions: total - errors };
}

// ---------------------------------------------------------------------------
// Ricerca della citazione nel testo renderizzato

const ZERO_WIDTH = /[​-‍⁠﻿­]/;
const FOLD = { '“': '"', '”': '"', '„': '"', '«': '"', '»': '"', '″': '"', '‘': "'", '’': "'", '′': "'", '–': '-', '—': '-', '‑': '-', '…': '...', ' ': ' ' };
const LETTER_OR_DIGIT = /[\p{L}\p{N}]/u;

// Normalizza `s` e tiene, per ogni carattere del risultato, la posizione [inizio, fine)
// nella stringa originale. `loose`: solo lettere e cifre (secondo tentativo, per citazioni
// che differiscono nella punteggiatura o che attraversano due blocchi).
function normalize(s, loose) {
  let out = '';
  const from = [];
  const to = [];
  let i = 0;
  for (const ch of s) {
    const start = i;
    i += ch.length;
    if (ZERO_WIDTH.test(ch)) continue;
    const c = FOLD[ch] || ch;
    if (/^\s+$/.test(c)) {
      if (!loose && out && !out.endsWith(' ')) (out += ' ', from.push(start), to.push(i));
      continue;
    }
    for (const lc of c.toLowerCase()) {
      if (loose && !LETTER_OR_DIGIT.test(lc)) continue;
      out += lc;
      from.push(start);
      to.push(i);
    }
  }
  return { s: out, from, to };
}

// Testo di un elemento come concatenazione dei suoi nodi di testo (esclusi i marcatori
// delle correzioni già inseriti), con l'offset di partenza di ogni nodo.
function textOf(el) {
  const nodes = [];
  let text = '';
  const walker = document.createTreeWalker(el, NodeFilter.SHOW_TEXT, {
    acceptNode: (n) => (n.parentElement && n.parentElement.closest('.corr-marker') ? NodeFilter.FILTER_REJECT : NodeFilter.FILTER_ACCEPT),
  });
  while (walker.nextNode()) {
    nodes.push({ node: walker.currentNode, start: text.length });
    text += walker.currentNode.nodeValue;
  }
  return { text, nodes };
}

function pointAt(nodes, offset, preferEnd) {
  for (let k = 0; k < nodes.length; k++) {
    const { node, start } = nodes[k];
    const end = start + node.nodeValue.length;
    if (offset < end || (preferEnd && offset === end) || k === nodes.length - 1) {
      return { node, offset: Math.max(0, Math.min(offset - start, node.nodeValue.length)) };
    }
  }
  return null;
}

const BLOCKS = 'p, li, td, th, pre, blockquote, h2, h3, h4, h5, h6, dt, dd';

export function locateQuote(root, quote) {
  if (!root || !quote) return null;
  for (const loose of [false, true]) {
    const q = normalize(quote, loose).s.trim();
    if (q.length < (loose ? 8 : 4)) continue; // troppo corta per essere affidabile
    // Il blocco più piccolo che contiene la citazione; altrimenti tutta la risposta
    // (citazione a cavallo di due blocchi).
    let best = null;
    for (const el of [...root.querySelectorAll(BLOCKS), root]) {
      const t = textOf(el);
      const norm = normalize(t.text, loose);
      if (best && norm.s.length >= best.norm.s.length) continue;
      const at = norm.s.indexOf(q);
      if (at >= 0) best = { t, norm, at };
    }
    if (!best) continue;
    const { t, norm, at } = best;
    const a = pointAt(t.nodes, norm.from[at], false);
    const b = pointAt(t.nodes, norm.to[at + q.length - 1], true);
    if (!a || !b) continue;
    const range = document.createRange();
    range.setStart(a.node, a.offset);
    range.setEnd(b.node, b.offset);
    return range;
  }
  return null;
}

// ---------------------------------------------------------------------------
// Evidenziazione

export const hasHighlightApi = () => typeof CSS !== 'undefined' && 'highlights' in CSS && typeof window.Highlight === 'function';
const NAMES = ['corr-errore', 'corr-imprecisione', 'corr-active'];

export function clearHighlights() {
  if (!hasHighlightApi()) return;
  for (const name of NAMES) CSS.highlights.delete(name);
}

// Fallback senza Highlight API: avvolge in <mark> i pezzi di testo del range. Restituisce
// i <mark> creati (e un nuovo range che li copre).
export function wrapRange(range, severity) {
  const parts = [];
  const add = (n) => {
    const s = n === range.startContainer ? range.startOffset : 0;
    const e = n === range.endContainer ? range.endOffset : n.nodeValue.length;
    if (e > s && n.nodeValue.slice(s, e).trim()) parts.push({ n, s, e });
  };
  const root = range.commonAncestorContainer;
  if (root.nodeType === Node.TEXT_NODE) add(root);
  else {
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
    while (walker.nextNode()) if (range.intersectsNode(walker.currentNode)) add(walker.currentNode);
  }
  const marks = [];
  for (const { n, s, e } of parts.reverse()) {
    if (e < n.nodeValue.length) n.splitText(e);
    const target = s > 0 ? n.splitText(s) : n;
    const mark = document.createElement('mark');
    mark.className = `corr-mark corr-mark-${severity}`;
    target.replaceWith(mark);
    mark.append(target);
    marks.unshift(mark);
  }
  if (!marks.length) return { marks, range };
  const r = document.createRange();
  r.setStartBefore(marks[0]);
  r.setEndAfter(marks[marks.length - 1]);
  return { marks, range: r };
}

// Registra gli highlight della pagina (da chiamare quando la pagina è nel documento).
export function registerHighlights(entries) {
  clearHighlights();
  if (!hasHighlightApi()) return;
  for (const sev of ['errore', 'imprecisione']) {
    const ranges = entries.filter((x) => x.severity === sev).map((x) => x.range);
    if (ranges.length) CSS.highlights.set(`corr-${sev}`, new Highlight(...ranges));
  }
}

export function setActiveHighlight(range) {
  if (!hasHighlightApi()) return;
  if (!range) return CSS.highlights.delete('corr-active');
  const hl = new Highlight(range);
  hl.priority = 1;
  CSS.highlights.set('corr-active', hl);
}
