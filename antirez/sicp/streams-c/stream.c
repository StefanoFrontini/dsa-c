/* SICP JS 3.5 — stream in C. I corpi li scrivi tu, una tappa alla volta
 * (README.md). Per ora la memoria non si libera mai: chi possiede le celle è la
 * domanda della tappa 4. */

#include "stream.h"

#include <stdio.h>
#include <stdlib.h>

#define TODO(tappa)                                                            \
  do {                                                                         \
    fprintf(stderr, "TODO tappa %d: %s da implementare\n", tappa, __func__);   \
    abort();                                                                   \
  } while (0)

static void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}
/* -------------------------------------------------------------------------
 * Tappa 1 — una cella con la coda ritardata
 * ------------------------------------------------------------------------- */

Stream *cons_stream(Value head, TailFn tail_fn, void *env) {
  // (void)head, (void)tail_fn, (void)env;
  Stream *s = xmalloc(sizeof(Stream));
  s->head = head;
  s->tail_fn = tail_fn;
  s->env = env;
  return s;
  // TODO(1);
}

Value stream_head(Stream *s) {
  // (void)s;
  // TODO(1);
  if (s == THE_EMPTY_STREAM) {
    fprintf(stderr, "stream is empty\n");
    abort();
  }
  return s->head;
}

Stream *stream_tail(Stream *s) {
  // (void)s;
  // TODO(1);
  if (s == THE_EMPTY_STREAM) {
    fprintf(stderr, "stream is empty\n");
    abort();
  }
  return s->tail_fn(s->env);
}

Value stream_ref(Stream *s, size_t n) {
  // (void)s, (void)n;
  // TODO(1);
  while (n--) {
    s = stream_tail(s);
  }
  return stream_head(s);
  // return n == 0 ? stream_head(s) : stream_ref(stream_tail(s), n - 1);
}

size_t take(Stream *s, size_t n, Value *out) {
  // (void)s, (void)n, (void)out;
  // TODO(1);
  size_t i = 0;
  for (; i < n; i++) {
    if (s == THE_EMPTY_STREAM) break;
    out[i] = stream_head(s);
    if (i < n - 1) {
      s = stream_tail(s);
    }
  }
  return i;
}
static Stream *integers_tail(void *env) {
  Value *v = (Value *)env;
  return integers_from(*v);
}

Stream *integers_from(Value n) {
  Value *next = xmalloc(sizeof(Value));
  *next = n + 1;
  return cons_stream(n, integers_tail, next);
}

struct Env {
  Value low;
  Value high;
};

static Stream *enum_tail(void *env) {
  struct Env *e = (struct Env *)env;
  return stream_enumerate_interval(e->low, e->high);
}

Stream *stream_enumerate_interval(Value low, Value high) {
  if (low > high) return THE_EMPTY_STREAM;
  struct Env *env = xmalloc(sizeof(struct Env));
  env->low = low + 1;
  env->high = high;
  return cons_stream(low, enum_tail, env);
}

struct EnvArr {
  const Value *next;
  const Value *end;
};
static Stream *iter(const Value *xs, const Value *end);

static Stream *array_tail(void *env) {
  struct EnvArr *e = (struct EnvArr *)env;
  return iter(e->next, e->end);
}

static Stream *iter(const Value *xs, const Value *end) {
  if (xs >= end) return THE_EMPTY_STREAM;
  struct EnvArr *e = xmalloc(sizeof(struct EnvArr));
  Value v = xs[0];
  e->next = xs + 1;
  e->end = end;
  return cons_stream(v, array_tail, e);
}

Stream *array_to_stream(const Value *xs, size_t n) {
  const Value *end = xs + n;
  return iter(xs, end);
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
