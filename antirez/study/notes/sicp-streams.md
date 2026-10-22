# Stream (SICP 3.5) in TypeScript
Periodo: 2026-10-03 → in corso (tappe 1-5 fatte) · Percorso a tappe con test in `sicp/streams/README.md` · Progetto: [sicp-streams](../projects/sicp-streams.md)

## Mappa in breve
- Uno stream è `null` oppure `{ head, tail }`: la testa è già un valore, la coda è un **thunk** (funzione senza argomenti) che calcola il resto solo quando viene forzata (`stream_tail`).
- Con la **memoizzazione** (`memo`) ogni coda si calcola una volta sola: è ciò che rende possibili le definizioni implicite (`integers`, `fibs`, `primes`) senza costi esponenziali.
- `stream_map` / `stream_filter` separano *chi descrive il calcolo* da *chi consuma*: si calcola solo quello che qualcuno chiede.
- Gli stream infiniti si definiscono **in termini di se stessi**: funziona perché la coda è ritardata, quindi quando il thunk parte la costante esiste già.
- Domanda di fondo che torna in ogni tappa e che diventerà centrale nella versione C: **chi tiene vivo cosa**, cioè chi ha un riferimento alla testa e quindi impedisce di liberare le celle già calcolate.
- Le risposte qui sono quelle corrette, verificate con i test (`cd sicp/streams && node --test`) o con piccoli esperimenti in Node.

## Concetti

### Coda ritardata (thunk) e valutazione eager
- Cos'è: JavaScript valuta gli argomenti **prima** di chiamare la funzione. `make_stream(n, integers_from(n + 1))` esegue `integers_from(n + 1)` prima che `make_stream` possa avvolgerlo in una lambda: ricorsione infinita, stack overflow. La lambda deve scriverla **il chiamante**: `cons_stream(n, () => integers_from(n + 1))`.
- `stream_ref(integers_from(1), 1000)` chiama `integers_from` 1001 volte: una per la testa, una per ogni coda forzata.
- `take(s, n)` deve forzare solo n-1 code: dopo l'ultimo elemento richiesto non serve chiedere la coda (c'è un test apposta).
- Stesso principio del picture language: `M(make_fractal)` non va in loop perché la chiamata ricorsiva sta dentro una funzione che parte solo quando serve.

### Memoizzazione delle code e memoria
- `memo(f)` restituisce un thunk che chiama `f` la prima volta e poi restituisce sempre lo stesso risultato. Servono **due** variabili nella closure, un flag e il valore: usare il valore come flag (`if (value === undefined)`) sbaglia quando il risultato è `null` (lo stream vuoto), `0` o `undefined`.
- Senza memo ogni `stream_tail` ricalcola la coda: due `take(s, 10)` sullo stesso `s` forzano 9 + 9 code.
- Con memo le celle forzate restano attaccate alla catena (ogni coda memoizzata punta alla cella successiva). Restano in memoria **solo finché qualcuno tiene la testa**: con `const s = integers_from(1); stream_ref(s, 1e6)` il milione di celle è raggiungibile da `s`; con `stream_ref(integers_from(1), 1e6)` (e un `stream_ref` iterativo) le celle superate diventano spazzatura.
- Senza memo una cella tiene solo il thunk: si paga in tempo (ricalcolo), non in memoria.
- In C non c'è il GC: bisognerà decidere chi possiede la cella calcolata (refcount come in toyforth, o un'arena).

### stream_map, stream_filter e la pigrizia
- `stream_map` applica `f` alla testa **subito**, quando costruisce la cella, e ritarda il resto. Con più stream (es. 3.50) si ferma appena uno è vuoto.
- Es. 3.51 con `x = stream_map(display, stream_enumerate_interval(0, 10))`: alla definizione stampa `0`; `stream_ref(x, 5)` stampa `1 2 3 4 5`; poi `stream_ref(x, 7)` stampa solo `6 7` grazie alla memo. Senza memo la seconda chiamata ristamperebbe `1 2 3 4 5 6 7`.
- `display` deve restituire il valore (`(console.log(v), v)`, operatore virgola): ciò che restituisce `f` diventa la testa dello stream risultato.
- Costruire uno stream non lo percorre: per applicare un effetto a tutti gli elementi qualcuno deve forzare le code fino a `null` (`stream_for_each`). Su uno stream infinito non termina.
- `stream_filter`: se la testa non soddisfa il predicato non c'è una cella da restituire, quindi bisogna cercare **subito** (senza lambda). `stream_filter(x => false, integers_from(1))` non può terminare.
- Scritto con la ricorsione, il ramo "scarta" va in stack overflow anche su stream finiti lunghi (10.000 elementi scartati di fila), perché JavaScript non ottimizza le chiamate in coda. Con un ciclo (`while (s !== null && !pred(stream_head(s))) s = stream_tail(s);`) lo stack resta costante e, se nessuno tiene la testa, anche lo heap: la riassegnazione di `s` rende irraggiungibili le celle scartate.
- Il predicato va chiamato **una volta** per elemento: chiamarlo due volte (nel test e nel ciclo) fa fallire il test che conta le chiamate a `is_prime` (secondo primo tra 10.000 e 1.000.000: 10 chiamate, non 11).

### Closure: si cattura la variabile, non il valore
- In `cons_stream(stream_head(s), () => stream_filter(pred, stream_tail(s)))` la lambda cattura la **variabile** `s` e ne legge il valore quando viene eseguita. Riassegnare `s` non crea una nuova variabile.
- Funziona perché la lambda nasce **dopo** il ciclo e `s` non viene più riassegnata: quando la coda viene forzata, `s` contiene ancora la cella trovata.
- Ogni *chiamata* di `stream_filter` ha il suo `s` (parametri nuovi a ogni invocazione): per questo le lambda di celle diverse non interferiscono.
- `let s = 1; const f = () => s; s = 2; f()` restituisce 2.
- In C una "closure" è una `struct` in cui si copiano i valori (cattura per valore); tenere un puntatore a una variabile sullo stack di una funzione già ritornata è un bug.

### Stream infiniti e definizioni implicite
- `ones = cons_stream(1, () => ones)`: una sola cella che punta a se stessa.
- `integers = cons_stream(1, () => add_streams(ones, integers))`: lo stream prodotto da `add_streams` **è** la coda di `integers`, non una copia. Per la cella k si legge la cella k-1, già memoizzata. Lavoro e memoria per l'elemento 1.000.000 sono dello stesso ordine di `integers_from(1)` (circa un milione di celle e di passi); cambia la costante: ogni passo è un `stream_map` con un'addizione e celle un po' più grosse.
- La differenza vera è la dipendenza dalla memo: senza memo `integers` implicito diventa quadratico e `fibs` esponenziale, mentre `integers_from` funziona lo stesso (ogni cella sa già il suo numero).
- `factorials = cons_stream(1, () => mul_streams(stream_tail(integers), factorials))`: si moltiplica 2, 3, 4, … per 1!, 2!, 3!, …; con `integers` si ottiene uno sfasamento (1, 1, 2, 6, …).
- Le costanti di modulo (`integers`, `primes`, `fibs`) sono tenute vive dal modulo stesso: le celle forzate non vengono mai liberate.

### primes e is_prime: definizione circolare
- `primes = cons_stream(2, () => stream_filter(is_prime, integers_from(3)))` e `is_prime(n)` scorre `primes` finché `p * p > n`.
- Vantaggio della costante memoizzata: ogni primo viene **calcolato** una volta sola in tutta la vita del programma e tutte le chiamate a `is_prime` lo rileggono gratis (cache dei risultati, non risparmio di allocazioni). Se ogni chiamata ricostruisse il suo stream dei primi, ogni livello rifarebbe il lavoro del precedente. Prezzo: le celle non vengono mai liberate (memoria contro tempo).
- Perché non si blocca: la testa 2 è data, e `is_prime(n)` legge solo primi con `p² ≤ n`. Quando `primes` cerca il primo dopo pₖ, i candidati sono minori di pₖ² (il primo successivo è già minore di 2·pₖ, postulato di Bertrand), quindi `is_prime` resta dentro le celle già calcolate. Per i candidati vicino a 7919 servono i primi fino a 89 (circa il 24°).
- Senza il 2 esplicito (`primes = stream_filter(is_prime, integers_from(2))`) il filtro chiama subito `is_prime(2)`, che legge `primes` mentre la costante è ancora in inizializzazione: `ReferenceError: Cannot access 'primes' before initialization` (temporal dead zone di `const`).
- `is_prime` è esportata: deve rispondere `false` per `n < 2` (0, 1, negativi), altrimenti `2 * 2 > n` la fa rispondere `true`.

### Sequenze di approssimazioni e acceleratori
- `sqrt_stream(x)`: 1 seguito da `stream_map(improve, stream)` sullo stream stesso (Newton, `(g + x/g) / 2`). Per √2 le cifre corrette circa raddoppiano a ogni passo (0, 1, 3, 6, 12, 16). Con x = 0 la radice è doppia: il passo diventa `g / 2` e la convergenza è lineare.
- Lo stream separa *cosa* si calcola da *quando fermarsi*: decide il consumatore (`take`, `stream_ref`, `stream_limit` dell'es. 3.64). Non si calcola meno che con un `while`, ma generatore e criterio si combinano liberamente e la sequenza diventa un valore da trasformare.
- `pi_stream` = 4 · somme parziali di 1, −1/3, 1/5, … (dispari con `stream_map(n => 2*n - 1, integers)`, segni con `alternating_ones = cons_stream(1, () => stream_map(x => -x, alternating_ones))`). Errore circa 1/n: per 10 cifre servirebbero 10¹⁰ termini (e non basterebbero per gli arrotondamenti).
- `euler_transform(s)`: il primo elemento usa S0, S1, S2, la coda è `euler_transform(stream_tail(s))` (finestra che scorre). Su `pi_stream` l'errore all'elemento 7 passa da 0,12 a 3,4·10⁻⁴.
- `make_tableau(t, s)` è uno stream di stream (`s`, `t(s)`, `t(t(s))`, …) con la stessa forma di `sqrt_stream`; `accelerated_sequence` prende la testa di ogni riga. Su π: precisione macchina (≈10⁻¹⁵) in 8 elementi.
- Oltre la precisione del double (64 bit, 53 di mantissa, 15-17 cifre) una riga diventa costante: `0 / 0 = NaN` dall'elemento 10, e il `NaN` si propaga a tutte le righe successive. Serve un consumatore che si fermi.
- Tipi: `stream_map(stream_head, …)` inferisce `R = unknown` (la generica `T` non ha da dove essere dedotta); una lambda tipizzata evita il cast `as`.

### Serie di potenze come stream di coefficienti
- `partial_sums(s)`: `const ps_stream = cons_stream(stream_head(s), () => add_streams(stream_tail(s), ps_stream))`, con `null` per `s` vuoto. Dare un nome al risultato fa condividere le celle (lineare); chiamare `partial_sums(s)` nella coda crea uno stream nuovo a ogni livello (quadratico). Senza memo è quadratico in entrambi i casi.
- `integrate_series(s)`: moltiplica elemento per elemento per 1, 1/2, 1/3, … (coefficiente di xⁿ → coefficiente di xⁿ⁺¹).
- `exp_series = cons_stream(1, () => integrate_series(exp_series))`: 1, 1, 1/2, 1/6, … = 1/n!, senza fattoriali.
- `cosine_series = cons_stream(1, () => integrate_series(scale_stream(sine_series, -1)))` e `sine_series = cons_stream(0, () => integrate_series(cosine_series))`: costanti cos 0 = 1 e sin 0 = 0; si usano a vicenda solo dentro le lambda, quindi l'ordine delle dichiarazioni non conta.

## Domande di autoverifica
1. Perché `integers_from(n) = make_stream(n, integers_from(n + 1))` va in stack overflow? Dove deve stare la lambda?
2. Quante volte `stream_ref(integers_from(1), 1000)` chiama `integers_from`? E senza memo, quante code forzano due `take(s, 10)` sullo stesso `s`?
3. Perché `memo` ha bisogno di un flag separato dal valore?
4. Con la memo, perché `const s = integers_from(1); stream_ref(s, 1_000_000)` occupa memoria per un milione di celle e `stream_ref(integers_from(1), 1_000_000)` no?
5. Es. 3.51: cosa stampano la definizione di `x`, `stream_ref(x, 5)` e poi `stream_ref(x, 7)`? E senza memo?
6. Perché la funzione passata a `stream_map` per stampare deve restituire il valore?
7. Perché `stream_map(display, s)` da solo non stampa tutti gli elementi?
8. Perché nel ramo "scarta" di `stream_filter` non si può mettere la lambda? Perché la versione ricorsiva fallisce con 10.000 elementi scartati e quella col ciclo no?
9. Cosa succede se `stream_filter` chiama il predicato due volte sullo stesso elemento?
10. La coda di `stream_filter` usa `s`, che il ciclo riassegna: perché funziona?
11. `integers` implicito e `integers_from(1)`: che differenza c'è in lavoro e memoria per l'elemento 1.000.000? E senza memo?
12. Che vantaggio ha `primes` come costante memoizzata per `is_prime`?
13. Perché la definizione circolare `primes`/`is_prime` non si blocca? Cosa succede senza il 2 esplicito?
14. In `partial_sums`, perché chiamare `partial_sums(s)` nella coda è quadratico e la versione con `const ps_stream` no? E senza memo?
15. Con uno stream di approssimazioni chi decide quando fermarsi? Che vantaggio ha rispetto a un `while`?
16. Perché `sqrt_stream(9)` raddoppia le cifre corrette e `sqrt_stream(0)` no?
17. Con l'errore di `pi_stream` circa 1/n, quanti termini servono per 10 cifre di π? Basterebbero?
18. Perché `accelerated_sequence(euler_transform, pi_stream)` dà `NaN` dall'elemento 10?
19. Perché `stream_map(stream_head, …)` ha bisogno di `as Stream<number>`?
20. Come si ottengono `exp_series`, `sine_series` e `cosine_series` senza fattoriali? Quali sono le costanti?
21. Perché in `exp_series` la chiamata a `integrate_series` deve stare dentro la lambda?
