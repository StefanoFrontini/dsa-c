// Tappa 2 — SICP JS 3.5.1 (memo): ogni coda va calcolata una volta sola.
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { memo, cons_stream, take, type Stream } from "../src/stream.ts";

describe("tappa 2 · memoizzazione", () => {
  it("memo chiama la funzione solo la prima volta", () => {
    let calls = 0;
    const f = memo(() => { calls++; return 42; });
    assert.equal(calls, 0, "memo non deve chiamare f subito");
    assert.equal(f(), 42);
    assert.equal(f(), 42);
    assert.equal(calls, 1);
  });

  it("memo funziona anche quando il valore è null, 0 o undefined", () => {
    for (const v of [null, 0, undefined, false, ""]) {
      let calls = 0;
      const f = memo(() => { calls++; return v; });
      f(); f(); f();
      assert.equal(calls, 1, `valore ${String(v)}: f chiamata ${calls} volte`);
    }
  });

  it("leggere due volte gli stessi elementi non ricalcola le code", () => {
    let created = 0;
    function counted(n: number): Stream<number> {
      created++;
      return cons_stream(n, () => counted(n + 1));
    }
    const s = counted(1);
    take(s, 10);
    take(s, 10);
    assert.equal(created, 10);
  });
});
