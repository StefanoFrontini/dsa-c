export type Pair = {
  head: number | Pair | null;
  tail: number | Pair | null | (() => number | Pair | null);
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

type Stream = {
  head: number;
  tail: () => Stream | null;
};

function make_stream(h: number, t: Stream): Stream {
  return {
    head: h,
    tail: () => t,
  };
}

function stream_tail(s: Stream): Stream | null {
  return s.tail();
}

function stream_head(s: Stream): number {
  return s.head;
}

function stream_ref(s: Stream, n: number): number | null {
  return n === 0
    ? stream_head(s)
    : stream_tail(s)
      ? stream_ref(stream_tail(s) as Stream, n - 1)
      : null;
}
