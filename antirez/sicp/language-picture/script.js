"use strict";
/*
Bluebird Bxyz = x(yz)
*/
// const B = x => y => z => x(y(z))
// Un tipo di utilità per estrarre il tipo dell'argomento e del ritorno di una funzione
// type Func = (arg: any) => any;
// Tipo ricorsivo che convalida la catena di funzioni
// type Pipeline<Fns extends Func[], FirstArg> =
//   Fns extends [infer First extends Func, ...infer Rest extends Func[]]
//     ? [ (arg: FirstArg) => ReturnType<First>, ...Pipeline<Rest, ReturnType<First>> ]
//     : [];
// function pipe<T, Fns extends Func[]>(
//   initialValue: T,
//   ...fns: Pipeline<Fns, T> & Fns
// ): any {
//   return fns.reduce((acc, fn) => fn(acc), initialValue);
// }
// const incrementa = (n: number): number => n + 1;
// const raddoppia  = (n: number): number => n * 2;
// const quadrato   = (n: number): number => n * n;
// const inStringa  = (n: number): string => `Risultato finale: ${n}`;
// Funziona perfettamente! TypeScript deduce tutti i tipi lungo la catena.
// const risultato = pipe(
//   2,
//   incrementa, // 2 -> 3
//   raddoppia,  // 3 -> 6
//   quadrato,   // 6 -> 36
//   inStringa   // 36 -> "Risultato finale: 36"
// );
// console.log("res: ", risultato)
// const trasformaInStringa = (n: number): string => `Numero: ${n}`;
// const richiedeArray = (arr: any[]): number => arr.length; // Si aspetta un Array, non una stringa!
// const erroreCompilazione = pipe(
//   5,
//   incrementa,
//   trasformaInStringa,
// );
const B = (x) => (y) => (z) => x(y(z));
const K = (x) => (y) => x;
// Cardinal Cxyz = xzy
// const C =
//   <U, T, V>(x: (arg: T) => (arg: U) => V) =>
//   (y: (arg: U) => V) =>
//   (z: T): V =>
//     x(z)(y);
const C = (x) => (y) => (z) => x(z)(y);
// Il Mockingbird fortemente tipizzato
const M = (x) => x(x);
// Warbler Wxy = xyy 
const W = (x) => (y) => x(y)(y);
// Robin Rxyz = yzx
const R = (x) => (y) => (z) => y(z)(x);
// Thrush Txy = yx
const T = (x) => (y) => y(x);
// Finch Fxyz = zyx
const F = (x) => (y) => (z) => z(y)(x);
// const Becard = B(B(B))(B);
// function B<U, V>(x: (arg: U) => V) {
//   return function first<T>(y: (arg: T) => U) {
//     return function second(z: T): V {
//       return x(y(z));
//     };
//   };
// }
// z: La funzione di partenza. Prende un testo e lo stampa.
// const logBase = (messaggio: string): void => {
//   console.log(`[LOG]: ${messaggio}`);
// };
// y: Prende una funzione e restituisce una nuova funzione che aggiunge un timestamp prima di eseguirla
// const conTimestamp = (fn: (msg: string) => void) => {
//   return (messaggio: string): void => {
//     const data = new Date().toLocaleTimeString();
//     fn(`[${data}] ${messaggio}`);
//   };
// };
// x: Prende una funzione e restituisce una nuova funzione che aggiunge dei separatori grafici visivi
// const conSeparatore = (fn: (msg: string) => void) => {
//   return (messaggio: string): void => {
//     console.log("--- INIZIO LOG ---");
//     fn(messaggio);
//     console.log("--- FINE LOG ---");
//   };
// };
// const addOne = (n: number): number => n + 1;
// const intToString = (n: number): string => `Result: ${n}`
// const pipeline = B(intToString)(addOne)
// const output = B(conSeparatore)(conTimestamp)(logBase)
// console.log(conSeparatore(output))
// const K = x => y => x
// const pipeline = K("ciao")(5);
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
function segments_to_painter(ctx, segment_list) {
    return (frame) => for_each((segment) => draw_line(ctx, frame_coord_map(frame)(start_segment(segment)), frame_coord_map(frame)(end_segment(segment))), segment_list);
}
function image_to_painter(ctx, img) {
    return (frame) => {
        if (!ctx)
            return;
        const o = origin_frame(frame);
        const e1 = edge1_frame(frame);
        const e2 = edge2_frame(frame);
        ctx.save();
        ctx.transform(xcor_vect(e1), ycor_vect(e1), xcor_vect(e2), ycor_vect(e2), xcor_vect(o), ycor_vect(o));
        // Ribalta l'asse Y nativo dell'immagine per allinearlo al sistema cartesiano
        ctx.translate(0, 1);
        ctx.scale(1, -1);
        ctx.drawImage(img, 0, 0, 1, 1);
        ctx.restore();
    };
}
function loadImage(url) {
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.crossOrigin = "anonymous";
        img.onload = () => resolve(img);
        img.onerror = (err) => reject(err);
        img.src = url;
    });
}
// george points
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
const painter = segments_to_painter(ctx, george_lines);
const frame1 = make_frame(make_vector(0, 600), // origine in basso a sinistra
make_vector(600, 0), // edge1 punta a destra
make_vector(0, -600));
function transform_painter(painter, origin, corner1, corner2) {
    return (frame) => {
        const m = frame_coord_map(frame);
        const new_origin = m(origin);
        return painter(make_frame(new_origin, sub_vect(m(corner1), new_origin), sub_vect(m(corner2), new_origin)));
    };
}
function identity(painter) {
    return painter;
}
function flip_vert(painter) {
    return transform_painter(painter, make_vector(0, 1), make_vector(1, 1), make_vector(0, 0));
}
function flip_horiz(painter) {
    return transform_painter(painter, make_vector(1, 0), make_vector(0, 0), make_vector(1, 1));
}
function shrink_to_upper_right(painter) {
    return transform_painter(painter, make_vector(0.5, 0.5), make_vector(1, 0.5), make_vector(0.5, 1));
}
function rotate90(painter) {
    // counterclockwise
    return transform_painter(painter, make_vector(1, 0), make_vector(1, 1), make_vector(0, 0));
}
function rotate180(painter) {
    // counterclockwise
    return transform_painter(painter, make_vector(1, 1), make_vector(0, 1), make_vector(1, 0));
}
function rotate270(painter) {
    // counterclockwise
    return transform_painter(painter, make_vector(0, 1), make_vector(0, 0), make_vector(1, 1));
}
function squash_inwards(painter) {
    return transform_painter(painter, make_vector(0, 0), make_vector(0.65, 0.35), make_vector(0.35, 0.65));
}
const beside = (p1) => {
    const split_point = make_vector(0.5, 0);
    const paint_left = transform_painter(p1, make_vector(0, 0), split_point, make_vector(0, 1));
    return (p2) => {
        const paint_right = transform_painter(p2, split_point, make_vector(1, 0), make_vector(0.5, 1));
        return (frame) => {
            paint_left(frame);
            paint_right(frame);
        };
    };
};
// function beside(
//   painter1: Painter,
//   painter2: Painter,
// ): Painter {
//   const split_point = make_vector(0.5, 0);
//   const paint_left = transform_painter(
//     painter1,
//     make_vector(0, 0),
//     split_point,
//     make_vector(0, 1),
//   );
//   const paint_right = transform_painter(
//     painter2,
//     split_point,
//     make_vector(1, 0),
//     make_vector(0.5, 1),
//   );
//   return (frame: Pair) => {
//     paint_left(frame);
//     paint_right(frame);
//   };
// }
const below = (p1) => {
    const split_point = make_vector(0, 0.5);
    const paint_bottom = transform_painter(p1, make_vector(0, 0), make_vector(1, 0), split_point);
    return (p2) => {
        const paint_top = transform_painter(p2, split_point, make_vector(1, 0.5), make_vector(0, 1));
        return (frame) => {
            paint_bottom(frame);
            paint_top(frame);
        };
    };
};
// function below(
//   painter1: Painter,
//   painter2: Painter,
// ): Painter {
//   const split_point = make_vector(0, 0.5);
//   const paint_bottom = transform_painter(
//     painter1,
//     make_vector(0, 0),
//     make_vector(1, 0),
//     split_point,
//   );
//   const paint_top = transform_painter(
//     painter2,
//     split_point,
//     make_vector(1, 0.5),
//     make_vector(0, 1),
//   );
//   return (frame: Pair) => {
//     paint_bottom(frame);
//     paint_top(frame);
//   };
// }
// function below2(
//   painter1: Painter,
//   painter2: Painter,
// ): Painter {
//   return rotate270(beside(rotate90(painter1))(rotate90(painter2)));
// }
function right_split(painter, n) {
    if (n === 0) {
        return painter;
    }
    else {
        const smaller = right_split(painter, n - 1);
        return beside(painter)(below(smaller)(smaller));
    }
}
function up_split(painter, n) {
    if (n === 0) {
        return painter;
    }
    else {
        const smaller = up_split(painter, n - 1);
        return below(painter)(beside(smaller)(smaller));
    }
}
function corner_split(painter, n) {
    if (n === 0) {
        return painter;
    }
    else {
        const up = up_split(painter, n - 1);
        const right = right_split(painter, n - 1);
        const top_left = beside(up)(up);
        const bottom_right = below(right)(right);
        const corner = corner_split(painter, n - 1);
        return beside(below(painter)(top_left))(below(bottom_right)(corner));
    }
}
function square_split(painter, n) {
    const quarter = corner_split(painter, n);
    const half = beside(flip_horiz(quarter))(quarter);
    return below(flip_vert(half))(half);
}
function square_of_four(tl, tr, bl, br) {
    return (painter) => {
        const top = beside(tl(painter))(tr(painter));
        const bottom = beside(bl(painter))(br(painter));
        return below(bottom)(top);
    };
}
function square_limit(painter, n) {
    const combine4 = square_of_four(flip_horiz, identity, rotate180, flip_vert);
    return combine4(corner_split(painter, n));
}
const flip_vert_painter = flip_vert(painter);
const flip_horiz_painter = flip_horiz(painter);
const shrink_to_upper_right_painter = shrink_to_upper_right(painter);
const rotate90_painter = rotate90(painter);
const rotate180_painter = rotate180(painter);
const rotate270_painter = rotate270(painter);
const squash_inwards_painter = squash_inwards(painter);
const beside_painter = beside(painter)(painter);
// const below_painter = below(painter)(painter);
const below_painter = W(below)(painter);
const wave2 = beside(painter)(flip_vert(painter));
const wave4 = below(wave2)(wave2);
const r_split = right_split(painter, 3);
const u_split = up_split(painter, 1);
const c_split = corner_split(painter, 4);
const s_split = square_split(painter, 4);
const s_limit = square_limit(painter, 2);
const make_fractal = (self) => (p) => (depth) => {
    if (depth === 0)
        return p;
    // 🪄 IL TRUCCO DEL MOCKINGBIRD:
    // M(self) rigenera la funzione ricorsiva al volo!
    const smaller = M(self)(p)(depth - 1);
    // Mettiamo il pittore p a sinistra, e a destra mettiamo p sotto il livello ricorsivo
    return beside(p)(below(p)(smaller));
};
// const fractal = M(make_fractal)
// fractal(painter)(2)(frame1)
// beside(painter)(flip_vert(painter))
// R(flip_vert(painter))(beside)(painter)(frame1)
//
// flip_vert(painter)(frame1)
// T(painter)(flip_vert)(frame1)
F(flip_horiz(painter))(rotate90(painter))(beside)(frame1);
// flip_vert_painter(w)
// const besideFlipped = C(beside)
// beside(painter)(right_split(painter, 2))(frame1)
// besideFlipped(painter)(right_split(painter, 2))(frame1)
// const result = C(B)(rotate90)(flip_vert);
// result(painter)(frame1);
// const rotate_270 = B(B(flip_horiz)(flip_vert))(rotate90);
// const new_transform = B(rotate90)(beside(painter)(painter));
// beside(rotate_180(painter), rotate_270(painter))(frame1)
// rotate_270(painter)(frame1)
// rotate180_painter(frame1)
// s_limit(frame1);
// s_split(frame1);
// c_split(frame1)
// u_split(frame1);
// r_split(frame1);
// wave4(frame1);
// === below_painter(painter, painter)
// const beside_painter2 = rotate270(beside(rotate90(painter), rotate90(painter)));
// const below_painter2 = below2(painter, painter)
// painter(frame1)
// flip_horiz_painter(frame1)
// flip_painter(frame1);
// shrink_to_upper_right_painter(frame1);
// painter(frame1);
// rotate270_painter(frame1);
// squash_inwards_painter(frame1);
// beside_painter(frame1);
// const v1 = make_vector(0, 0);
// const v2 = make_vector(100, 100);
// draw_line(ctx, v1, v2);
// const v = make_vector(1, 2);
// print_list(xcor_vect(v));
// print_list(ycor_vect(v));
// async function run() {
//   const img = await loadImage("./foto.jpeg");
//   const imgPainter = image_to_painter(ctx, img);
//   // const wave2 = beside(painter, flip_vert(painter));
//   const wave2 = below(painter, flip_horiz(imgPainter));
//   wave2(frame1);
//   // const escher_style = square_limit(imgPainter, 2);
//   // escher_style(frame1);
// }
// run();
