// SICP JS 3.5 — Stream in TypeScript.
// Le firme sono fissate (i test le importano), i corpi li scrivi tu, una tappa alla volta.
// Percorso e suggerimenti: ../README.md. Test: `npm test` oppure `node --test test/01-basi.test.ts`.

/** Uno stream è vuoto (null) oppure una testa già calcolata e una coda *ritardata*. */
export type Stream<T> = null | { readonly head: T; readonly tail: () => Stream<T> };

function todo(tappa: number): never {
  throw new Error(`TODO tappa ${tappa}: da implementare`);
}

// ---------------------------------------------------------------------------
// Tappa 1 — basi (SICP JS 3.5.1)
// ---------------------------------------------------------------------------

export const the_empty_stream = null;

/** Costruisce uno stream. `tail` è un thunk: NON va chiamato qui. (Tappa 2: memoizzalo.) */
export function cons_stream<T>(head: T, tail: () => Stream<T>): Stream<T> {
  return todo(1);
}

export function stream_head<T>(s: Stream<T>): T {
  return todo(1);
}

/** Forza la coda: è qui che il calcolo ritardato viene eseguito. */
export function stream_tail<T>(s: Stream<T>): Stream<T> {
  return todo(1);
}

/** L'elemento di indice n (0 = la testa). */
export function stream_ref<T>(s: Stream<T>, n: number): T {
  return todo(1);
}

/** I primi n elementi in un array (meno se lo stream finisce prima). Non forzare più del necessario. */
export function take<T>(s: Stream<T>, n: number): T[] {
  return todo(1);
}

export function array_to_stream<T>(xs: readonly T[]): Stream<T> {
  return todo(1);
}

/** low, low+1, ..., high (vuoto se low > high). */
export function stream_enumerate_interval(low: number, high: number): Stream<number> {
  return todo(1);
}

/** n, n+1, n+2, ... all'infinito. */
export function integers_from(n: number): Stream<number> {
  return todo(1);
}

// ---------------------------------------------------------------------------
// Tappa 2 — memoizzazione (SICP JS 3.5.1, "memo")
// ---------------------------------------------------------------------------

/** Restituisce un thunk che chiama `f` solo la prima volta e poi restituisce sempre lo stesso valore. */
export function memo<T>(f: () => T): () => T {
  return todo(2);
}

// ---------------------------------------------------------------------------
// Tappa 3 — operazioni (SICP JS 3.5.1, es. 3.50)
// ---------------------------------------------------------------------------

/** Come `map`, ma su uno o più stream: f riceve un elemento da ciascuno. */
export function stream_map<R>(f: (...args: any[]) => R, ...streams: Stream<any>[]): Stream<R> {
  return todo(3);
}

export function stream_filter<T>(pred: (x: T) => boolean, s: Stream<T>): Stream<T> {
  return todo(3);
}

export function add_streams(a: Stream<number>, b: Stream<number>): Stream<number> {
  return todo(3);
}

export function mul_streams(a: Stream<number>, b: Stream<number>): Stream<number> {
  return todo(3);
}

export function scale_stream(s: Stream<number>, factor: number): Stream<number> {
  return todo(3);
}

// ---------------------------------------------------------------------------
// Tappa 4 — stream infiniti (SICP JS 3.5.2)
// Le costanti valgono null finché non le scrivi. Definiscile con cons_stream e,
// dove indicato, *implicitamente*: lo stream si riferisce a se stesso nella coda.
// ---------------------------------------------------------------------------

/** 1, 1, 1, ... (implicito) */
export const ones: Stream<number> = null;

/** 1, 2, 3, ... (implicito: con ones e add_streams, senza integers_from) */
export const integers: Stream<number> = null;

/** 0, 1, 1, 2, 3, 5, ... (implicito: con add_streams su fibs stesso) */
export const fibs: Stream<number> = null;

/** Crivello di Eratostene: sieve(integers_from(2)) = 2, 3, 5, 7, ... */
export function sieve(s: Stream<number>): Stream<number> {
  return todo(4);
}

/** 2, 3, 5, 7, ... (implicito: usa is_prime, che a sua volta usa primes) */
export const primes: Stream<number> = null;

/** Primalità controllando solo i divisori primi fino a √n, presi da `primes`. */
export function is_prime(n: number): boolean {
  return todo(4);
}

/** Es. 3.56: fonde due stream crescenti senza ripetizioni. */
export function merge(a: Stream<number>, b: Stream<number>): Stream<number> {
  return todo(4);
}

/** Es. 3.56: i numeri senza fattori primi diversi da 2, 3, 5, in ordine. */
export const hamming: Stream<number> = null;

/** Es. 3.54: 1, 2, 6, 24, ... (l'elemento n vale (n+1)!) con mul_streams. */
export const factorials: Stream<number> = null;

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
export function make_tableau(transform: (s: Stream<number>) => Stream<number>, s: Stream<number>): Stream<Stream<number>> {
  return todo(5);
}

/** La testa di ogni riga del tableau. */
export function accelerated_sequence(transform: (s: Stream<number>) => Stream<number>, s: Stream<number>): Stream<number> {
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
export function integral(delayed_integrand: () => Stream<number>, initial: number, dt: number): Stream<number> {
  return todo(6);
}

/** Risolve dy/dt = f(y) con y(0) = y0: y e dy si definiscono a vicenda. */
export function solve(f: (y: number) => number, y0: number, dt: number): Stream<number> {
  return todo(6);
}

// ---------------------------------------------------------------------------
// Tappa 7 — segnali (SICP JS 3.5.3, es. 3.74-3.76)
// ---------------------------------------------------------------------------

/** +1 se il segnale passa da negativo a ≥ 0, -1 se passa da ≥ 0 a negativo, altrimenti 0. */
export function sign_change_detector(current: number, previous: number): number {
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
export function smoothed_zero_crossings(sense_data: Stream<number>): Stream<number> {
  return todo(7);
}
