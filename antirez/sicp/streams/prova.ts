import {
  stream_map,
  stream_enumerate_interval,
  stream_ref,
  stream_filter,
  integers_from
} from "./src/stream.ts";
stream_filter(x => false, integers_from(1))
// const display = (v: number) => (console.log(v), v);
// const x = stream_map(display, stream_enumerate_interval(0, 10));
// console.log("---");
// stream_ref(x, 5);
// console.log("---");
// stream_ref(x, 7);
