/* SICP JS 3.5 — stream in C. I corpi li scrivi tu, una tappa alla volta (README.md).
 * Per ora la memoria non si libera mai: chi possiede le celle è la domanda della tappa 4. */

#include "stream.h"

#include <stdio.h>
#include <stdlib.h>

#define TODO(tappa)                                                            \
  do {                                                                         \
    fprintf(stderr, "TODO tappa %d: %s da implementare\n", tappa, __func__);  \
    abort();                                                                   \
  } while (0)

/* -------------------------------------------------------------------------
 * Tappa 1 — una cella con la coda ritardata
 * ------------------------------------------------------------------------- */

Stream *cons_stream(Value head, TailFn tail_fn, void *env) {
  (void)head, (void)tail_fn, (void)env;
  TODO(1);
}

Value stream_head(Stream *s) {
  (void)s;
  TODO(1);
}

Stream *stream_tail(Stream *s) {
  (void)s;
  TODO(1);
}

Value stream_ref(Stream *s, size_t n) {
  (void)s, (void)n;
  TODO(1);
}

size_t take(Stream *s, size_t n, Value *out) {
  (void)s, (void)n, (void)out;
  TODO(1);
}

Stream *integers_from(Value n) {
  (void)n;
  TODO(1);
}

Stream *stream_enumerate_interval(Value low, Value high) {
  (void)low, (void)high;
  TODO(1);
}

Stream *array_to_stream(const Value *xs, size_t n) {
  (void)xs, (void)n;
  TODO(1);
}

/* -------------------------------------------------------------------------
 * Tappa 3 — map, filter, operazioni
 * ------------------------------------------------------------------------- */

Stream *stream_map(MapFn f, void *ctx, Stream *s) {
  (void)f, (void)ctx, (void)s;
  TODO(3);
}

Stream *stream_map2(MapFn2 f, void *ctx, Stream *a, Stream *b) {
  (void)f, (void)ctx, (void)a, (void)b;
  TODO(3);
}

Stream *stream_filter(Pred pred, void *ctx, Stream *s) {
  (void)pred, (void)ctx, (void)s;
  TODO(3);
}

Stream *add_streams(Stream *a, Stream *b) {
  (void)a, (void)b;
  TODO(3);
}

Stream *mul_streams(Stream *a, Stream *b) {
  (void)a, (void)b;
  TODO(3);
}

Stream *scale_stream(Stream *s, Value factor) {
  (void)s, (void)factor;
  TODO(3);
}
