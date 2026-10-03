// Tappa 4 — SICP JS 3.5.2: stream infiniti e definizioni implicite.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import {
  take, stream_ref, integers_from, ones, integers, fibs, sieve, primes, is_prime, hamming, factorials,
} from "../src/stream.ts";

const first10primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29];

describe("tappa 4 · stream infiniti", () => {
  it("ones e integers (definiti implicitamente)", () => {
    assert.deepEqual(take(ones, 5), [1, 1, 1, 1, 1]);
    assert.deepEqual(take(integers, 5), [1, 2, 3, 4, 5]);
    assert.equal(stream_ref(integers, 1000), 1001);
  });

  it("fibs: senza memo questo test non finisce in tempo", { timeout: 2000 }, () => {
    assert.deepEqual(take(fibs, 10), [0, 1, 1, 2, 3, 5, 8, 13, 21, 34]);
    assert.equal(stream_ref(fibs, 50), 12586269025);
  });

  it("crivello di Eratostene", () => {
    assert.deepEqual(take(sieve(integers_from(2)), 10), first10primes);
  });

  it("primes definito implicitamente con is_prime che usa primes", () => {
    assert.deepEqual(take(primes, 10), first10primes);
    assert.equal(stream_ref(primes, 999), 7919);
    assert.deepEqual([2, 3, 97, 7919].map(is_prime), [true, true, true, true]);
    assert.deepEqual([1, 91, 100, 7917].map(is_prime), [false, false, false, false]);
  });

  it("es. 3.56: numeri di Hamming (solo fattori 2, 3, 5) con merge", () => {
    assert.deepEqual(take(hamming, 15), [1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24]);
  });

  it("es. 3.54: factorials con mul_streams", () => {
    assert.deepEqual(take(factorials, 6), [1, 2, 6, 24, 120, 720]);
  });
});
