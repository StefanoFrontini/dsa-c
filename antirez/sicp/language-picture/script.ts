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
    return pair(
      args_rec[0],
      list_rec(acc, ...args_rec.slice(1)),
    );

  }
  return list_rec(null, ...args);
}

function print_list(l: Pair | number | null) {
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

const l = list(1, list(4, 5), 3);
// const l = list(1, 2, 3, 4);
console.log("l:", l);
console.log("print_list");
print_list(l);
