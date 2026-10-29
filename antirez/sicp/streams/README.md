# SICP 3.5 — Stream in TypeScript

Esercizi guidati sul capitolo 3.5 di SICP (edizione JavaScript: <https://sourceacademy.org/sicpjs/3.5>).
Le firme delle funzioni sono in [`src/stream.ts`](src/stream.ts): i corpi sono `todo(n)` e li scrivi tu,
una tappa alla volta. I test in `test/` ti dicono quando una tappa è finita.

`streams.ts` (qui accanto) è la tua prima bozza, tutta commentata: la usiamo come punto di partenza della tappa 1.

## Come si lavora

```sh
cd antirez/sicp/streams
node --test test/01-basi.test.ts     # una tappa alla volta
npm test                             # tutte le tappe
npm run typecheck                    # controllo dei tipi di src/ (scarica TypeScript con npx la prima volta)
```

Node 24 esegue direttamente i file `.ts` (toglie i tipi senza controllarli): per i tipi c'è `npm run typecheck`.
Non serve `npm install`.

Regole del gioco:
- **Segui l'ordine delle tappe.** Le costanti della tappa 4 (`ones`, `integers`, …) sono calcolate quando il modulo
  viene caricato: se le scrivi prima che `cons_stream` funzioni, falliscono *tutti* i file di test.
- Senza la memoizzazione (tappa 2) i test delle tappe 4 e 6 non finiscono (minuti): è voluto, interrompi con Ctrl-C.
- Puoi aggiungere funzioni di supporto, ma non cambiare le firme esportate.
- I suggerimenti sono chiusi in `<details>`: aprili solo dopo averci provato.

---

## Tappa 1 — Uno stream è una coppia con la coda ritardata

Lettura: SICP JS 3.5.1 fino a "Implementing delay and force" escluso.

Uno stream è `null` oppure `{ head, tail }`, dove `head` è già un valore e `tail` è una **funzione senza argomenti**
(un *thunk*) che, quando la chiami, restituisce il resto dello stream. Costruire uno stream non calcola la coda;
`stream_tail` la calcola.

Da implementare: `cons_stream`, `stream_head`, `stream_tail`, `stream_ref`, `take`, `array_to_stream`,
`stream_enumerate_interval`, `integers_from`.

**Prima di scrivere codice**, guarda la tua bozza in `streams.ts`:

```ts
function make_stream(h: number, t: Stream): Stream {
  return { head: h, tail: () => t };
}
```

Con `integers_from(n) = make_stream(n, integers_from(n + 1))` va in stack overflow. Perché? Dove deve stare la
lambda perché il calcolo della coda sia davvero rimandato?

<details><summary>Suggerimento</summary>

JavaScript valuta gli argomenti *prima* di chiamare la funzione: `integers_from(n + 1)` viene eseguito prima che
`make_stream` possa avvolgerlo in una lambda. La lambda deve scriverla **il chiamante**:
`cons_stream(n, () => integers_from(n + 1))`. È lo stesso motivo per cui `M(make_fractal)` non va in loop nel
picture language: la chiamata ricorsiva sta dentro una funzione che parte solo quando serve.

Per `take`: chiedi la coda solo se ti serve un altro elemento (c'è un test apposta).
</details>

Domanda per te: `stream_ref(integers_from(1), 1000)` quante volte chiama `integers_from`? E `take(s, 10)` chiamato
due volte sullo stesso `s`?

## Tappa 2 — Memoizzazione

Lettura: SICP JS 3.5.1, "Implementing delay and force" (la funzione `memo`).

Con la tappa 1 ogni `stream_tail` ricalcola la coda da capo. `memo(f)` restituisce un thunk che chiama `f` una volta
sola e poi restituisce sempre lo stesso valore. Poi fai in modo che `cons_stream` memoizzi la coda.

<details><summary>Suggerimento</summary>

Due variabili nella closure: un flag `done` e il valore. Non usare il valore stesso come flag
(`if (value === undefined)`): uno stream vuoto è `null`, e c'è un test per questo.
</details>

Domanda per te: in C, dove terresti il flag e il valore memoizzato? Chi è il proprietario della coda calcolata,
e quando la si può liberare? (Ci torneremo nella versione C.)

## Tappa 3 — map, filter e la pigrizia

Lettura: SICP JS 3.5.1, da "stream_map" all'esempio del secondo numero primo; esercizi 3.50 e 3.51.

Da implementare: `stream_map` con più stream (es. 3.50), `stream_filter`, `add_streams`, `mul_streams`, `scale_stream`.

Il test sul secondo primo tra 10.000 e 1.000.000 conta le chiamate a `is_prime`: con le liste sarebbero quasi un
milione, con gli stream sono 10. Il test dell'es. 3.51 controlla *quando* `stream_map` applica la funzione.

<details><summary>Suggerimento</summary>

`stream_map` calcola subito la testa (applica `f` alle teste) e ritarda il resto. Con più stream, se uno qualunque è
vuoto il risultato è vuoto. `add_streams` & co. sono una riga ciascuna, con `stream_map`.
</details>

## Tappa 4 — Stream infiniti e definizioni implicite

Lettura: SICP JS 3.5.2 ed esercizi 3.53, 3.54, 3.56.

Da implementare: `ones`, `integers`, `fibs`, `sieve`, `primes` + `is_prime`, `merge` + `hamming`, `factorials`.

Qui gli stream si definiscono **in termini di se stessi**: `ones` è un 1 seguito da `ones`. Funziona solo perché la
coda è ritardata: quando il thunk viene chiamato, la costante esiste già.

<details><summary>Suggerimento</summary>

- `integers`: il primo è 1, il resto è `integers` + `ones`, elemento per elemento.
- `fibs`: guarda la figura di SICP con `fibs` e `stream_tail(fibs)` sommati in colonna.
- `primes` e `is_prime` si usano a vicenda: per testare `n` bastano i primi `p` con `p * p <= n`.
- `hamming`: 1, poi la fusione degli stream `2·S`, `3·S` e `5·S`.
- `fibs` senza memo è esponenziale: se il test va in timeout, ricontrolla la tappa 2.
</details>

Domanda per te: `integers` implicito e `integers_from(1)` producono gli stessi numeri. Che differenza c'è nel lavoro
fatto e nella memoria occupata se tieni un riferimento alla testa e chiedi l'elemento 1.000.000?

## Tappa 5 — Somme parziali, approssimazioni, serie

Lettura: SICP JS 3.5.2 (es. 3.55, 3.59) e 3.5.3 fino al tableau.

Da implementare: `partial_sums`, `sqrt_stream`, `pi_stream`, `euler_transform`, `make_tableau`,
`accelerated_sequence`, `integrate_series`, `exp_series`, `cosine_series`, `sine_series`.

Uno stream può essere una **sequenza di approssimazioni**: invece di un ciclo con una condizione d'arresto, produci
tutte le approssimazioni e lasci a chi le usa decidere quando fermarsi. L'acceleratore di Eulero e il tableau
sono funzioni da stream a stream.

<details><summary>Suggerimento</summary>

- `partial_sums(s)`: il risultato si riferisce a se stesso, come `integers`.
- `pi_stream`: prima lo stream dei termini 1, -1/3, 1/5, …, poi `partial_sums` e `scale_stream` per 4.
- `exp_series`: e^x è uguale al suo integrale più la costante 1.
- `sine_series`/`cosine_series` si definiscono l'una con l'altra: stesse regole di `primes`/`is_prime`.
</details>

## Tappa 6 — Ritardo esplicito

Lettura: SICP JS 3.5.4.

Da implementare: `integral` (l'integrando arriva come thunk) e `solve`.

In `solve`, `y` dipende da `dy` e `dy` dipende da `y`: uno dei due deve essere passato come thunk, altrimenti la
seconda costante non esiste ancora quando la prima viene costruita.

Domanda per te: perché qui il ritardo della coda di `cons_stream` non basta più e serve un thunk anche
sull'argomento di `integral`?

## Tappa 7 — Segnali: il ponte verso l'audio

Lettura: SICP JS 3.5.3, esercizi 3.74, 3.75, 3.76.

Da implementare: `sign_change_detector`, `zero_crossings`, `smooth`, `smoothed_zero_crossings`.

Un segnale campionato (come il PCM che il tuo player passa a SDL) è uno stream di numeri. Contare i passaggi per lo
zero è un'operazione da stream a stream; con il rumore servono prima un filtro (`smooth`) e poi il rilevatore,
e il programma resta una composizione di pezzi piccoli (es. 3.76).

---

## Tappa 8 — Generatori: stream senza memoria

Lettura: MDN, [`function*`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/function*)
e il [protocollo degli iteratori](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Iteration_protocols).

Da implementare, in `src/generators.ts`: `integers_from_gen`, `take_gen`, `map_gen`, `filter_gen`, `stream_to_gen`,
`gen_to_stream`, `sieve_gen`. Test: `node --test test/08-generatori.test.ts`.

Un generatore è pigro come uno stream: `function*` non esegue nulla finché qualcuno non chiama `next()`, e ogni
`yield` sospende il calcolo come la lambda della coda. Ma lo stato sta **dentro** il generatore e avanza a ogni
`next()`: non c'è una cella da rileggere. Gli ultimi due ponti (`stream_to_gen`, `gen_to_stream`) mostrano cosa
serve per passare da un modello all'altro.

<details><summary>Suggerimento</summary>

- `take_gen` con `for...of` e `break` sembra comodo, ma `break` chiama `return()` sull'iteratore e **chiude** il
  generatore: il test "un generatore si consuma" fallisce. Usa `next()` direttamente.
- `map_gen` con più iteratori: chiedi un valore a ciascuno e fermati appena uno risponde `done`.
- `gen_to_stream`: la testa è il primo `next()`; la coda è... un altro `gen_to_stream` dello stesso generatore.
  Perché questo funziona solo grazie alla memo di `cons_stream`?
- `sieve_gen`: il primo valore è un primo `p`; il resto è il crivello degli elementi non divisibili per `p`
  (`yield*` delega a un altro generatore).
</details>

Domande per te:
1. Quale test della tappa 2 fallirebbe se `cons_stream` fosse un generatore? Perché?
2. Puoi scrivere `ones` o `fibs` *implicitamente* con i generatori, cioè riferendoti al generatore stesso come fai
   con `const ones = cons_stream(1, () => ones)`? Cosa manca?
3. In `gen_to_stream`, cosa succede se qualcun altro chiama `g.next()` dopo che hai costruito lo stream?
4. `sieve_gen` annida un `filter_gen` per ogni primo trovato. Cosa succede alla profondità delle chiamate quando
   chiedi il millesimo primo? Confrontalo con `sieve` sugli stream.

## Tappa 9 — Animare il picture language

Lettura: MDN, [`requestAnimationFrame`](https://developer.mozilla.org/en-US/docs/Web/API/Window/requestAnimationFrame);
il tuo `../language-picture/script.ts` (`beside`, `below`, `right_split`).

Due parti:
- **pura, con i test**, in `src/animation.ts`: `iterate`, `cycle`, `play`. Test: `node --test test/09-animazione.test.ts`.
  Non c'è DOM: un "painter" qui può essere anche una stringa, e l'orologio del browser è un finto
  `requestAnimationFrame` (`fakeClock` nel test) che chiama le callback agli istanti scelti dal test.
- **la colla con il browser**, in `../language-picture/animation.ts`: tre passi descritti nei commenti, senza test.
  Si guarda con gli occhi:
  ```sh
  cd ../language-picture
  npx -y -p typescript@5.9 tsc -p tsconfig.anim.json   # → dist/ (anche script.js e i tuoi stream)
  python3 -m http.server 8000                          # → http://localhost:8000/animation.html
  ```
  `animation.html` carica prima `script.js` come script classico: i suoi nomi (`painter`, `beside`, `frame1`, `ctx`…)
  sono globali e il modulo `animation.ts` li vede senza import.

`play` è il consumatore: lo stream dice *cosa* mostrare, l'orologio dice *quando*. A ogni frame `play` guarda il
tempo trascorso, calcola quale elemento tocca e fa avanzare lo stream fin lì.

<details><summary>Suggerimento</summary>

- `iterate`: la testa è `x`; la coda è un altro `iterate`. Dove chiami `f` perché costruire lo stream non la chiami?
- `cycle`: è una definizione implicita, come `ones`. Lo stream deve tornare alla sua **prima cella**, non a una
  copia: costruisci le celle con una funzione interna `build(i)` e, dopo l'ultima, la coda restituisce la prima.
- `play`: tieni lo stato in variabili locali chiuse nella callback (lo stream corrente, l'indice mostrato,
  l'istante del primo frame, un flag per `stop`). L'avanzamento di più elementi va fatto con un **ciclo**: il test
  "scheda tornata dopo molto tempo" avanza di 100.000 elementi in un frame solo.
- Nella colla: `right_split(p, n)` = `beside(p)(below(smaller)(smaller))` con `smaller` il livello `n - 1`.
</details>

Domande per te:
1. Chi decide il ritmo dell'animazione: lo stream o il browser? E se `play` mostrasse un elemento **a ogni frame**
   invece che a ogni `period`, cosa cambierebbe su uno schermo a 60 Hz e su uno a 120 Hz?
2. Con `stream_map(n => right_split(p, n), integers_from(0))` invece di `iterate`, quanti `beside`/`below`
   costruisci per arrivare al livello 10? E con `iterate`? Costruire un painter costa quanto disegnarlo?
3. `cycle` è infinito: quante celle occupa in memoria? Cosa succederebbe senza la memo di `cons_stream`?
4. Se `play` tenesse da parte la testa dello stream (per esempio per poter ricominciare), cosa succederebbe alla
   memoria dopo un'ora di `iterate` a 60 elementi al secondo? E con `cycle`?
5. Quando il browser è lento, `play` salta degli elementi: li calcola comunque? Perché non può farne a meno?
6. `requestAnimationFrame` *spinge* una callback a ogni frame, `play` *tira* gli elementi dallo stream. Dove trovi
   la stessa coppia nel player audio (pensa alla callback audio di SDL)?

## Dopo i test: tre esercizi aperti

1. **Animare il picture language con uno stream.** Diventato la tappa 9 (sopra).
2. **Generatori JS.** Diventato la tappa 8 (sopra).
3. **Stream in C** (cartella da creare, prossimo passo): la coda diventa un puntatore a funzione più un ambiente
   (`struct` con i "parametri catturati"), la memoizzazione un campo nella cella. La domanda vera sarà la memoria:
   chi possiede le celle già calcolate, con il refcount di toyforth o con un'arena?

Il collegamento con l'audio player: la tua pipeline (byte di rete → chunk HTTP → pacchetti TS → frame PCM → SDL) è
già una catena di trasformazioni da stream a stream. `getNextByte` è un iteratore che *tira* i byte quando servono,
come `stream_tail`; gli attori invece *spingono* messaggi. SICP 3.5.5 mette proprio a confronto oggetti con stato
(gli attori) e stream: lo leggeremo dopo la versione C.
