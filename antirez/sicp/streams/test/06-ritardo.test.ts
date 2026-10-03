// Tappa 6 — SICP JS 3.5.4: ritardo esplicito, integral e solve.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { take, stream_ref, ones, integral, solve } from "../src/stream.ts";

describe("tappa 6 · ritardo esplicito", () => {
  it("integral somma l'integrando moltiplicato per dt", () => {
    assert.deepEqual(take(integral(() => ones, 0, 0.5), 5), [0, 0.5, 1, 1.5, 2]);
  });

  it("integral non chiama il thunk dell'integrando quando viene costruito", () => {
    assert.doesNotThrow(() => integral(() => { throw new Error("integrando forzato subito"); }, 0, 1));
  });

  it("solve(y => y, 1, 0.001) approssima e dopo 1000 passi", () => {
    const y = solve((v) => v, 1, 0.001);
    assert.ok(Math.abs(stream_ref(y, 1000) - 2.716923932235896) < 1e-9);
  });
});
