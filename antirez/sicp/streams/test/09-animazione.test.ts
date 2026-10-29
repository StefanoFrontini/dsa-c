// Tappa 9 — animazione: lo stream dà gli elementi, l'orologio del browser decide quando mostrarli.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { take, stream_tail, array_to_stream, integers_from, the_empty_stream } from "../src/stream.ts";
import { iterate, cycle, play } from "../src/animation.ts";

/**
 * Un finto requestAnimationFrame: tiene da parte le callback richieste e le chiama quando il test
 * dice che è arrivato un frame all'istante `t`.
 */
function fakeClock() {
  let pending: ((t: number) => void)[] = [];
  return {
    schedule(cb: (t: number) => void) {
      pending.push(cb);
    },
    get pending() {
      return pending.length;
    },
    frame(t: number) {
      const cbs = pending;
      pending = [];
      for (const cb of cbs) cb(t);
    },
  };
}

describe("tappa 9 · animazione", () => {
  it("iterate: x, f(x), f(f(x)), ...", () => {
    assert.deepEqual(take(iterate((x: number) => 2 * x, 1), 5), [1, 2, 4, 8, 16]);
    assert.deepEqual(take(iterate((s: string) => s + "!", "a"), 3), ["a", "a!", "a!!"]);
  });

  it("iterate è pigro e calcola ogni elemento una volta sola", () => {
    let calls = 0;
    const s = iterate((x: number) => (calls++, x + 1), 0);
    assert.equal(calls, 0, "costruire lo stream non chiama f");
    assert.deepEqual(take(s, 5), [0, 1, 2, 3, 4]);
    assert.equal(calls, 4);
    take(s, 5);
    assert.equal(calls, 4, "rileggere non ricalcola");
  });

  it("right_split come iterate: ogni livello riusa il precedente", () => {
    // Un "painter" finto: una stringa che descrive il disegno. beside = "|", below = "/".
    let combined = 0;
    const step = (smaller: string) => (combined++, `p|(${smaller}/${smaller})`);
    const levels = iterate(step, "p");
    assert.deepEqual(take(levels, 3), ["p", "p|(p/p)", "p|(p|(p/p)/p|(p/p))"]);
    take(levels, 7);
    assert.equal(combined, 6, "un passo per livello, non ricostruire da zero");
  });

  it("cycle ripete gli elementi all'infinito", () => {
    assert.deepEqual(take(cycle([1, 2, 3]), 7), [1, 2, 3, 1, 2, 3, 1]);
    assert.deepEqual(take(cycle(["x"]), 3), ["x", "x", "x"]);
    assert.equal(cycle([]), the_empty_stream);
  });

  it("cycle è circolare: dopo k code si torna alla stessa cella", () => {
    const c = cycle([1, 2, 3]);
    assert.equal(stream_tail(stream_tail(stream_tail(c))), c, "infinito, ma con 3 celle in tutto");
  });

  it("play: un elemento per periodo, disegnato una volta sola", () => {
    const clock = fakeClock();
    const drawn: number[] = [];
    play(integers_from(0), (x) => drawn.push(x), clock.schedule, 100);
    assert.deepEqual(drawn, [], "non disegna prima del primo frame");
    assert.equal(clock.pending, 1, "chiede subito il primo frame");
    for (const t of [1000, 1050, 1099, 1100, 1150, 1200]) clock.frame(t);
    assert.deepEqual(drawn, [0, 1, 2], "il tempo parte dal primo frame; frame nello stesso periodo non ridisegnano");
    assert.equal(clock.pending, 1, "un solo frame richiesto alla volta");
  });

  it("play salta gli elementi se il browser è lento, ma non li disegna", () => {
    const clock = fakeClock();
    const drawn: number[] = [];
    let built = 0;
    const s = iterate((x: number) => (built++, x + 1), 0);
    play(s, (x) => drawn.push(x), clock.schedule, 100);
    clock.frame(0);
    clock.frame(350);
    assert.deepEqual(drawn, [0, 3]);
    assert.equal(built, 3, "gli elementi saltati si calcolano comunque: lo stream è sequenziale");
    clock.frame(100_000 * 100);
    assert.deepEqual(drawn, [0, 3, 100_000], "una scheda tornata in primo piano dopo molto tempo");
  });

  it("play si ferma quando lo stream finisce", () => {
    const clock = fakeClock();
    const drawn: string[] = [];
    play(array_to_stream(["a", "b"]), (x) => drawn.push(x), clock.schedule, 100);
    clock.frame(0);
    clock.frame(100);
    clock.frame(200);
    assert.deepEqual(drawn, ["a", "b"]);
    assert.equal(clock.pending, 0, "niente più frame richiesti");

    const empty = fakeClock();
    const none: unknown[] = [];
    play(the_empty_stream, (x) => none.push(x), empty.schedule, 100);
    empty.frame(0);
    assert.deepEqual(none, []);
    assert.equal(empty.pending, 0);
  });

  it("stop ferma l'animazione", () => {
    const clock = fakeClock();
    const drawn: number[] = [];
    const stop = play(integers_from(0), (x) => drawn.push(x), clock.schedule, 100);
    clock.frame(0);
    clock.frame(100);
    stop();
    clock.frame(200);
    assert.deepEqual(drawn, [0, 1], "il frame già richiesto non disegna");
    assert.equal(clock.pending, 0, "e non ne chiede altri");
  });
});
