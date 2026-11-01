// Tappa 1 — una cella con la coda ritardata: funzione + ambiente al posto della lambda.
#include "../stream.h"
#include "test.h"

/* Una coda "spia": conta quante volte viene forzata e restituisce lo stream che le hai dato come env. */
static int calls;
static Stream *spy_tail(void *env) {
  calls++;
  return (Stream *)env;
}

static void cons_non_forza_la_coda(void) {
  calls = 0;
  Stream *s = cons_stream(1, spy_tail, THE_EMPTY_STREAM);
  CHECK_EQ(calls, 0, "cons_stream non deve chiamare tail_fn");
  CHECK_EQ(stream_head(s), 1, "la testa è quella passata a cons_stream");
  CHECK(stream_tail(s) == THE_EMPTY_STREAM, "la coda è quella calcolata da tail_fn");
  CHECK_EQ(calls, 1, "forzare la coda chiama tail_fn");
}

static void take_non_forza_oltre(void) {
  calls = 0;
  Stream *c = cons_stream(3, spy_tail, THE_EMPTY_STREAM);
  Stream *b = cons_stream(2, spy_tail, c);
  Stream *a = cons_stream(1, spy_tail, b);
  Value out[5];
  size_t n = take(a, 2, out);
  CHECK_VALUES(out, n, "i primi due", 1, 2);
  CHECK_EQ(calls, 1, "per leggere 2 elementi basta forzare 1 coda");
  n = take(a, 5, out);
  CHECK_VALUES(out, n, "meno valori se lo stream finisce", 1, 2, 3);
  CHECK_EQ(take(a, 0, out), 0, "take 0");
}

static void array_to_stream_prende_in_prestito(void) {
  const Value xs[] = {10, 20, 30};
  Value out[5];
  size_t n = take(array_to_stream(xs, 3), 5, out);
  CHECK_VALUES(out, n, "gli elementi dell'array", 10, 20, 30);
  CHECK(array_to_stream(xs, 0) == THE_EMPTY_STREAM, "array vuoto → stream vuoto");
}

static void integers_from_e_stream_ref(void) {
  Value out[5];
  size_t n = take(integers_from(1), 5, out);
  CHECK_VALUES(out, n, "integers_from(1)", 1, 2, 3, 4, 5);
  CHECK_EQ(stream_ref(integers_from(0), 10), 10, "stream_ref(integers_from(0), 10)");
}

static void enumerate_interval(void) {
  Value out[10];
  size_t n = take(stream_enumerate_interval(3, 7), 10, out);
  CHECK_VALUES(out, n, "da 3 a 7", 3, 4, 5, 6, 7);
  CHECK(stream_enumerate_interval(5, 4) == THE_EMPTY_STREAM, "low > high → stream vuoto");
}

static void un_milione_di_elementi(void) {
  /* In C non c'è l'ottimizzazione delle chiamate in coda (non a -O0): una take o una stream_ref
   * ricorsive qui finiscono lo stack (8 MB). Servono dei cicli. */
  static Value out[1000000];
  size_t n = take(integers_from(0), 1000000, out);
  CHECK_EQ(n, 1000000, "un milione di valori");
  CHECK_EQ(out[999999], 999999, "l'ultimo");
  CHECK_EQ(stream_ref(integers_from(0), 1000000), 1000000, "stream_ref lontano");
}

int main(void) {
  printf("tappa 1 · basi\n");
  RUN(cons_non_forza_la_coda, "cons_stream non forza la coda");
  RUN(take_non_forza_oltre, "take non forza oltre l'ultimo elemento richiesto");
  RUN(array_to_stream_prende_in_prestito, "array_to_stream");
  RUN(integers_from_e_stream_ref, "integers_from e stream_ref");
  RUN(enumerate_interval, "stream_enumerate_interval");
  RUN(un_milione_di_elementi, "un milione di elementi a stack costante");
  return test_summary("tappa 1");
}
