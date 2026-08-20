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
// const a = pair(list(1, 2), list(3, 4))
// print_list(len(list(a, a)));
// for_each(print_list, list(10, 20, 30));
// for_each((x:number) => x * 2, l);
// print_list(map(((x:number) => x * x), l));
function make_vector(x, y) {
    return pair(x, y);
}
function xcor_vect(vector) {
    return head(vector);
}
function ycor_vect(vector) {
    return tail(vector);
}
function add_vect(vector1, vector2) {
    return make_vector(xcor_vect(vector1) + xcor_vect(vector2), ycor_vect(vector1) + ycor_vect(vector2));
}
function sub_vect(vector1, vector2) {
    return make_vector(xcor_vect(vector1) - xcor_vect(vector2), ycor_vect(vector1) - ycor_vect(vector2));
}
function scale_vect(f, vector) {
    return make_vector(f * xcor_vect(vector), f * ycor_vect(vector));
}
function make_segment(vector1, vector2) {
    return pair(vector1, vector2);
}
function start_segment(segment) {
    return head(segment);
}
function end_segment(segment) {
    return tail(segment);
}
function make_frame(origin, edge1, edge2) {
    return list(origin, edge1, edge2);
}
function origin_frame(frame) {
    return list_ref(frame, 0);
}
function edge1_frame(frame) {
    return list_ref(frame, 1);
}
function edge2_frame(frame) {
    return list_ref(frame, 2);
}
function draw_line(ctx, vector1, vector2) {
    if (!ctx)
        return;
    ctx.beginPath(); // Start a new path
    ctx.moveTo(xcor_vect(vector1), ycor_vect(vector1)); // Move the pen to vector1
    ctx.lineTo(xcor_vect(vector2), ycor_vect(vector2)); // Draw a line to vector2
    ctx.stroke(); // Render the path
}
function frame_coord_map(frame) {
    return (v) => add_vect(origin_frame(frame), add_vect(scale_vect(xcor_vect(v), edge1_frame(frame)), scale_vect(ycor_vect(v), edge2_frame(frame))));
}
function segments_to_painter(segment_list) {
    return (ctx, frame) => for_each((segment) => draw_line(ctx, frame_coord_map(frame)(start_segment(segment)), frame_coord_map(frame)(end_segment(segment))), segment_list);
}
const p1 = make_vector(0.25, 0);
const p2 = make_vector(0.35, 0.5);
const p3 = make_vector(0.3, 0.6);
const p4 = make_vector(0.15, 0.4);
const p5 = make_vector(0, 0.65);
const p6 = make_vector(0.4, 0);
const p7 = make_vector(0.5, 0.3);
const p8 = make_vector(0.6, 0);
const p9 = make_vector(0.75, 0);
const p10 = make_vector(0.6, 0.45);
const p11 = make_vector(1, 0.15);
const p12 = make_vector(1, 0.35);
const p13 = make_vector(0.75, 0.65);
const p14 = make_vector(0.6, 0.65);
const p15 = make_vector(0.65, 0.85);
const p16 = make_vector(0.6, 1);
const p17 = make_vector(0.4, 1);
const p18 = make_vector(0.35, 0.85);
const p19 = make_vector(0.4, 0.65);
const p20 = make_vector(0.3, 0.65);
const p21 = make_vector(0.15, 0.6);
const p22 = make_vector(0, 0.85);
const george_lines = list(make_segment(p1, p2), make_segment(p2, p3), make_segment(p3, p4), make_segment(p4, p5), make_segment(p6, p7), make_segment(p7, p8), make_segment(p9, p10), make_segment(p10, p11), make_segment(p12, p13), make_segment(p13, p14), make_segment(p14, p15), make_segment(p15, p16), make_segment(p17, p18), make_segment(p18, p19), make_segment(p19, p20), make_segment(p20, p21), make_segment(p21, p22));
const painter = segments_to_painter(george_lines);
const frame1 = make_frame(make_vector(0, 600), // origine in basso a sinistra
make_vector(600, 0), // edge1 punta a destra
make_vector(0, -600));
function transform_painter(painter, origin, corner1, corner2) {
    return (ctx, frame) => {
        const m = frame_coord_map(frame);
        const new_origin = m(origin);
        return painter(ctx, make_frame(new_origin, sub_vect(m(corner1), new_origin), sub_vect(m(corner2), new_origin)));
    };
}
function flip_vert(painter) {
    return transform_painter(painter, make_vector(0, 1), make_vector(1, 1), make_vector(0, 0));
}
function shrink_to_upper_right(painter) {
    return transform_painter(painter, make_vector(0.5, 0.5), make_vector(1, 0.5), make_vector(0.5, 1));
}
function rotate90(painter) {
    return transform_painter(painter, make_vector(1, 0), make_vector(1, 1), make_vector(0, 0));
}
function squash_inwards(painter) {
    return transform_painter(painter, make_vector(0, 0), make_vector(0.65, 0.35), make_vector(0.35, 0.65));
}
function beside(painter1, painter2) {
    const split_point = make_vector(0.5, 0);
    const paint_left = transform_painter(painter1, make_vector(0, 0), split_point, make_vector(0, 1));
    const paint_right = transform_painter(painter2, split_point, make_vector(1, 0), make_vector(0.5, 1));
    return (ctx, frame) => {
        paint_left(ctx, frame);
        paint_right(ctx, frame);
    };
}
const flip_painter = flip_vert(painter);
const shrink_to_upper_right_painter = shrink_to_upper_right(painter);
const rotate90_painter = rotate90(painter);
const squash_inwards_painter = squash_inwards(painter);
const beside_painter = beside(painter, painter);
// flip_painter(ctx, frame1);
// shrink_to_upper_right_painter(ctx, frame1);
// rotate90_painter(ctx, frame1);
// squash_inwards_painter(ctx, frame1);
beside_painter(ctx, frame1);
// const v1 = make_vector(0, 0);
// const v2 = make_vector(100, 100);
// draw_line(ctx, v1, v2);
// const v = make_vector(1, 2);
// print_list(xcor_vect(v));
// print_list(ycor_vect(v));
