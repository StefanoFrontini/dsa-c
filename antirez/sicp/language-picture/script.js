"use strict";
const canvas = document.getElementById("canvas");
if (!canvas)
    throw new Error("canvas is null");
const ctx = canvas.getContext("2d");
if (!ctx)
    throw new Error("ctx is null");
// console.log(ctx);
const CANVAS_WIDTH = (canvas.width = 600);
const CANVAS_HEIGHT = (canvas.height = 600);
// cons
function pair(a, b) {
    return {
        head: a,
        tail: b,
    };
}
// car
function head(p) {
    return typeof p === "number" || p === null ? p : p.head;
}
// cdr
function tail(p) {
    return typeof p === "number" || p === null ? p : p.tail;
}
const x = pair(1, 2);
const y = pair(3, 4);
const z = pair(x, y);
// console.log(head(x));
// console.log(tail(x));
// console.log(head(head(z)));
function list(...args) {
    // console.log("args: ", args);
    function list_rec(acc, ...args_rec) {
        if (args_rec.length === 0)
            return acc;
        // console.log("args_rec[0]: ", args_rec[0]);
        // console.log("...args_rec.slice(1):", ...args_rec.slice(1));
        return pair(args_rec[0], list_rec(acc, ...args_rec.slice(1)));
    }
    return list_rec(null, ...args);
}
function print_list(l) {
    if (typeof l === "number" || l === null) {
        console.log(l);
        return;
    }
    function print_list_rec(l, acc) {
        if (l === null)
            return acc;
        return print_list_rec(tail(l), [...acc, head(l)]);
    }
    const res = print_list_rec(l, []);
    console.log(res);
}
// const l = list(1, list(4, 5), 3);
const l = list(1, 2, 3, 4);
const l2 = list(5, 6, 7, 8);
// console.log("l:", l);
// console.log("print_list");
// const p = pair(10, l);
// print_list(l);
// print_list(head(p));
// print_list(tail(p));
function list_ref(items, n) {
    return n === 0 ? head(items) : list_ref(tail(items), n - 1);
}
function is_null(l) {
    return l === null;
}
function len(items) {
    return is_null(items) ? 0 : 1 + len(tail(items));
}
function append(list1, list2) {
    return is_null(list1) ? list2 : pair(head(list1), append(tail(list1), list2));
}
// print_list(len(l));
// print_list(append(l, l2));
function map(fun, items) {
    return is_null(items)
        ? null
        : pair(fun(head(items)), map(fun, tail(items)));
}
function for_each(fun, items) {
    if (is_null(items))
        return;
    fun(head(items));
    for_each(fun, tail(items));
}
const a = pair(list(1, 2), list(3, 4));
print_list(len(list(a, a)));
// for_each(print_list, list(10, 20, 30));
// for_each((x:number) => x * 2, l);
// print_list(map(((x:number) => x * x), l));
