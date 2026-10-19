// SICP JS 3.5 — Stream in TypeScript.
// Le firme sono fissate (i test le importano), i corpi li scrivi tu, una tappa alla volta.
// Percorso e suggerimenti: ../README.md. Test: `npm test` oppure `node --test test/01-basi.test.ts`.

/** Uno stream è vuoto (null) oppure una testa già calcolata e una coda *ritardata*. */
export type Stream<T> = null | {
  readonly head: T;
  readonly tail: () => Stream<T>;
};

// type Pair = {
//   head: number | Pair | null;
//   tail: number | Pair | null;
// };

function todo(tappa: number): never {
  throw new Error(`TODO tappa ${tappa}: da implementare`);
}

// ---------------------------------------------------------------------------
// Tappa 1 — basi (SICP JS 3.5.1)
// ---------------------------------------------------------------------------

export const the_empty_stream = null;

/** Costruisce uno stream. `tail` è un thunk: NON va chiamato qui. (Tappa 2: memoizzalo.) */
export function cons_stream<T>(head: T, tail: () => Stream<T>): Stream<T> {
  return {
    head,
    tail: memo(tail),
  };
}

export function stream_head<T>(s: Stream<T>): T {
  if (s === the_empty_stream) throw new Error("Stream is empty");
  return s.head;
}

/** Forza la coda: è qui che il calcolo ritardato viene eseguito. */
export function stream_tail<T>(s: Stream<T>): Stream<T> {
  if (s === the_empty_stream) throw new Error("Stream is empty");
  return s.tail();
}

/** L'elemento di indice n (0 = la testa). */
export function stream_ref<T>(s: Stream<T>, n: number): T {
  return n === 0 ? stream_head(s) : stream_ref(stream_tail(s), n - 1);
}

/** I primi n elementi in un array (meno se lo stream finisce prima). Non forzare più del necessario. */
export function take<T>(s: Stream<T>, n: number): T[] {
  function iter(stream: Stream<T>, count: number, acc: T[]): T[] {
    return count === 0 || stream === the_empty_stream
      ? acc
      : iter(
          count === 1 ? the_empty_stream : stream_tail(stream),
          count - 1,
          (acc.push(stream_head(stream)), acc),
        );
  }
  return iter(s, n, []);
}

export function array_to_stream<T>(xs: readonly T[]): Stream<T> {
  function iter(i: number): Stream<T> {
    return i >= xs.length
      ? the_empty_stream
      : cons_stream(xs[i], () => iter(i + 1));
  }
  return iter(0);
}

/** low, low+1, ..., high (vuoto se low > high). */
export function stream_enumerate_interval(
  low: number,
  high: number,
): Stream<number> {
  return low > high
    ? the_empty_stream
    : cons_stream(low, () => stream_enumerate_interval(low + 1, high));
}

/** n, n+1, n+2, ... all'infinito. */
export function integers_from(n: number): Stream<number> {
  return cons_stream(n, () => integers_from(n + 1));
}

// console.log(array_to_stream(take(integers_from(5), 3)));
// console.log(take(array_to_stream(["x", "y", "z"]), 10));

// ---------------------------------------------------------------------------
// Tappa 2 — memoizzazione (SICP JS 3.5.1, "memo")
// ---------------------------------------------------------------------------

/** Restituisce un thunk che chiama `f` solo la prima volta e poi restituisce sempre lo stesso valore. */
export function memo<T>(f: () => T): () => T {
  let already_run = false;
  let result: T;
  return () => {
    if (!already_run) {
      result = f();
      already_run = true;
    }
    return result;
  };
}

// ---------------------------------------------------------------------------
// Tappa 3 — operazioni (SICP JS 3.5.1, es. 3.50)
// ---------------------------------------------------------------------------

/** Come `map`, ma su uno o più stream: f riceve un elemento da ciascuno. */
export function stream_map2<A, B, R>(
  f: (arg1: A, arg2: B) => R,
  s1: Stream<A>,
  s2: Stream<B>,
): Stream<R> {
  if (s1 === the_empty_stream || s2 === the_empty_stream)
    return the_empty_stream;
  return cons_stream(f(stream_head(s1), stream_head(s2)), () =>
    stream_map2(f, stream_tail(s1), stream_tail(s2)),
  );
}
export function stream_map<R>(
  f: (...args: any[]) => R,
  ...streams: Stream<any>[]
): Stream<R> {
  return streams.some((s) => s === the_empty_stream)
    ? the_empty_stream
    : cons_stream(f(...streams.map((s) => stream_head(s))), () =>
        stream_map(f, ...streams.map((s) => stream_tail(s))),
      );
}
export function stream_filter<T>(
  pred: (x: T) => boolean,
  s: Stream<T>,
): Stream<T> {
  while (!(s === the_empty_stream || pred(stream_head(s)))) {
    s = stream_tail(s);
  }
  return s === the_empty_stream
    ? the_empty_stream
    : cons_stream(stream_head(s), () => stream_filter(pred, stream_tail(s)));
}

export function stream_filter2<T>(
  pred: (x: T) => boolean,
  s: Stream<T>,
): Stream<T> {
  return s === the_empty_stream
    ? the_empty_stream
    : pred(stream_head(s))
      ? cons_stream(stream_head(s), () => stream_filter2(pred, stream_tail(s)))
      : stream_filter(pred, stream_tail(s));
}

export function add_streams(
  a: Stream<number>,
  b: Stream<number>,
): Stream<number> {
  return stream_map((x: number, y: number) => x + y, a, b);
}

export function mul_streams(
  a: Stream<number>,
  b: Stream<number>,
): Stream<number> {
  return stream_map((x: number, y: number) => x * y, a, b);
}

export function scale_stream(
  s: Stream<number>,
  factor: number,
): Stream<number> {
  return stream_map((x: number) => x * factor, s);
}

// ---------------------------------------------------------------------------
// Tappa 4 — stream infiniti (SICP JS 3.5.2)
// Le costanti valgono null finché non le scrivi. Definiscile con cons_stream e,
// dove indicato, *implicitamente*: lo stream si riferisce a se stesso nella coda.
// ---------------------------------------------------------------------------

/** 1, 1, 1, ... (implicito) */
export const ones: Stream<number> = cons_stream(1, () => ones);

/** 1, 2, 3, ... (implicito: con ones e add_streams, senza integers_from) */
export const integers: Stream<number> = cons_stream(1, () =>
  add_streams(ones, integers),
);

// /** n, n+1, n+2, ... all'infinito. */
// export function integers_from(n: number): Stream<number> {
//   return cons_stream(n, () => integers_from(n + 1));
// }

/** 0, 1, 1, 2, 3, 5, ... (implicito: con add_streams su fibs stesso) */
export const fibs: Stream<number> = cons_stream(0, () =>
  cons_stream(1, () => add_streams(stream_tail(fibs), fibs)),
);

export function is_divisible(x: number, y: number): boolean {
  return x % y === 0;
}

/** Crivello di Eratostene: sieve(integers_from(2)) = 2, 3, 5, 7, ... */
export function sieve(s: Stream<number>): Stream<number> {
  return cons_stream(stream_head(s), () =>
    sieve(
      stream_filter((x) => !is_divisible(x, stream_head(s)), stream_tail(s)),
    ),
  );
}
function square(x: number): number {
  return x * x;
}

/** 2, 3, 5, 7, ... (implicito: usa is_prime, che a sua volta usa primes) */
export const primes: Stream<number> = cons_stream(2, () =>
  stream_filter(is_prime, integers_from(3)),
);

/** Primalità controllando solo i divisori primi fino a √n, presi da `primes`. */
export function is_prime(n: number): boolean {
  if (n <= 1) return false;
  function iter(ps: Stream<number>): boolean {
    return square(stream_head(ps)) > n
      ? true
      : is_divisible(n, stream_head(ps))
        ? false
        : iter(stream_tail(ps));
  }
  return iter(primes);
}

/** Es. 3.56: fonde due stream crescenti senza ripetizioni. */
export function merge(a: Stream<number>, b: Stream<number>): Stream<number> {
  if (a === the_empty_stream) return b;
  else if (b === the_empty_stream) return a;
  else {
    const a_head = stream_head(a);
    const b_head = stream_head(b);
    return a_head < b_head
      ? cons_stream(a_head, () => merge(stream_tail(a), b))
      : a_head > b_head
        ? cons_stream(b_head, () => merge(a, stream_tail(b)))
        : cons_stream(a_head, () => merge(stream_tail(a), stream_tail(b)));
  }
}

/** Es. 3.56: i numeri senza fattori primi diversi da 2, 3, 5, in ordine. */
export const hamming: Stream<number> = cons_stream(1, () =>
  merge(
    scale_stream(hamming, 2),
    merge(scale_stream(hamming, 3), scale_stream(hamming, 5)),
  ),
);

/** Es. 3.54: 1, 2, 6, 24, ... (l'elemento n vale (n+1)!) con mul_streams. */
export const factorials: Stream<number> = cons_stream(1, () =>
  mul_streams(stream_tail(integers), factorials),
);

// ---------------------------------------------------------------------------
// Tappa 5 — sequenze (SICP JS 3.5.2-3.5.3, es. 3.55, 3.59)
// ---------------------------------------------------------------------------

/** Es. 3.55: s0, s0+s1, s0+s1+s2, ... */
export function partial_sums(s: Stream<number>): Stream<number> {
  return todo(5);
}

/** Approssimazioni successive di √x col metodo di Newton, partendo da 1. */
export function sqrt_stream(x: number): Stream<number> {
  return todo(5);
}

/** 4 · (1 - 1/3 + 1/5 - 1/7 + ...), come somme parziali. */
export const pi_stream: Stream<number> = null;

/** Acceleratore di Eulero: S(n+1) - (S(n+1) - S(n))² / (S(n-1) - 2S(n) + S(n+1)). */
export function euler_transform(s: Stream<number>): Stream<number> {
  return todo(5);
}

/** Stream di stream: s, t(s), t(t(s)), ... */
export function make_tableau(
  transform: (s: Stream<number>) => Stream<number>,
  s: Stream<number>,
): Stream<Stream<number>> {
  return todo(5);
}

/** La testa di ogni riga del tableau. */
export function accelerated_sequence(
  transform: (s: Stream<number>) => Stream<number>,
  s: Stream<number>,
): Stream<number> {
  return todo(5);
}

/** Es. 3.59: a0, a1, a2, ... → a0/1, a1/2, a2/3, ... (coefficienti dell'integrale, senza costante). */
export function integrate_series(s: Stream<number>): Stream<number> {
  return todo(5);
}

/** Coefficienti di e^x (implicito: e^x è l'integrale di se stessa). */
export const exp_series: Stream<number> = null;

/** Coefficienti di cos x e sin x, definiti l'uno con l'altro: sin' = cos, cos' = -sin. */
export const cosine_series: Stream<number> = null;
export const sine_series: Stream<number> = null;

// ---------------------------------------------------------------------------
// Tappa 6 — ritardo esplicito (SICP JS 3.5.4)
// ---------------------------------------------------------------------------

/** initial, initial + dt·i0, initial + dt·(i0+i1), ... L'integrando arriva come thunk. */
export function integral(
  delayed_integrand: () => Stream<number>,
  initial: number,
  dt: number,
): Stream<number> {
  return todo(6);
}

/** Risolve dy/dt = f(y) con y(0) = y0: y e dy si definiscono a vicenda. */
export function solve(
  f: (y: number) => number,
  y0: number,
  dt: number,
): Stream<number> {
  return todo(6);
}

// ---------------------------------------------------------------------------
// Tappa 7 — segnali (SICP JS 3.5.3, es. 3.74-3.76)
// ---------------------------------------------------------------------------

/** +1 se il segnale passa da negativo a ≥ 0, -1 se passa da ≥ 0 a negativo, altrimenti 0. */
export function sign_change_detector(
  current: number,
  previous: number,
): number {
  return todo(7);
}

/** Es. 3.74: per ogni campione, sign_change_detector rispetto al precedente (il primo si confronta con 0). */
export function zero_crossings(sense_data: Stream<number>): Stream<number> {
  return todo(7);
}

/** Es. 3.76: media di ogni campione con il successivo (lo stream risultante ha un elemento in meno). */
export function smooth(s: Stream<number>): Stream<number> {
  return todo(7);
}

/** Es. 3.76: zero_crossings del segnale prima lisciato con smooth. */
export function smoothed_zero_crossings(
  sense_data: Stream<number>,
): Stream<number> {
  return todo(7);
}
