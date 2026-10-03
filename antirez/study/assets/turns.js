// Turni della conversazione con Gemini (turns/NNN.md + turns/index.json).
//
// - `configureTurns(index)` riceve turns/index.json (o null se non si carica);
// - `linkTurnRefs(root)` trasforma in link i riferimenti "turno 33", "turni 44-45",
//   "Turni: 1-2, 24-26, 31", "turni 128 e 134", "Turni 39 vs 40" nei nodi di testo
//   (fuori da <code>, <pre>, <a>); un numero senza la parola turno/turni resta testo;
// - `turnLink(n)` crea il link per un numero di turno dei JSON.

const MONTHS = ['gen', 'feb', 'mar', 'apr', 'mag', 'giu', 'lug', 'ago', 'set', 'ott', 'nov', 'dic'];

let byN = new Map(); // n -> { n, date, preview }
let max = 0; // 0 = indice non disponibile: si collega qualunque n >= 1

export function configureTurns(index) {
  const list = index && Array.isArray(index.turns) ? index.turns : [];
  byN = new Map(list.filter((t) => Number.isInteger(t.n)).map((t) => [t.n, t]));
  max = byN.size ? Math.max(...byN.keys()) : 0;
}

export const turnCount = () => max;
export const turnInfo = (n) => byN.get(n) || null;
export const turnHref = (n) => `#/turno/${n}`;
export const validTurn = (n) => Number.isInteger(n) && n >= 1 && (!max || n <= max);

export function fmtTurnDate(iso) {
  const m = /^(\d{4})-(\d{2})-(\d{2})/.exec(iso || '');
  return m ? `${Number(m[3])} ${MONTHS[Number(m[2]) - 1]} ${m[1]}` : '';
}

// Tooltip: "Turno 24 · 14 mar 2026 — inizio della domanda…"
export function turnTitle(n) {
  const t = byN.get(n);
  if (!t) return `Turno ${n}`;
  const preview = t.preview && t.preview.length > 110 ? t.preview.slice(0, 110).trimEnd() + '…' : t.preview || '';
  return `Turno ${n}` + (t.date ? ` · ${fmtTurnDate(t.date)}` : '') + (preview ? ` — ${preview}` : '');
}

export function turnLink(n, text = String(n)) {
  if (!validTurn(n)) return document.createTextNode(text);
  const a = document.createElement('a');
  a.href = turnHref(n);
  a.className = 'turn-link';
  a.title = turnTitle(n);
  a.textContent = text;
  return a;
}

// "turno"/"turni" + elenco di numeri e intervalli, separati da virgola, " e ", " vs ";
// fra un elemento e la virgola può stare una parentesi breve ("280 (codice completo), 270").
const ITEM = String.raw`\d+(?:\s*[-–]\s*\d+)?`;
const SEP = String.raw`(?:\s*\([^()]{1,40}\))?(?:,\s*|\s+e\s+|\s+vs\.?\s+)`;
const TURN_RE = new RegExp(String.raw`(?<![\p{L}\p{N}_])turn[oi](?:\s*:\s*|\s+)${ITEM}(?:${SEP}${ITEM})*(?![\p{L}\p{N}_])`, 'giu');
const NUM_RE = /\d+/g;

// Restituisce il numero di link creati.
export function linkTurnRefs(root) {
  const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, {
    acceptNode(node) {
      if (!/turn[oi]/i.test(node.nodeValue)) return NodeFilter.FILTER_REJECT;
      const p = node.parentElement;
      if (p && p.closest('code, pre, a, script, style')) return NodeFilter.FILTER_REJECT;
      return NodeFilter.FILTER_ACCEPT;
    },
  });
  const nodes = [];
  while (walker.nextNode()) nodes.push(walker.currentNode);

  let count = 0;
  for (const node of nodes) {
    const s = node.nodeValue;
    const frag = document.createDocumentFragment();
    let last = 0;
    let changed = false;
    for (const m of s.matchAll(TURN_RE)) {
      // Dentro il riferimento: la parola resta testo, ogni numero (anche gli estremi di
      // un intervallo) diventa un link; le parentesi intermedie restano testo.
      const ref = m[0];
      const words = ref.match(/^turn[oi](?:\s*:\s*|\s+)/i)[0];
      const body = ref.slice(words.length);
      const parts = [];
      let pos = 0;
      let depth = 0;
      for (const nm of body.matchAll(NUM_RE)) {
        // numeri dentro una parentesi ("(200k e 600k)") non sono turni
        const between = body.slice(pos, nm.index);
        depth += (between.match(/\(/g) || []).length - (between.match(/\)/g) || []).length;
        parts.push(between);
        const n = Number(nm[0]);
        if (depth === 0 && validTurn(n)) {
          parts.push(turnLink(n, nm[0]));
          count++;
        } else parts.push(nm[0]);
        pos = nm.index + nm[0].length;
      }
      parts.push(body.slice(pos));
      frag.append(s.slice(last, m.index), words, ...parts);
      last = m.index + ref.length;
      changed = true;
    }
    if (!changed) continue;
    frag.append(s.slice(last));
    node.replaceWith(frag);
  }
  return count;
}
