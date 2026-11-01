// Tappa 2 — memoizzazione: la coda si calcola una volta sola, anche quando il risultato è lo stream vuoto.
#include "../stream.h"
#include "test.h"

static int calls;
static Stream *spy_tail(void *env) {
  calls++;
  return (Stream *)env;
}

/* 0, 1, 2, ... come integers_from, ma conta le code calcolate. env punta al prossimo valore. */
static Stream *counting_tail(void *env) {
  calls++;
  Value *next = malloc(sizeof *next);
  *next = *(Value *)env + 1;
  return cons_stream(*(Value *)env, counting_tail, next);
}
static Stream *counting_from(Value n) {
  Value *next = malloc(sizeof *next);
  *next = n + 1;
  return cons_stream(n, counting_tail, next);
}

static void la_coda_si_calcola_una_volta(void) {
  calls = 0;
  Stream *b = cons_stream(2, spy_tail, THE_EMPTY_STREAM);
  Stream *a = cons_stream(1, spy_tail, b);
  Stream *t1 = stream_tail(a);
  Stream *t2 = stream_tail(a);
  CHECK_EQ(calls, 1, "forzare due volte chiama tail_fn una volta");
  CHECK(t1 == t2 && t1 == b, "e restituisce sempre la stessa cella");
}

static void anche_la_coda_vuota(void) {
  /* NULL è un risultato valido della coda: non può voler dire anche "non ancora calcolata". */
  calls = 0;
  Stream *s = cons_stream(1, spy_tail, THE_EMPTY_STREAM);
  stream_tail(s);
  stream_tail(s);
  CHECK_EQ(calls, 1, "una coda vuota si memoizza come le altre");
}

static void rileggere_non_ricalcola(void) {
  calls = 0;
  Stream *s = counting_from(0);
  Value out[5];
  size_t n = take(s, 5, out);
  CHECK_VALUES(out, n, "i primi 5", 0, 1, 2, 3, 4);
  CHECK_EQ(calls, 4, "4 code per 5 elementi");
  take(s, 5, out);
  CHECK_EQ(calls, 4, "rileggere non ricalcola");
  CHECK(stream_tail(integers_from(0)) != THE_EMPTY_STREAM, "integers_from funziona ancora");
  Stream *ints = integers_from(0);
  CHECK(stream_tail(ints) == stream_tail(ints), "anche integers_from restituisce sempre la stessa coda");
}

/* `const ones = cons_stream(1, () => ones)`: la lambda catturava la *variabile* ones.
 * In C catturare una variabile vuol dire passare il suo indirizzo. */
static Stream *ones_tail(void *env) { return *(Stream **)env; }

static void ones_implicito(void) {
  Stream *ones;
  ones = cons_stream(1, ones_tail, &ones);
  Value out[5];
  size_t n = take(ones, 5, out);
  CHECK_VALUES(out, n, "uno stream infinito con una cella sola", 1, 1, 1, 1, 1);
  CHECK(stream_tail(ones) == ones, "la coda è la cella stessa");
}

int main(void) {
  printf("tappa 2 · memo\n");
  RUN(la_coda_si_calcola_una_volta, "la coda si calcola una volta sola");
  RUN(anche_la_coda_vuota, "anche la coda vuota si memoizza");
  RUN(rileggere_non_ricalcola, "rileggere non ricalcola");
  RUN(ones_implicito, "ones implicito: catturare una variabile in C");
  return test_summary("tappa 2");
}
