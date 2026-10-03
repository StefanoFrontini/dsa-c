// Tappa 3 — SICP JS 3.5.1: stream_map (es. 3.50), stream_filter e la pigrizia.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import {
  take, stream_ref, stream_head, stream_tail, integers_from, stream_enumerate_interval,
  stream_map, stream_filter, add_streams, mul_streams, scale_stream,
} from "../src/stream.ts";

describe("tappa 3 · operazioni", () => {
  it("stream_map con uno stream", () => {
    assert.deepEqual(take(stream_map((x: number) => x * x, integers_from(1)), 5), [1, 4, 9, 16, 25]);
  });

  it("stream_map con più stream (es. 3.50)", () => {
    const s = stream_map((a: number, b: number, c: number) => a + b + c,
      integers_from(1), integers_from(10), integers_from(100));
    assert.deepEqual(take(s, 3), [111, 114, 117]);
    assert.deepEqual(take(stream_map((a: number, b: number) => a * b,
      stream_enumerate_interval(1, 3), stream_enumerate_interval(4, 6)), 10), [4, 10, 18]);
  });

  it("stream_filter", () => {
    assert.deepEqual(take(stream_filter((x: number) => x % 2 === 0, integers_from(1)), 5), [2, 4, 6, 8, 10]);
    assert.equal(stream_filter((x: number) => x > 10, stream_enumerate_interval(1, 5)), null);
  });

  it("la pigrizia: il secondo primo tra 10000 e 1000000 costa 10 test, non 990000", () => {
    let tests = 0;
    const is_prime = (n: number) => {
      tests++;
      for (let d = 2; d * d <= n; d++) if (n % d === 0) return false;
      return n > 1;
    };
    const second = stream_head(stream_tail(stream_filter(is_prime, stream_enumerate_interval(10000, 1000000))));
    assert.equal(second, 10009);
    assert.ok(tests <= 10, `is_prime chiamato ${tests} volte`);
  });

  it("es. 3.51: stream_map applica f solo quando serve, e una volta sola", () => {
    const shown: number[] = [];
    const show = (x: number) => { shown.push(x); return x; };
    const x = stream_map(show, stream_enumerate_interval(0, 10));
    assert.deepEqual(shown, [0]);
    stream_ref(x, 5);
    assert.deepEqual(shown, [0, 1, 2, 3, 4, 5]);
    stream_ref(x, 7);
    assert.deepEqual(shown, [0, 1, 2, 3, 4, 5, 6, 7]);
  });

  it("add_streams, mul_streams, scale_stream", () => {
    assert.deepEqual(take(add_streams(integers_from(1), integers_from(10)), 3), [11, 13, 15]);
    assert.deepEqual(take(mul_streams(integers_from(1), integers_from(1)), 4), [1, 4, 9, 16]);
    assert.deepEqual(take(scale_stream(integers_from(1), 3), 4), [3, 6, 9, 12]);
  });
});
