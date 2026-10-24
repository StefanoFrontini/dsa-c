# SICP 3.5 Streams in TypeScript (percorso a tappe)  — stato: working
Path: `sicp/streams/src/stream.ts`, `sicp/streams/test/` · Percorso: `sicp/streams/README.md` · Ultima modifica: 2026-10-24

## Cosa fa
Implementazione degli stream del capitolo 3.5 di SICP (edizione JS) a tappe, con le firme fissate in
`sicp/streams/src/stream.ts` e un file di test per tappa in `sicp/streams/test/`.
Concetti e domande di autoverifica (con le risposte) nell'area [Stream (SICP 3.5)](../notes/sicp-streams.md).

## Come si compila / si esegue
```sh
cd antirez/sicp/streams
node --test test/01-basi.test.ts     # una tappa alla volta (Node 24 esegue i .ts)
npm test                             # tutte le tappe
npm run typecheck                    # tsc --noEmit
```

## Esito verifica
- 7 tappe: 37/37 test (`npm test`), typecheck senza errori (2026-10-24).

## Cosa funziona
- Tappa 1: `cons_stream` con coda ritardata, `take` che non forza oltre l'ultimo elemento, `array_to_stream`, `integers_from`.
- Tappa 2: `memo` con flag separato dal valore, `cons_stream` memoizzato.
- Tappa 3: `stream_map` su più stream, `stream_filter` iterativo (stack costante, un test del predicato per elemento), `add_streams`/`mul_streams`/`scale_stream`.
- Tappa 4: `ones`, `integers`, `fibs`, `sieve`, `primes`/`is_prime`, `merge` + `hamming`, `factorials`.
- Tappa 5: `partial_sums` implicito e lineare, `sqrt_stream`, `pi_stream`, `euler_transform`, `make_tableau`/`accelerated_sequence`, `integrate_series`, `exp_series`, `sine_series`/`cosine_series`.
- Tappa 6: `integral` con l'integrando come thunk, `solve` (Eulero; `solve(y => y, 1, 0.001)` ≈ e).
- Tappa 7: `sign_change_detector`, `zero_crossings` con lo 0 davanti, `smooth`, `smoothed_zero_crossings` come composizione.

## Cosa manca
- Esercizi aperti: animazione del picture language con uno stream, generatori JS, stream in C.
- La prima bozza `sicp/streams/streams.ts` (commentata) resta come storia: `make_stream(h, t)` riceveva la coda già calcolata e andava in stack overflow.
- `sicp/pair.ts` (commentato): esperimenti di coppia procedurale (SICP 2.1.3, es. 2.4); `pair.ts:53` `head2` restituisce `p` inesistente.

## Concetti applicati
- [Coda ritardata (thunk) e valutazione eager](../notes/sicp-streams.md#coda-ritardata-thunk-e-valutazione-eager)
- [Memoizzazione delle code e memoria](../notes/sicp-streams.md#memoizzazione-delle-code-e-memoria)
- [stream_map, stream_filter e la pigrizia](../notes/sicp-streams.md#stream_map-stream_filter-e-la-pigrizia)
- [Stream infiniti e definizioni implicite](../notes/sicp-streams.md#stream-infiniti-e-definizioni-implicite)
- [Sequenze di approssimazioni e acceleratori](../notes/sicp-streams.md#sequenze-di-approssimazioni-e-acceleratori)
- [Serie di potenze come stream di coefficienti](../notes/sicp-streams.md#serie-di-potenze-come-stream-di-coefficienti)
- [Ritardo esplicito: integral e solve](../notes/sicp-streams.md#ritardo-esplicito-integral-e-solve)
- [Segnali come stream: zero crossing e smooth](../notes/sicp-streams.md#segnali-come-stream-zero-crossing-e-smooth)

## Prossimo passo consigliato
Uno dei tre esercizi aperti in `sicp/streams/README.md`: animazione del picture language, generatori JS, **stream in C** (verso il player audio).
