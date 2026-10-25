// Tappa 8 — generatori JS a confronto con gli stream di SICP.
// Le firme sono fissate (i test le importano), i corpi li scrivi tu.
// Percorso e domande: ../README.md, "Tappa 8". Test: `node --test test/08-generatori.test.ts`.

import type { Stream } from "./stream.ts";

function todo(): never {
  throw new Error("TODO tappa 8: da implementare");
}

/** n, n+1, n+2, ... all'infinito (come integers_from, ma con function*). */
export function* integers_from_gen(n: number): Generator<number> {
  todo();
}

/** I primi n valori in un array (meno se il generatore finisce prima). Non chiedere più valori del necessario. */
export function take_gen<T>(g: Iterator<T>, n: number): T[] {
  return todo();
}

/** Come stream_map su più stream: f riceve un valore da ciascun iteratore; finisce quando uno finisce. */
export function* map_gen<R>(
  f: (...args: any[]) => R,
  ...gens: Iterator<any>[]
): Generator<R> {
  todo();
}

export function* filter_gen<T>(pred: (x: T) => boolean, g: Iterator<T>): Generator<T> {
  todo();
}

/** Percorre uno stream con un generatore: forza le code una alla volta, solo quando viene chiesto il valore successivo. */
export function* stream_to_gen<T>(s: Stream<T>): Generator<T> {
  todo();
}

/** Il ponte inverso: uno stream (memoizzato) che chiede a `g` ogni valore una volta sola. */
export function gen_to_stream<T>(g: Iterator<T>): Stream<T> {
  return todo();
}

/** Crivello di Eratostene con i generatori: sieve_gen(integers_from_gen(2)) = 2, 3, 5, 7, ... */
export function* sieve_gen(g: Iterator<number>): Generator<number> {
  todo();
}
