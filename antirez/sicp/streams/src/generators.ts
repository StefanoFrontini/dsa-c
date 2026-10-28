// Tappa 8 — generatori JS a confronto con gli stream di SICP.
// Le firme sono fissate (i test le importano), i corpi li scrivi tu.
// Percorso e domande: ../README.md, "Tappa 8". Test: `node --test test/08-generatori.test.ts`.

import {
  the_empty_stream,
  stream_head,
  stream_tail,
  cons_stream,
  type Stream,
} from "./stream.ts";

function todo(): never {
  throw new Error("TODO tappa 8: da implementare");
}

/** n, n+1, n+2, ... all'infinito (come integers_from, ma con function*). */
export function* integers_from_gen(n: number): Generator<number> {
  while (true) {
    yield n++;
  }
  // yield n;
  // yield* integers_from_gen(n + 1);
}

/** I primi n valori in un array (meno se il generatore finisce prima). Non chiedere più valori del necessario. */
export function take_gen<T>(g: Iterator<T>, n: number): T[] {
  // const g_iterable = {
  //   [Symbol.iterator]() {
  //     return {
  //       next() {
  //         return g.next();
  //       },
  //     };
  //   },
  // };
  const result: T[] = [];
  if (n === 0) return result;
  // for (const x of g_iterable) {
  //   result.push(x);
  //   n--;
  //   if (n === 0) break;
  // }
  while (n--) {
    const next = g.next();
    if (next.done) return result;
    result.push(next.value);
  }
  return result;
}
// console.log(take_gen(integers_from_gen(1), 5));

// export function stream_map<R>(
//   f: (...args: any[]) => R,
//   ...streams: Stream<any>[]
// ): Stream<R> {
//   return streams.some((s) => s === the_empty_stream)
//     ? the_empty_stream
//     : cons_stream(f(...streams.map((s) => stream_head(s))), () =>
//         stream_map(f, ...streams.map((s) => stream_tail(s))),
//       );
// }

/** Come stream_map su più stream: f riceve un valore da ciascun iteratore; finisce quando uno finisce. */
export function* map_gen<R>(
  f: (...args: any[]) => R,
  ...gens: Iterator<any>[]
): Generator<R> {
  while (true) {
    const next = gens.map((x) => x.next());
    if (next.some((s) => s.done === true)) return;
    yield f(...next.map((x) => x.value));
  }
}

export function* filter_gen<T>(
  pred: (x: T) => boolean,
  g: Iterator<T>,
): Generator<T> {
  while (true) {
    const next = g.next();
    if (next.done) return;
    if (pred(next.value)) {
      yield next.value;
    }
  }
}

/** Percorre uno stream con un generatore: forza le code una alla volta, solo quando viene chiesto il valore successivo. */
export function* stream_to_gen<T>(s: Stream<T>): Generator<T> {
  while (s !== the_empty_stream) {
    yield stream_head(s);
    s = stream_tail(s);
  }
  return;
}

/** Il ponte inverso: uno stream (memoizzato) che chiede a `g` ogni valore una volta sola. */
export function gen_to_stream<T>(g: Iterator<T>): Stream<T> {
  const next = g.next();
  return next.done
    ? the_empty_stream
    : cons_stream(next.value, () => gen_to_stream(g));
}

/** Crivello di Eratostene con i generatori: sieve_gen(integers_from_gen(2)) = 2, 3, 5, 7, ... */
export function* sieve_gen(g: Iterator<number>): Generator<number> {
  todo();
}
