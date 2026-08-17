"use strict";
const canvas = document.getElementById("canvas");
if (!canvas)
    throw new Error("canvas is null");
const ctx = canvas.getContext("2d");
if (!ctx)
    throw new Error("ctx is null");
console.log(ctx);
const CANVAS_WIDTH = (canvas.width = 600);
const CANVAS_HEIGHT = (canvas.height = 600);
// type PairFn = (a: number | Pair, b: number | Pair) => Pair;
// const pair: PairFn = (a, b) => ({ head: a, tail: b });
function pair(a, b) {
    return {
        head: a,
        tail: b,
    };
}
