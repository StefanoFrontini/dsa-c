/* SICP JS 3.5 — stream in C.
 * Le firme sono fissate (i test le usano), i corpi li scrivi tu in stream.c,
 * una tappa alla volta. Percorso e domande: README.md. Test: `make test` oppure
 * `make t1` (solo la tappa 1).
 *
 * In TypeScript la coda era una lambda: `() => integers_from(n + 1)`. In C non
 * ci sono closure, quindi la coda diventa due cose: una funzione (TailFn) e il
 * suo ambiente (env), cioè i "parametri catturati" messi a mano in una struct
 * sull'heap. */

#ifndef STREAM_H
#define STREAM_H

#include <stdbool.h>
#include <stddef.h>

/* Il tipo degli elementi: double basta per gli interi esatti fino a 2^53 e per
 * i campioni audio. */
typedef double Value;

typedef struct Stream Stream;

/* Calcola la coda a partire dal suo ambiente: è il corpo della lambda della
 * coda. */
typedef Stream *(*TailFn)(void *env);

struct Stream {
  Value head;
  // Stream *(*tail_fn)(void *env);
  TailFn tail_fn; /* la coda ritardata: funzione... */
  void *env; /* ...più il suo ambiente */
  /* Tappa 2: aggiungi qui i campi che servono alla memoizzazione. */
};

/* Lo stream vuoto è il puntatore NULL. */
#define THE_EMPTY_STREAM ((Stream *)NULL)

/* -------------------------------------------------------------------------
 * Tappa 1 — una cella con la coda ritardata
 * ------------------------------------------------------------------------- */

/* Costruisce una cella. NON chiama tail_fn: la coda si calcola solo quando
 * qualcuno la forza. */
Stream *cons_stream(Value head, TailFn tail_fn, void *env);

Value stream_head(Stream *s);

/* Forza la coda: è qui che il calcolo ritardato viene eseguito. (Tappa 2: una
 * volta sola.) */
Stream *stream_tail(Stream *s);

/* L'elemento di indice n (0 = la testa). */
Value stream_ref(Stream *s, size_t n);

/* Copia i primi n elementi in out (che ha posto per almeno n valori) e
 * restituisce quanti ne ha copiati (meno di n se lo stream finisce prima). Non
 * forza più code del necessario. */
size_t take(Stream *s, size_t n, Value *out);

/* n, n+1, n+2, ... all'infinito. */
Stream *integers_from(Value n);

/* low, low+1, ..., high (vuoto se low > high). */
Stream *stream_enumerate_interval(Value low, Value high);

/* Gli elementi di xs[0..n-1]. Lo stream *prende in prestito* xs: l'array deve
 * vivere quanto lo stream. */
Stream *array_to_stream(const Value *xs, size_t n);

/* -------------------------------------------------------------------------
 * Tappa 2 — memoizzazione: nessuna funzione nuova, cambia stream_tail (e la
 * struct qui sopra).
 * ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * Tappa 3 — map, filter, operazioni
 * In TS `x => x * factor` catturava factor; in C la funzione riceve un contesto
 * `ctx` scelto da chi chiama.
 * ------------------------------------------------------------------------- */

typedef Value (*MapFn)(Value x, void *ctx);
typedef Value (*MapFn2)(Value x, Value y, void *ctx);
typedef bool (*Pred)(Value x, void *ctx);

/* f(x0, ctx), f(x1, ctx), ... Come in SICP la testa si calcola subito, la coda
 * solo quando serve. */
Stream *stream_map(MapFn f, void *ctx, Stream *s);

/* f(a0, b0, ctx), f(a1, b1, ctx), ... Finisce quando finisce il più corto. */
Stream *stream_map2(MapFn2 f, void *ctx, Stream *a, Stream *b);

/* Gli elementi per cui pred è vero. Stack costante anche se scarta milioni di
 * elementi di fila; pred si chiama una volta sola per elemento. */
Stream *stream_filter(Pred pred, void *ctx, Stream *s);

Stream *add_streams(Stream *a, Stream *b);
Stream *mul_streams(Stream *a, Stream *b);
Stream *scale_stream(Stream *s, Value factor);

#endif
