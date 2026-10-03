// Tappa 1 — SICP JS 3.5.1: costruire uno stream con una coda ritardata.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import {
  the_empty_stream, cons_stream, stream_head, stream_tail, stream_ref, take,
  array_to_stream, stream_enumerate_interval, integers_from,
} from "../src/stream.ts";

describe("tappa 1 · basi", () => {
  it("lo stream vuoto è null", () => {
    assert.equal(the_empty_stream, null);
  });

  it("cons_stream non valuta la coda quando costruisce lo stream", () => {
    const s = cons_stream(1, () => { throw new Error("coda valutata troppo presto"); });
    assert.equal(stream_head(s), 1);
    assert.throws(() => stream_tail(s), /troppo presto/);
  });

  it("stream_head e stream_tail su uno stream di due elementi", () => {
    const s = cons_stream("a", () => cons_stream("b", () => null));
    assert.equal(stream_head(s), "a");
    assert.equal(stream_head(stream_tail(s)), "b");
    assert.equal(stream_tail(stream_tail(s)), null);
  });

  it("integers_from(1) è infinito ma non va in stack overflow", () => {
    const s = integers_from(1);
    assert.equal(stream_head(s), 1);
    assert.equal(stream_ref(s, 100), 101);
    assert.equal(stream_ref(s, 1000), 1001);
  });

  it("take restituisce al massimo n elementi in un array", () => {
    assert.deepEqual(take(integers_from(5), 3), [5, 6, 7]);
    assert.deepEqual(take(integers_from(5), 0), []);
    assert.deepEqual(take(stream_enumerate_interval(1, 3), 10), [1, 2, 3]);
  });

  it("take non forza la coda dopo l'ultimo elemento richiesto", () => {
    const s = cons_stream(1, () => cons_stream(2, () => { throw new Error("forzato un elemento in più"); }));
    assert.deepEqual(take(s, 2), [1, 2]);
  });

  it("stream_enumerate_interval è finito e vuoto se low > high", () => {
    assert.deepEqual(take(stream_enumerate_interval(3, 7), 100), [3, 4, 5, 6, 7]);
    assert.equal(stream_enumerate_interval(3, 2), null);
  });

  it("array_to_stream", () => {
    assert.deepEqual(take(array_to_stream(["x", "y", "z"]), 10), ["x", "y", "z"]);
    assert.equal(array_to_stream([]), null);
  });
});
