// Tappa 7 — SICP JS 3.5.3, es. 3.74-3.76: uno stream di campioni, come un segnale audio.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { take, array_to_stream, sign_change_detector, zero_crossings, smooth, smoothed_zero_crossings } from "../src/stream.ts";

const all = <T>(s: Parameters<typeof take<T>>[0]) => take(s, Infinity);

describe("tappa 7 · segnali", () => {
  it("sign_change_detector(corrente, precedente)", () => {
    assert.equal(sign_change_detector(1, -1), 1);
    assert.equal(sign_change_detector(-1, 1), -1);
    assert.equal(sign_change_detector(2, 1), 0);
    assert.equal(sign_change_detector(-2, -1), 0);
    assert.equal(sign_change_detector(0, -1), 1, "lo zero conta come positivo");
  });

  it("es. 3.74: zero_crossings sull'esempio del libro", () => {
    const sense_data = array_to_stream([1, 2, 1.5, 1, 0.5, -0.1, -2, -3, -2, -0.5, 0.2, 3, 4]);
    assert.deepEqual(all(zero_crossings(sense_data)), [0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0]);
  });

  it("es. 3.76: smooth fa la media di ogni coppia di campioni consecutivi", () => {
    assert.deepEqual(all(smooth(array_to_stream([0, 2, 4, 0]))), [1, 3, 2]);
  });

  it("es. 3.76: con il rumore i passaggi per lo zero spuri spariscono", () => {
    // 4 periodi di seno campionati 50 volte per periodo, più un rumore che alterna +0.3 e -0.3.
    const clean = Array.from({ length: 200 }, (_, i) => Math.sin((2 * Math.PI * (i + 0.5)) / 50));
    const noisy = clean.map((v, i) => v + (i % 2 === 0 ? 0.3 : -0.3));
    const count = (xs: number[]) => xs.filter((x) => x !== 0).length;
    const real = count(all(zero_crossings(array_to_stream(clean))));
    assert.ok(count(all(zero_crossings(array_to_stream(noisy)))) > real, "il rumore deve aggiungere passaggi spuri");
    assert.equal(count(all(smoothed_zero_crossings(array_to_stream(noisy)))), real);
  });
});
