# SICP 3.5 Streams (bozza) + coppie procedurali  — stato: stub
Path: `sicp/streams/streams.ts`, `sicp/pair.ts` · Ultima modifica: streams.ts 2026-09-21 (`pic_lang_9`), pair.ts 2026-09-18 (`pic_lang_8`)

## Cosa fa
Per ora niente a runtime: entrambi i file sono **interamente commentati**.
- `sicp/pair.ts:1-59`: esperimenti su SICP 2.1.3 ("che cosa si intende per dati"): coppia come funzione `dispatch(m)` (`:1-18`), coppia come oggetto `{head, tail}` (`:20-43`), coppia come funzione che riceve un selettore (es. 2.4, `:45-59`).
- `sicp/streams/streams.ts:1-112`: copia di `pair/head/tail/list/map/for_each` dal picture language, con `Pair.tail` che può essere anche una funzione (`:3`).
- `sicp/streams/streams.ts:114-140`: prima bozza di stream: `type Stream = {head: number; tail: () => Stream | null}`, `make_stream`, `stream_head`, `stream_tail`, `stream_ref`.
Nella conversazione con Gemini gli stream non sono mai stati discussi (solo citati al turno 270).

## Come si compila / si esegue
`tsc --noEmit --strict streams/streams.ts pair.ts` (TypeScript 5.9.3 dalla build dir) → exit 0, ma solo perché non c'è codice attivo.

## Esito verifica
- Compila (vuoto). Nessun output.
- Ho riprodotto in Node la `make_stream` commentata (`streams.ts:119-124`) con `integers_from(n) = make_stream(n, integers_from(n+1))` → `RangeError: Maximum call stack size exceeded`. Motivo: `make_stream(h, t: Stream)` riceve la coda **già calcolata**; avvolgerla in `() => t` dopo non ritarda nulla. La coda va passata come *thunk* dal chiamante: `cons_stream(h, () => expr)`.
- Bug minore in `pair.ts:53` (commentato): `head2` restituisce `p`, che non esiste (doveva essere `a`).

## Cosa funziona
- L'idea di base è giusta: `tail` come funzione (`streams.ts:116`) e `stream_ref` ricorsivo (`streams.ts:134-140`).
- `pair.ts` mostra già la comprensione di "dati = procedure + contratto" (SICP 2.1.3), utile per capire `delay/force`.

## Cosa manca / bug noti
- Ritardo vero della coda (bug sopra), nessuna memoizzazione, nessun `stream_map/filter`, nessuno stream infinito, niente esempi.
- `Stream` limitato a `number` e non collegato al `Pair` tipizzato del picture language.

## Concetti applicati
- [pair/head/tail e liste](../notes/sicp-combinatori.md#pair--head--tail-e-liste-come-in-sicp-js) (stessa base del picture language)
- Valutazione ritardata / thunk: collegata alla discussione su eager vs lazy di M e Y in [Mockingbird e ricorsione anonima](../notes/sicp-combinatori.md#mockingbird-e-ricorsione-anonima-make_fractal-selfapplicable)

## Prossimo passo consigliato
Percorso a tappe con test automatici in `sicp/streams/README.md` (dal 2026-10-03):
le firme sono in `sicp/streams/src/stream.ts`, i test in `sicp/streams/test/` (`node --test test/01-basi.test.ts`).
Tappe: 1 basi e coda ritardata · 2 memo · 3 map/filter e pigrizia · 4 stream infiniti e definizioni implicite ·
5 somme parziali, Eulero, serie · 6 ritardo esplicito (`integral`, `solve`) · 7 segnali (zero crossing, ponte verso l'audio).
Il punto di partenza è il bug della tua bozza: `make_stream(h, t)` riceve la coda già calcolata, quindi va in stack overflow.
