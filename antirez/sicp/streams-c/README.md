# SICP 3.5 — Stream in C

Gli stessi stream del percorso in TypeScript (`../streams/`), riscritti in C, verso il player audio.
Le firme sono in [`stream.h`](stream.h), i corpi in [`stream.c`](stream.c) sono `TODO(n)` e li scrivi tu, una
tappa alla volta. I test in `test/` ti dicono quando una tappa è finita.

In TypeScript tre cose erano gratis, e in C diventano esplicite:

| in TypeScript | in C |
|---|---|
| la coda è una lambda `() => integers_from(n + 1)` | una **funzione** (`TailFn`) più il suo **ambiente** (`void *env`): i "parametri catturati" messi a mano in una struct sull'heap |
| la lambda cattura le variabili | catturare una variabile vuol dire passare il suo **indirizzo** (`&ones`); catturarne il valore vuol dire **copiarlo** nell'ambiente |
| il garbage collector libera le celle | **qualcuno** deve chiamare `free`: chi possiede le celle? (tappa 4) |

## Come si lavora

```sh
cd antirez/sicp/streams-c
make t1          # solo la tappa 1 (t2, t3 per le altre)
make test        # tutte le tappe
make clean
```

- I binari finiscono in `$TMPDIR/streams-c`, fuori dal repo.
- Si compila con `-O0`, AddressSanitizer e UBSan. `-O0` toglie l'ottimizzazione delle chiamate in coda: una
  ricorsione dove serve un ciclo finisce lo stack (8 MB) e il test va in crash. AddressSanitizer segnala gli
  accessi fuori dai limiti e i puntatori a variabili locali di funzioni già tornate (`stack-use-after-return`).
- Ogni test gira in un processo figlio (`fork`): un crash (anche l'`abort` di un `TODO`) o un test che dura più
  di 10 secondi fanno fallire solo quel test.
- **Per ora la memoria non si libera mai.** Le tappe 1–3 allocano e basta: la memoria è il tema della tappa 4.
- Segui l'ordine delle tappe; puoi aggiungere funzioni `static` di supporto, ma non cambiare le firme di `stream.h`.
- I suggerimenti sono chiusi in `<details>`: aprili solo dopo averci provato.

---

## Tappa 1 — Una cella con la coda ritardata

Da implementare: `cons_stream`, `stream_head`, `stream_tail`, `stream_ref`, `take`, `integers_from`,
`stream_enumerate_interval`, `array_to_stream`. Test: `make t1`.

La cella è già nella `struct Stream`: la testa, la funzione della coda e il suo ambiente. `cons_stream` alloca la
cella e **non** chiama `tail_fn`; `stream_tail` la chiama passandole `env`. Ogni stream "con parametri"
(`integers_from(n)`, `stream_enumerate_interval(low, high)`, `array_to_stream(xs, n)`) ha bisogno della sua
funzione di coda e del suo ambiente.

<details><summary>Suggerimento</summary>

- `integers_from(n)`: l'ambiente è il prossimo numero. Alloca un `Value` sull'heap, mettici `n + 1` e scrivi una
  `static Stream *integers_tail(void *env)` che lo legge e chiama `integers_from`.
- Quando l'ambiente ha più campi (`low` e `high`, oppure l'array e la posizione) definisci una piccola struct.
- `take` e `stream_ref` vanno scritte con un **ciclo**: il test del milione di elementi lo verifica.
- `take` non deve forzare la coda dopo l'ultimo elemento richiesto: il test "take non forza oltre" conta le code.
</details>

Domande per te:
1. Perché l'ambiente di `integers_from` deve stare sull'heap? Cosa succederebbe con `Value next = n + 1;` e
   `cons_stream(n, integers_tail, &next)`?
2. `array_to_stream` *prende in prestito* l'array. Scrivi un caso in cui lo stream sopravvive all'array: cosa
   leggerebbe la coda? In TypeScript il problema c'era?
3. In TypeScript `take` era ricorsiva e funzionava su un milione di elementi? Perché in C serve un ciclo?

## Tappa 2 — Memoizzazione

Nessuna funzione nuova: aggiungi i campi che servono nella `struct Stream` (in `stream.h`, dove c'è il commento) e
cambia `cons_stream` e `stream_tail`. Test: `make t2`.

È la tappa 2 di TypeScript (`memo`), con una differenza: in C la memo sta **dentro la cella** invece che in una
closure che avvolge la coda.

<details><summary>Suggerimento</summary>

- Ti servono la coda calcolata e un modo per sapere **se** è già stata calcolata. `NULL` non basta: è anche lo
  stream vuoto, un risultato valido (test "anche la coda vuota").
- Il test `ones_implicito` costruisce `ones` passando `&ones` come ambiente: la coda legge la variabile quando
  viene forzata, dopo l'assegnamento. È la closure sulla variabile di `cycle` (tappa 9), scritta a mano.
</details>

Domande per te:
1. Dopo che la coda è stata calcolata, la cella ha ancora bisogno di `tail_fn` e di `env`? Cosa ne faresti?
   (Tienilo a mente per la tappa 4.)
2. In `ones_implicito` l'ambiente è `&ones`, l'indirizzo di una variabile **locale** del test. Quando diventerebbe
   un puntatore pendente?
3. `stream_tail` scrive dentro la cella. Cosa vuol dire per uno stream condiviso fra due thread (pensa agli attori
   del player)?

## Tappa 3 — map, filter e operazioni

Da implementare: `stream_map`, `stream_map2`, `stream_filter`, `add_streams`, `mul_streams`, `scale_stream`.
Test: `make t3`.

In TypeScript `scale_stream` era `stream_map(x => x * factor, s)`: la lambda catturava `factor`. In C una
funzione non può catturare niente, quindi `MapFn`, `MapFn2` e `Pred` ricevono un **contesto** `void *ctx` scelto
da chi chiama: `stream_map(f, ctx, s)` chiamerà `f(x, ctx)` per ogni elemento.

<details><summary>Suggerimento</summary>

- `stream_map`: se `s` è vuoto, vuoto; altrimenti una cella con testa `f(head, ctx)` e un ambiente che ricorda
  `f`, `ctx` e `s` (una struct). La coda applica `stream_map` alla coda di `s`.
- `stream_filter`: è la tua tappa 3 in TypeScript. Prima un **ciclo** che scarta gli elementi finché il predicato
  è falso, poi una cella. Il predicato una volta sola per elemento.
- `add_streams`, `mul_streams` sono `stream_map2` con una funzione `static`. `scale_stream` deve dare a
  `stream_map` un `ctx` che punti a `factor`… ma quale `factor`? Il test `scaled_ints` forza la coda quando
  `scale_stream` è già tornata, e AddressSanitizer controlla.
- `fibs` nel test è definito come in SICP: tre celle e `add_streams(stream_tail(fibs), fibs)`.
</details>

Domande per te:
1. Perché `scale_stream` non può passare `&factor` a `stream_map`? Che errore dà AddressSanitizer, e perché
   senza AddressSanitizer potrebbe *sembrare* funzionare?
2. In `stream_map` l'ambiente deve contenere `ctx` (il puntatore) o il valore a cui punta? Chi decide quanto vive
   il contesto?
3. Il test `fibs_implicito` calcola F(78) in un istante. Quanto ci metterebbe senza memo, e perché?
4. In `fibs`, chi punta a chi? Disegna le celle e le frecce dopo `take(fibs, 5)`: c'è un ciclo? (Tappa 4.)

---

## Le prossime tappe (in preparazione)

Le preparo quando arrivi qui, così le adattiamo a come hai scritto le prime tre.

4. **Chi possiede le celle.** Finora non si libera niente. Con il refcount di toyforth (`retain`/`release`) ogni
   cella possiede la sua coda e l'ambiente possiede gli stream da cui legge: consumando `integers_from(0)` senza
   tenere la testa, la memoria deve restare costante (un contatore di celle vive lo verificherà). Poi la domanda:
   cosa succede con `ones` e `fibs`, che puntano a se stessi?
5. **Cicli: refcount o arena?** Le definizioni implicite sono cicli, e il refcount non libera i cicli. Un'arena
   libera tutto insieme ma niente prima. Quale scegliere, e quando?
6. **Il ponte verso il player.** Uno stream che tira i dati da una sorgente esterna (come `gen_to_stream`, o come
   `read_packet` che tira byte dal ring buffer), e una pipeline di campioni: `smooth`, `scale_stream` come volume,
   `zero_crossings`. Chi spinge, chi tira, e dove sta il buffer.
