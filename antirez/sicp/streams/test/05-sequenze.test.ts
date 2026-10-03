// Tappa 5 — SICP JS 3.5.2-3.5.3: somme parziali, approssimazioni, serie di potenze.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import {
  take, stream_ref, integers, ones, partial_sums, sqrt_stream, pi_stream, euler_transform,
  accelerated_sequence, integrate_series, exp_series, cosine_series, sine_series,
} from "../src/stream.ts";

const close = (a: number, b: number, eps: number, msg?: string) =>
  assert.ok(Math.abs(a - b) < eps, msg ?? `${a} non è vicino a ${b} (eps ${eps})`);
const closeAll = (as: number[], bs: number[], eps = 1e-12) =>
  as.forEach((a, i) => close(a, bs[i], eps, `elemento ${i}: ${a} invece di ${bs[i]}`));

describe("tappa 5 · sequenze", () => {
  it("es. 3.55: partial_sums", () => {
    assert.deepEqual(take(partial_sums(integers), 5), [1, 3, 6, 10, 15]);
  });

  it("sqrt_stream converge a √2", () => {
    close(stream_ref(sqrt_stream(2), 5), Math.SQRT2, 1e-12);
  });

  it("pi_stream converge lentamente", () => {
    closeAll(take(pi_stream, 3), [4, 4 - 4 / 3, 4 - 4 / 3 + 4 / 5]);
    assert.ok(Math.abs(stream_ref(pi_stream, 7) - Math.PI) > 0.1);
  });

  it("euler_transform accelera la convergenza", () => {
    close(stream_ref(euler_transform(pi_stream), 7), Math.PI, 1e-3);
  });

  it("accelerated_sequence (tableau) arriva quasi alla precisione macchina", () => {
    close(stream_ref(accelerated_sequence(euler_transform, pi_stream), 7), Math.PI, 1e-13);
  });

  it("es. 3.59: integrate_series e la serie di e^x", () => {
    closeAll(take(integrate_series(ones), 4), [1, 1 / 2, 1 / 3, 1 / 4]);
    closeAll(take(exp_series, 5), [1, 1, 1 / 2, 1 / 6, 1 / 24]);
  });

  it("es. 3.59: seno e coseno definiti l'uno con l'altro", () => {
    closeAll(take(cosine_series, 6), [1, 0, -1 / 2, 0, 1 / 24, 0]);
    closeAll(take(sine_series, 6), [0, 1, 0, -1 / 6, 0, 1 / 120]);
  });
});
