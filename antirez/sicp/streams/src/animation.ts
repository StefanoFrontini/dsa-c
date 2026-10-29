// Tappa 9 — animare il picture language con uno stream.
// Qui solo la parte pura, senza DOM: funziona con qualunque tipo di elemento (painter, stringhe, numeri).
// La colla con il browser è in ../../language-picture/animation.ts.
// Percorso e domande: ../README.md, "Tappa 9". Test: `node --test test/09-animazione.test.ts`.

import { cons_stream, the_empty_stream, type Stream } from "./stream.ts";

function todo(): never {
  throw new Error("TODO tappa 9: da implementare");
}

/** x, f(x), f(f(x)), ... all'infinito. Ogni elemento si calcola dal precedente, una volta sola. */
export function iterate<T>(f: (x: T) => T, x: T): Stream<T> {
  return cons_stream(x, () => iterate(f, f(x)));
}

/** xs[0], xs[1], ..., xs[k-1], xs[0], xs[1], ... all'infinito, con k celle in tutto (vuoto se xs è vuoto). */
export function cycle<T>(xs: readonly T[]): Stream<T> {
  let stream: Stream<T> = the_empty_stream;
  function iter(i: number): Stream<T> {
    return i >= xs.length ? stream : cons_stream(xs[i], () => iter(i + 1));
  }
  stream = iter(0);
  return stream;
}

/** Ha la stessa forma di `requestAnimationFrame`: chiede di chiamare `callback(time)` al prossimo frame. */
export type Schedule = (callback: (time: number) => void) => unknown;

/**
 * Mostra lo stream nel tempo: un elemento ogni `period` millisecondi.
 * - Chiede il primo frame a `schedule` subito (senza disegnare); al primo frame disegna la testa e prende
 *   quel `time` come istante 0.
 * - A ogni frame successivo calcola quale elemento tocca adesso: se è cambiato lo disegna (una volta sola),
 *   saltando gli elementi rimasti indietro se il browser è stato lento; poi chiede il frame successivo.
 * - Quando lo stream finisce smette di chiedere frame. Uno stream vuoto non disegna nulla.
 * Restituisce `stop`: dopo averla chiamata, non disegna più e non chiede altri frame.
 */
export function play<T>(
  s: Stream<T>,
  draw: (x: T) => void,
  schedule: Schedule,
  period: number,
): () => void {
  return todo();
}
