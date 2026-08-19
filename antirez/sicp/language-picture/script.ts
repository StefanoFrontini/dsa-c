const canvas = document.getElementById("canvas") as HTMLCanvasElement | null;
if (!canvas) throw new Error("canvas is null");

const ctx = canvas.getContext("2d");
if (!ctx) throw new Error("ctx is null");

// console.log(ctx);

const CANVAS_WIDTH = (canvas.width = 600);
const CANVAS_HEIGHT = (canvas.height = 600);

type Pair = {
  head: number | Pair | null;
  tail: number | Pair | null;
};

// cons
function pair(a: Pair["head"], b: Pair["tail"]): Pair {
  return {
    head: a,
    tail: b,
  };
}

// car
function head(p: number | Pair | null): number | Pair | null {
  return typeof p === "number" || p === null ? p : p.head;
}

// cdr
function tail(p: number | Pair | null): number | Pair | null {
  return typeof p === "number" || p === null ? p : p.tail;
}

const x = pair(1, 2);
const y = pair(3, 4);
const z = pair(x, y);

// console.log(head(x));
// console.log(tail(x));

// console.log(head(head(z)));

function list(...args: (Pair | number | null)[]) {
  // console.log("args: ", args);
  function list_rec(
    acc: Pair | null,
    ...args_rec: (Pair | number | null)[]
  ): Pair | null {
    if (args_rec.length === 0) return acc;
    // console.log("args_rec[0]: ", args_rec[0]);
    // console.log("...args_rec.slice(1):", ...args_rec.slice(1));
    return pair(args_rec[0], list_rec(acc, ...args_rec.slice(1)));
  }
  return list_rec(null, ...args);
}

function print_list(l: Pair | number | null) {
  if (typeof l === "number" || l === null) {
    console.log(l);
    return;
  }
  function print_list_rec(
    l: Pair | number | null,
    acc: (Pair | number | null)[],
  ) {
    if (l === null) return acc;
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
function list_ref(
  items: Pair | number | null,
  n: number,
): Pair | number | null {
  return n === 0 ? head(items) : list_ref(tail(items), n - 1);
}

function is_null(l: Pair | number | null) {
  return l === null;
}

function len(items: Pair | number | null): number {
  return is_null(items) ? 0 : 1 + len(tail(items));
}

function append(
  list1: Pair | number | null,
  list2: Pair | number | null,
): Pair | number | null {
  return is_null(list1) ? list2 : pair(head(list1), append(tail(list1), list2));
}
// print_list(len(l));
// print_list(append(l, l2));

function map<T extends number | Pair | null>(
  fun: (a: T) => T,
  items: Pair | number | null,
): Pair | number | null {
  return is_null(items)
    ? null
    : pair(fun(head(items) as T), map(fun, tail(items)));
}

function for_each<T extends number | Pair | null>(
  fun: (a: T) => void,
  items: Pair | number | null,
): void {
  if (is_null(items)) return;
  fun(head(items) as T);
  for_each(fun, tail(items));
}

// const a = pair(list(1, 2), list(3, 4))

// print_list(len(list(a, a)));

// for_each(print_list, list(10, 20, 30));
// for_each((x:number) => x * 2, l);
// print_list(map(((x:number) => x * x), l));

function make_vector(x: number, y: number): Pair {
  return pair(x, y);
}

function xcor_vect(vector: Pair): number {
  return head(vector) as number;
}

function ycor_vect(vector: Pair): number {
  return tail(vector) as number;
}

function add_vect(vector1: Pair, vector2: Pair): Pair {
  return make_vector(
    xcor_vect(vector1) + xcor_vect(vector2),
    ycor_vect(vector1) + ycor_vect(vector2),
  );
}

function sub_vect(vector1: Pair, vector2: Pair): Pair {
  return make_vector(
    xcor_vect(vector1) - xcor_vect(vector2),
    ycor_vect(vector1) - ycor_vect(vector2),
  );
}

function scale_vect(f: number, vector: Pair): Pair {
  return make_vector(f * xcor_vect(vector), f * ycor_vect(vector));
}

function make_segment(vector1: Pair, vector2: Pair): Pair {
  return pair(vector1, vector2);
}

function start_segment(segment: Pair): Pair {
  return head(segment) as Pair;
}

function end_segment(segment: Pair): Pair {
  return tail(segment) as Pair;
}

function make_frame(origin: Pair, edge1: Pair, edge2: Pair): Pair {
  return list(origin, edge1, edge2) as Pair;
}

function origin_frame(frame: Pair): Pair {
  return list_ref(frame, 0) as Pair;
}

function edge1_frame(frame: Pair): Pair {
  return list_ref(frame, 1) as Pair;
}

function edge2_frame(frame: Pair): Pair {
  return list_ref(frame, 2) as Pair;
}

function draw_line(
  ctx: CanvasRenderingContext2D | null,
  vector1: Pair,
  vector2: Pair,
): void {
  if (!ctx) return;
  ctx.beginPath(); // Start a new path
  ctx.moveTo(xcor_vect(vector1), ycor_vect(vector1)); // Move the pen to vector1
  ctx.lineTo(xcor_vect(vector2), ycor_vect(vector2)); // Draw a line to vector2
  ctx.stroke(); // Render the path
}

function frame_coord_map(frame: Pair): (v: Pair) => Pair {
  return (v: Pair) =>
    add_vect(
      origin_frame(frame),
      add_vect(
        scale_vect(xcor_vect(v), edge1_frame(frame)),
        scale_vect(ycor_vect(v), edge2_frame(frame)),
      ),
    );
}

function segments_to_painter(segment_list: Pair | null) {
  return (ctx: CanvasRenderingContext2D | null, frame: Pair) =>
    for_each(
      (segment: Pair) =>
        draw_line(
          ctx,
          frame_coord_map(frame)(start_segment(segment)),
          frame_coord_map(frame)(end_segment(segment)),
        ),
      segment_list,
    );
}

const p1 = make_vector(0.25, 1 - 0);
const p2 = make_vector(0.35, 1 - 0.5);
const p3 = make_vector(0.3, 1 -0.6);
const p4 = make_vector(0.15, 1-0.4);
const p5 = make_vector(0, 1-0.65);
const p6 = make_vector(0.4, 1-0);
const p7 = make_vector(0.5, 1-0.3);
const p8 = make_vector(0.6, 1-0);
const p9 = make_vector(0.75, 1-0);
const p10 = make_vector(0.6, 1-0.45);
const p11 = make_vector(1, 1-0.15);
const p12 = make_vector(1, 1-0.35);
const p13 = make_vector(0.75, 1-0.65);
const p14 = make_vector(0.6, 1-0.65);
const p15 = make_vector(0.65, 1-0.85);
const p16 = make_vector(0.6, 1-1);
const p17 = make_vector(0.4, 1-1);
const p18 = make_vector(0.35, 1-0.85);
const p19 = make_vector(0.4, 1-0.65);
const p20 = make_vector(0.3, 1-0.65);
const p21 = make_vector(0.15, 1-0.6);
const p22 = make_vector(0, 1-0.85);


const george_lines = list(
  make_segment(p1, p2),
  make_segment(p2, p3),
  make_segment(p3, p4),
  make_segment(p4, p5),
  make_segment(p6, p7),
  make_segment(p7, p8),
  make_segment(p9, p10),
  make_segment(p10, p11),
  make_segment(p12, p13),
  make_segment(p13, p14),
  make_segment(p14, p15),
  make_segment(p15, p16),
  make_segment(p17, p18),
  make_segment(p18, p19),
  make_segment(p19, p20),
  make_segment(p20, p21),
  make_segment(p21, p22),
);

const pic = segments_to_painter(george_lines);
const frame1 = make_frame(
  make_vector(0, 0),
  make_vector(600, 0),
  make_vector(0, 600),
);

pic(ctx, frame1);

// const v1 = make_vector(0, 0);
// const v2 = make_vector(100, 100);

// draw_line(ctx, v1, v2);

// const v = make_vector(1, 2);

// print_list(xcor_vect(v));
// print_list(ycor_vect(v));
