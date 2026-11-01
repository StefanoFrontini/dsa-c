// Tappa 3 — map, filter e operazioni: le funzioni ricevono un contesto al posto delle variabili catturate.
#include "../stream.h"
#include "test.h"

static int calls;

static Value times(Value x, void *ctx) {
  calls++;
  return x * *(Value *)ctx;
}
static Value plus(Value x, Value y, void *ctx) {
  (void)ctx;
  return x + y;
}
static bool is_even(Value x, void *ctx) {
  (void)ctx;
  calls++;
  return fmod(x, 2) == 0;
}
static bool multiple_of(Value x, void *ctx) { return fmod(x, *(Value *)ctx) == 0; }
static bool greater_than(Value x, void *ctx) { return x > *(Value *)ctx; }

static void map_con_contesto(void) {
  calls = 0;
  Value three = 3;
  Stream *s = stream_map(times, &three, integers_from(1));
  CHECK_EQ(calls, 1, "come in SICP la testa si calcola subito");
  Value out[3];
  size_t n = take(s, 3, out);
  CHECK_VALUES(out, n, "1·3, 2·3, 3·3", 3, 6, 9);
  CHECK_EQ(calls, 3, "f una volta per elemento letto");
  take(s, 3, out);
  CHECK_EQ(calls, 3, "rileggere non richiama f");
}

static void map2_finisce_col_piu_corto(void) {
  const Value xs[] = {10, 20, 30};
  Value out[10];
  size_t n = take(stream_map2(plus, NULL, integers_from(1), array_to_stream(xs, 3)), 10, out);
  CHECK_VALUES(out, n, "1+10, 2+20, 3+30", 11, 22, 33);
  CHECK(stream_map2(plus, NULL, integers_from(1), THE_EMPTY_STREAM) == THE_EMPTY_STREAM, "con uno stream vuoto");
}

static Stream *scaled_ints(void) {
  /* Quando la coda verrà forzata, questa funzione (e scale_stream) avranno già restituito il controllo. */
  return scale_stream(integers_from(1), 2.5);
}

static void operazioni(void) {
  Value out[4];
  size_t n = take(add_streams(integers_from(1), integers_from(10)), 4, out);
  CHECK_VALUES(out, n, "add_streams", 11, 13, 15, 17);
  n = take(mul_streams(integers_from(1), integers_from(1)), 4, out);
  CHECK_VALUES(out, n, "mul_streams", 1, 4, 9, 16);
  n = take(scaled_ints(), 4, out);
  CHECK_VALUES(out, n, "scale_stream: il fattore deve vivere quanto lo stream", 2.5, 5, 7.5, 10);
}

static void filter(void) {
  calls = 0;
  Stream *evens = stream_filter(is_even, NULL, integers_from(1));
  Value out[4];
  size_t n = take(evens, 3, out);
  CHECK_VALUES(out, n, "i pari", 2, 4, 6);
  CHECK_EQ(calls, 6, "il predicato una volta per elemento esaminato (da 1 a 6)");
  Value ten = 10;
  const Value xs[] = {1, 2, 3};
  CHECK(stream_filter(greater_than, &ten, array_to_stream(xs, 3)) == THE_EMPTY_STREAM,
        "nessun elemento buono in uno stream finito → stream vuoto");
}

static void filter_a_stack_costante(void) {
  Value million = 1000000;
  CHECK_EQ(stream_head(stream_filter(multiple_of, &million, integers_from(1))), 1000000,
           "scarta 999.999 elementi di fila senza finire lo stack");
}

/* fibs = 0, 1, fibs + coda di fibs: tre celle implicite che si riferiscono alla variabile fibs. */
static Stream *fibs_rest(void *env) {
  Stream *fibs = *(Stream **)env;
  return add_streams(stream_tail(fibs), fibs);
}
static Stream *fibs_second(void *env) { return cons_stream(1, fibs_rest, env); }

static void fibs_implicito(void) {
  Stream *fibs;
  fibs = cons_stream(0, fibs_second, &fibs);
  Value out[10];
  size_t n = take(fibs, 10, out);
  CHECK_VALUES(out, n, "Fibonacci", 0, 1, 1, 2, 3, 5, 8, 13, 21, 34);
  CHECK_EQ(stream_ref(fibs, 78), 8944394323791464.0, "F(78): lineare grazie alla memo");
}

int main(void) {
  printf("tappa 3 · operazioni\n");
  RUN(map_con_contesto, "stream_map con un contesto");
  RUN(map2_finisce_col_piu_corto, "stream_map2 finisce con il più corto");
  RUN(operazioni, "add_streams, mul_streams, scale_stream");
  RUN(filter, "stream_filter");
  RUN(filter_a_stack_costante, "stream_filter a stack costante");
  RUN(fibs_implicito, "fibs implicito");
  return test_summary("tappa 3");
}
