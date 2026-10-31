# SICP 3.5 Streams in TypeScript (percorso a tappe)  — stato: working
Path: `sicp/streams/src/stream.ts`, `sicp/streams/src/generators.ts`, `sicp/streams/src/animation.ts`, `sicp/streams/test/` · Percorso: `sicp/streams/README.md` · Ultima modifica: 2026-10-31

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
- 9 tappe: 54/54 test (`npm test`), typecheck senza errori (2026-10-31).

## Cosa funziona
- Tappa 1: `cons_stream` con coda ritardata, `take` che non forza oltre l'ultimo elemento, `array_to_stream`, `integers_from`.
- Tappa 2: `memo` con flag separato dal valore, `cons_stream` memoizzato.
- Tappa 3: `stream_map` su più stream, `stream_filter` iterativo (stack costante, un test del predicato per elemento), `add_streams`/`mul_streams`/`scale_stream`.
- Tappa 4: `ones`, `integers`, `fibs`, `sieve`, `primes`/`is_prime`, `merge` + `hamming`, `factorials`.
- Tappa 5: `partial_sums` implicito e lineare, `sqrt_stream`, `pi_stream`, `euler_transform`, `make_tableau`/`accelerated_sequence`, `integrate_series`, `exp_series`, `sine_series`/`cosine_series`.
- Tappa 6: `integral` con l'integrando come thunk, `solve` (Eulero; `solve(y => y, 1, 0.001)` ≈ e).
- Tappa 7: `sign_change_detector`, `zero_crossings` con lo 0 davanti, `smooth`, `smoothed_zero_crossings` come composizione.
- Tappa 8 (`src/generators.ts`): `integers_from_gen`, `take_gen` (controlla `done`, non chiede valori in più), `map_gen`/`filter_gen` come cicli, i ponti `stream_to_gen`/`gen_to_stream` (memoizzato, un `next()` per valore), `sieve_gen`.
- Tappa 9 (`src/animation.ts`): `iterate` (la `f` sta nella coda: un passo per livello, niente calcoli in anticipo), `cycle` circolare con k celle (la coda dell'ultima torna alla prima), `play` con `requestAnimationFrame` come orologio (indice assoluto dal primo frame, elementi in ritardo attraversati ma non disegnati, avanzamento con un ciclo, `stop`). Colla con il browser in `sicp/language-picture/animation.ts`: livelli di `right_split` con `iterate`, andata e ritorno 0…5…1 con `take` + `cycle`, `play` con `requestAnimationFrame`; verificata nel browser (`animation.html`, build `tsc -p tsconfig.anim.json`).

## Cosa manca
- Esercizio aperto: stream in C.
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
- [Generatori JS: stream senza memoria](../notes/sicp-streams.md#generatori-js-stream-senza-memoria)
- [Animazione: lo stream dice cosa, l'orologio dice quando](../notes/sicp-streams.md#animazione-lo-stream-dice-cosa-lorologio-dice-quando)

## Prossimo passo consigliato
L'esercizio aperto **stream in C** (verso il player audio): coda come puntatore a funzione più ambiente, memo nella cella, e la domanda vera, chi possiede le celle.
