// Tappa 9 — la colla tra lo stream e il browser (percorso: ../streams/README.md, "Tappa 9").
// animation.html carica prima dist/language-picture/script.js come script classico: i suoi nomi globali
// (painter, beside, below, right_split, frame1, ctx, CANVAS_WIDTH, CANVAS_HEIGHT, il tipo Painter...) qui si
// usano direttamente, senza import.
// Build: npx -y -p typescript@5.9 tsc -p tsconfig.anim.json   (da questa cartella)
// Server: python3 -m http.server 8000   →   http://localhost:8000/animation.html

import { take } from "../streams/src/stream.ts";
import { iterate, cycle, play } from "../streams/src/animation.ts";

// 1. Lo stream infinito dei livelli di right_split di `painter` (0, 1, 2, ...) costruito con `iterate`,
//    senza chiamare right_split: ogni livello si ottiene dal precedente.

// 2. Un'animazione che cresce fino al livello 5 e poi torna indietro, all'infinito: prendi i primi livelli
//    con `take` e falli girare con `cycle` (0 1 2 3 4 5 4 3 2 1 0 1 ...).

// 3. Mostrala con `play`, usando requestAnimationFrame come orologio: a ogni elemento pulisci il canvas
//    e disegna il painter in frame1. Prova periodi diversi (100, 500, 1000 ms).
