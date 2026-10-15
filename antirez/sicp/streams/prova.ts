import {
  stream_map,
  stream_enumerate_interval,
  stream_ref,
} from "./src/stream.ts";
const display = (v: number) => (console.log(v), v);
const x = stream_map(display, stream_enumerate_interval(0, 10));
console.log("---");
stream_ref(x, 5);
console.log("---");
stream_ref(x, 7);
