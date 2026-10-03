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
Percorso concreto per SICP 3.5 (JS edition), un commit per passo. L'ho provato in un prototipo nella build dir (`build/sicp/streams_proto.ts`, compila con `--strict` e stampa i risultati attesi); qui sotto solo la traccia, il codice scrivilo tu.
1. **Tipo e costruttore (3.5.1)**: parti dalla pair del picture language ma rendila generica: `type Stream<T> = null | { head: T; tail: () => Stream<T> }`. Scrivi `cons_stream(h, t: () => Stream<T>)`: il chiamante passa sempre una lambda (`() => integers_from(n + 1)`). Test: `integers_from(1)` non deve più andare in stack overflow.
2. **delay/force con memoizzazione**: `memo(f)` restituisce un thunk che calcola `f()` solo la prima volta (flag `done` + valore in closure); `cons_stream` avvolge la coda con `memo`. Test: un contatore dentro `integers_from` deve restare a 10 dopo due letture dei primi 10 elementi (senza memo raddoppia).
3. **Operazioni di base**: `stream_ref`, `take(s, n)` (in array, per stampare), `stream_map(f, s)`, `stream_map2(f, s1, s2)`, `stream_filter(p, s)`. Test: `take(stream_filter(even, integers), 5)` → `2,4,6,8,10`.
4. **Stream infiniti (3.5.2)**: `integers`, crivello di Eratostene `sieve(s) = cons_stream(head, () => sieve(filter(non multiplo di head, tail)))` → primi 10 primi `2,3,5,…,29`; definizioni implicite `ones = cons_stream(1, () => ones)` e `integers = cons_stream(1, () => add_streams(ones, integers))` (qui la memo evita lavoro esponenziale).
5. **Esercizi 3.5.2-3.5.3**: `partial_sums(integers)` → `1,3,6,10,15,…` (es. 3.55), `scale_stream`, poi le approssimazioni di π con `partial_sums` e l'accelerazione di Eulero.
6. **Integrali e ritardo esplicito (3.5.4)**: `integral(integrand: () => Stream<number>, initial, dt)` con l'integrando **ritardato** (thunk), poi `solve(f, y0, dt)` con `y` e `dy` che si riferiscono a vicenda. Test: `stream_ref(solve(y => y, 1, 0.001), 1000)` ≈ 2.7169 (approssima e).
Collegamento utile: `cons_stream` con thunk è la stessa idea che fa funzionare `make_fractal` (la chiamata ricorsiva è dentro una funzione e parte solo quando serve).
