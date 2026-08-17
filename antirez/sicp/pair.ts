export function pair(x: number, y: number) {
  function dispatch(m: number) {
    return m === 0 ? x : m === 1 ? y : 1;
  }
  return dispatch;
}

// function head(z: (m: number) => number) {
//   return z(0);
// }

// function tail(z: (m: number) => number) {
//   return z(1);
// }

// const p = pair(2, 3);
// console.log(head(p));
// console.log(tail(p));

// type P = {
//   head: number | ((m: number) => number);
//   tail: number | ((m: number) => number);
// };
// function pair3(
//   a: number | ((m: number) => number),
//   b: number | ((m: number) => number),
// ): P {
//   return {
//     head: a,
//     tail: b,
//   };
// }

// const p3 = pair3(2, pair(1, 3));

// function head3(p: P) {
//   return p.head;
// }
// function tail3(p: P) {
//   return p.tail;
// }
// console.log(head3(p3));
// console.log(tail3(p3));

// function pair2(x: number, y: number){
//     function a(m: (x: number, y:number) => number){
//         return m(x,y);
//     }
//     return a;
// }
// const p2 = pair2(4, 5);
// function head2(z: (p: number, q:number) => number): number {
//     return z((a: number, b: number) => p);
// }
// function tail2(z){
//     return z((p,q) => q);
// }
// console.log(head2(p2));
// console.log(tail2(p2));
