// Configurazione del sito. L'unica cosa hardcoded è l'ordine del percorso di studio:
// tutto il resto (titoli, periodi, concetti, progetti) viene letto a runtime da
// data/<area>.json, notes/<area>.md e projects/<id>.md.

export const AREAS = [
  'memoria-c',
  'parsing',
  'algebra-fp',
  'pratt-alberi',
  'concorrenza',
  'networking',
  'audio',
  'actor-model',
  'sicp-combinatori',
];

// Dove puntano i link al codice. I percorsi nelle note sono relativi ad `antirez/`.
export const REPO = {
  url: 'https://github.com/StefanoFrontini/dsa-c',
  branch: 'master',
  base: 'antirez/',
};

// Ordine e etichette degli stati dei progetti (dal più urgente al più stabile).
export const STATUSES = [
  { id: 'broken', label: 'broken' },
  { id: 'stub', label: 'stub' },
  { id: 'in-progress', label: 'in corso' },
  { id: 'working', label: 'working' },
  { id: 'done', label: 'done' },
];

// Dopo quanti giorni una domanda segnata "so" torna fra quelle da ripassare.
export const REVIEW_AFTER_DAYS = 14;
// Quante domande mostrare in "Da ripassare oggi" prima di "mostra altre".
export const REVIEW_LIMIT = 8;
