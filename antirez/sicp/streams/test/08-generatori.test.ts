// Tappa 8 — generatori JS: stesse idee degli stream, ma senza memoria.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { take, stream_enumerate_interval, integers_from } from "../src/stream.ts";
import {
  integers_from_gen, take_gen, map_gen, filter_gen, stream_to_gen, gen_to_stream, sieve_gen,
} from "../src/generators.ts";

/** Avvolge un iteratore e conta quante volte qualcuno chiama next(). */
function counting<T>(it: Iterator<T>) {
  const c = {
    pulls: 0,
    next(): IteratorResult<T> {
      c.pulls++;
      return it.next();
    },
    [Symbol.iterator]() {
      return c;
    },
  };
  return c;
}

describe("tappa 8 · generatori", () => {
  it("integers_from_gen e take_gen", () => {
    assert.deepEqual(take_gen(integers_from_gen(1), 5), [1, 2, 3, 4, 5]);
    assert.deepEqual(take_gen([1, 2][Symbol.iterator](), 5), [1, 2], "meno valori se l'iteratore finisce");
    assert.deepEqual(take_gen(integers_from_gen(1), 0), []);
  });

  it("take_gen non chiede valori oltre l'ultimo richiesto", () => {
    const g = counting(integers_from_gen(1));
    take_gen(g, 3);
    assert.equal(g.pulls, 3);
  });

  it("un generatore si consuma: niente memoria", () => {
    const g = integers_from_gen(1);
    assert.deepEqual(take_gen(g, 3), [1, 2, 3]);
    assert.deepEqual(take_gen(g, 3), [4, 5, 6], "la seconda lettura riparte da dove si era fermata la prima");
  });

  it("map_gen è pigro e funziona con più iteratori", () => {
    let calls = 0;
    const g = map_gen((x: number) => (calls++, x * x), integers_from_gen(1));
    assert.equal(calls, 0, "creare il generatore non esegue nulla");
    assert.deepEqual(take_gen(g, 3), [1, 4, 9]);
    assert.equal(calls, 3);
    const sums = map_gen((a: number, b: number) => a + b, integers_from_gen(1), [10, 20, 30][Symbol.iterator]());
    assert.deepEqual(take_gen(sums, 10), [11, 22, 33], "finisce con l'iteratore più corto");
  });

  it("filter_gen", () => {
    assert.deepEqual(take_gen(filter_gen((x: number) => x % 3 === 0, integers_from_gen(1)), 4), [3, 6, 9, 12]);
    assert.deepEqual(take_gen(filter_gen((x: number) => x > 10, [1, 2, 3][Symbol.iterator]()), 5), []);
  });

  it("stream_to_gen: uno stream si può percorrere con for...of", () => {
    const xs: number[] = [];
    for (const x of stream_to_gen(stream_enumerate_interval(1, 5))) xs.push(x);
    assert.deepEqual(xs, [1, 2, 3, 4, 5]);
    assert.deepEqual(take_gen(stream_to_gen(integers_from(10)), 3), [10, 11, 12], "anche su uno stream infinito");
  });

  it("gen_to_stream: lo stream chiede ogni valore al generatore una volta sola", () => {
    const g = counting(integers_from_gen(1));
    const s = gen_to_stream(g);
    assert.equal(g.pulls, 1, "costruire lo stream calcola solo la testa");
    assert.deepEqual(take(s, 5), [1, 2, 3, 4, 5]);
    assert.deepEqual(take(s, 5), [1, 2, 3, 4, 5], "a differenza del generatore, lo stream si rilegge");
    assert.equal(g.pulls, 5);
    assert.equal(gen_to_stream([][Symbol.iterator]()), null, "generatore vuoto → stream vuoto");
  });

  it("sieve_gen: crivello con i generatori", () => {
    assert.deepEqual(take_gen(sieve_gen(integers_from_gen(2)), 10), [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]);
  });
});
