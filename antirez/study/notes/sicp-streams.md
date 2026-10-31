# Stream (SICP 3.5) in TypeScript
Periodo: 2026-10-03 → 2026-10-31 (9 tappe fatte, 54/54 test) · Percorso a tappe con test in `sicp/streams/README.md` · Progetto: [sicp-streams](../projects/sicp-streams.md)

## Mappa in breve
- Uno stream è `null` oppure `{ head, tail }`: la testa è già un valore, la coda è un **thunk** (funzione senza argomenti) che calcola il resto solo quando viene forzata (`stream_tail`).
- Con la **memoizzazione** (`memo`) ogni coda si calcola una volta sola: è ciò che rende possibili le definizioni implicite (`integers`, `fibs`, `primes`) senza costi esponenziali.
- `stream_map` / `stream_filter` separano *chi descrive il calcolo* da *chi consuma*: si calcola solo quello che qualcuno chiede.
- Gli stream infiniti si definiscono **in termini di se stessi**: funziona perché la coda è ritardata, quindi quando il thunk parte la costante esiste già.
- I generatori JS (tappa 8) sono pigri come gli stream ma **senza memoria**: un valore letto è consumato; `gen_to_stream` aggiunge la memo e ne fa uno stream.
- Con l'animazione (tappa 9) lo stream dice **cosa** mostrare e l'orologio del browser (`requestAnimationFrame`) dice **quando**: `play` è il consumatore che tira gli elementi al ritmo del tempo, non dei frame.
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

### Ritardo esplicito: integral e solve
- `integral(delayed_integrand, initial, dt)`: `const integ = cons_stream(initial, () => add_streams(scale_stream(delayed_integrand(), dt), integ))`. Il thunk dell'integrando si chiama solo dentro la coda.
- In `solve`, `const y = integral(() => dy, y0, dt); const dy = stream_map(f, y);`: alla prima riga `dy` è in temporal dead zone. Il ritardo della coda di `cons_stream` protegge ciò che sta dentro la cella, non gli argomenti di una funzione: serve un ritardo esplicito sull'argomento.
- `solve(y => y, 1, dt)` è il metodo di Eulero su dy/dt = y: yₙ = (1 + dt)ⁿ, e con n = 1/dt passi si ottiene (1 + 1/n)ⁿ → e. Errore circa e·dt/2 (primo ordine): 1,36·10⁻³ con dt = 0,001.
- Eulero usa la pendenza all'inizio dell'intervallo: sottostima le curve convesse (eᵗ, ma anche e⁻ᵗ: 0,367695 contro 0,367879) e sovrastima quelle concave.

### Segnali come stream: zero crossing e smooth
- `sign_change_detector(corrente, precedente)`: +1 da negativo a ≥ 0, −1 da ≥ 0 a negativo, altrimenti 0 (lo zero conta come positivo).
- `zero_crossings(s) = stream_map(sign_change_detector, s, cons_stream(0, () => s))`: lo stream contro se stesso spostato di uno, con 0 davanti; niente casi particolari per il primo campione.
- `smooth(s) = stream_map(average, s, stream_tail(s))`, con il controllo per `s` vuoto (altrimenti `stream_tail(null)` lancia prima che `stream_map` possa fermarsi); un elemento in meno.
- `smoothed_zero_crossings(s) = zero_crossings(smooth(s))`: composizione, non copia. Copiare il corpo chiamando `smooth` due volte crea due stream separati e duplica il rilevatore.
- Ponte verso l'audio: il PCM che il player passa a SDL è uno stream di campioni; filtri e rilevatori sono trasformazioni da stream a stream.

### Generatori JS: stream senza memoria
- Cos'è: `function*` è pigro come uno stream (non esegue nulla finché qualcuno non chiama `next()`, ogni `yield` sospende il calcolo come la lambda della coda), ma lo stato sta **dentro** il generatore e avanza a ogni `next()`: un valore letto è consumato, non resta da rileggere. Uno stream memoizzato è un **valore**; un generatore è un **processo**.
- `integers_from_gen` (`sicp/streams/src/generators.ts:19`) è un `while (true) { yield n++; }`: lo stato in una variabile locale. Scritto come `yield n; yield* integers_from_gen(n + 1)` ogni livello resta sospeso dentro `yield*`: dopo k valori c'è una catena di k generatori e ogni `next()` la attraversa tutta (k frame sullo stack, O(n²) in tutto, `RangeError` dopo circa 5400 valori). `yield*` va bene solo se la profondità è limitata dalla forma del problema (l'altezza di un albero), non se cresce con la lunghezza della sequenza.
- `take_gen` (`generators.ts:28`) controlla `done`, non `value === undefined` (stesso errore del memo della tappa 2), e il controllo `n === 0` sta prima di chiedere valori. Con `for...of` + `break` il generatore verrebbe chiuso (`return()`): la seconda `take_gen` sullo stesso generatore darebbe `[]` (vedi le domande).
- `map_gen` (`generators.ts:66`) e `filter_gen` (`generators.ts:77`) sono cicli: stack costante. `map_gen` chiama `next()` su tutti gli iteratori prima di guardare `done`: se il primo è finito, il valore estratto dal secondo è perso. Uno stream invece ha la testa già calcolata e la si può guardare senza consumarla.
- `filter_gen(x => false, …)` su una sequenza infinita non termina, come `stream_filter`, ma si blocca solo al primo `next()`: lo stream è pigro sulla coda e non sulla testa, il generatore anche sul primo valore.
- I due ponti: `stream_to_gen` (`generators.ts:91`) percorre lo stream riassegnando `s` (le celle superate sono liberabili se nessun altro tiene la testa; con `primes` no, la tiene il modulo). `gen_to_stream` (`generators.ts:99`) è la memo che manca ai generatori: valore salvato + resto da chiedere quando serve = una cella di stream. Funziona solo con `cons_stream` memoizzato e se nessun altro chiama `g.next()`: passare `g` cede l'ownership allo stream.
- Le definizioni implicite (`ones`, `fibs`) non si traducono: sono dati letti in due punti, e con un generatore le due letture si rubano i valori. `fibs` diventa un ciclo con stato esplicito `(a, b)`.
- `sieve_gen` (`generators.ts:116`) usa `yield*` e annida un `filter_gen` per ogni primo: per il k-esimo primo ogni `next()` attraversa k livelli (2000 primi ok, 5000 `RangeError`). Anche `sieve` con gli stream va in `RangeError` a 5000: la catena di filtri è dell'algoritmo, `yield*` aggiunge la catena di delega.
- Test: `sicp/streams/test/08-generatori.test.ts` (8 test; `counting` conta le chiamate a `next()` per verificare la pigrizia e che `gen_to_stream` chieda ogni valore una volta sola).

### Animazione: lo stream dice cosa, l'orologio dice quando
- Cos'è: la tappa 9 separa *cosa* mostrare (uno stream di painter, o di stringhe e numeri nei test) da *quando* mostrarlo (l'orologio del browser). La parte pura sta in `sicp/streams/src/animation.ts`, senza DOM, testata con un orologio finto; la colla con il browser sta in `sicp/language-picture/animation.ts` (pagina `animation.html`, build `tsc -p tsconfig.anim.json` → `dist/`, esclusa dal repo con il `.gitignore`), fatta e verificata nel browser il 2026-10-31.
- La colla (`sicp/language-picture/animation.ts:13-34`): i livelli con `iterate(smaller => beside(painter)(below(smaller)(smaller)), painter)` (`:13`), cioè il passo ricorsivo di `right_split` (`script.ts:709`) con il painter originale fisso a sinistra e il livello precedente sotto e sopra; l'andata e ritorno con `cycle([...levels, ...levels.slice(1, -1).reverse()])` (`:21`), che dà 0 1 2 3 4 5 4 3 2 1 senza ripetere 5 e 0 quando il cerchio si chiude; `play` con `requestAnimationFrame`, `clearRect` prima di ogni painter (`:29`) e `stop` restituito (`:25`). I nomi di `script.js` (caricato prima come script classico) sono globali e il modulo li vede senza import; il restringimento `if (!ctx) throw` di `script.ts` però non attraversa i file, quindi in `animation.ts` `ctx` resta `CanvasRenderingContext2D | null` (`:28`).
- `iterate(f, x)` (`sicp/streams/src/animation.ts:19`): x, f(x), f(f(x)), … La chiamata a `f` sta dentro la lambda della coda: costruire lo stream non chiama `f` e, con la memo, ogni elemento si calcola una volta sola. Calcolare `f(x)` prima della `cons_stream` lascia la memo ma anticipa il lavoro di un elemento (vedi le domande).
- È la forma giusta per `right_split` (`sicp/language-picture/script.ts:709`): il livello n+1 è **un passo** (`beside(p)(below(s)(s))`) sul livello n. Fino al livello 10: 10 passi con `iterate`, 1 + 2 + … + 10 = 55 con `stream_map(n => right_split(p, n), integers_from(0))`, che ricostruisce ogni livello da zero. Costruire un painter costa poco (crea closure, non disegna); disegnarlo costa: il livello n contiene 2ⁿ⁺¹ − 1 copie del painter base, cioè 2047 George (17 segmenti) ≈ 34.800 linee al livello 10, a ogni disegno.
- `cycle(xs)` (`animation.ts:24`) è una definizione implicita come `ones`: `let stream` parte vuota (`:25`), `iter(i)` costruisce una cella con la coda ritardata e dopo l'ultima la coda restituisce `stream` (`:27`), che nel frattempo è stata assegnata alla prima cella (`:29`). Funziona perché la closure cattura la **variabile** e la coda viene forzata dopo l'assegnamento. Infinito con k celle in tutto (il test verifica che tre `stream_tail` su `cycle([1, 2, 3])` tornino alla stessa cella); con `xs = []` restituisce lo stream vuoto.
- `play(s, draw, schedule, period)` (`animation.ts:50`): `schedule` ha la forma di `requestAnimationFrame` (tipo `Schedule`, `:34`) e vale **una volta sola**, quindi la callback `step` si ri-registra a ogni frame (`:81`). Non è ricorsione sullo stack: `step` consegna se stessa al browser e ritorna, come un thunk ("chiamami dopo"). Fermarsi vuol dire non ri-registrarsi: fine dello stream (`:75`) o `stop`.
- Tempo assoluto: al primo frame `play` disegna la testa e fissa `start` (`:62-67`), mai più aggiornato; a ogni frame l'indice è `Math.floor((t − start) / period)` (`:70`) e un ciclo con un contatore separato fa avanzare lo stream fin lì (`:72-77`). Gli elementi rimasti indietro si **attraversano** (lo stream è sequenziale, le code vanno forzate) ma non si disegnano; frame nello stesso periodo non ridisegnano. Con il ciclo 100.000 elementi in un frame non fanno stack overflow.
- Perché non "un elemento per periodo dall'ultimo disegno": i frame non arrivano regolari (disegno pesante, GC, scheda in background che sospende `requestAnimationFrame`). Con il tempo relativo ogni elemento può durare fino a un frame in più e i ritardi si sommano (deriva): decide il browser. Con l'indice assoluto l'animazione resta in orario e salta elementi: decide l'orologio, come nei lettori audio/video (l'audio non rallenta, il video scarta fotogrammi).
- Tre "frame" diversi: il frame del browser (una chiamata della callback con il tempo `t`, circa 60 al secondo), l'elemento dello stream (mostrato per `period` ms, cioè per molti frame del browser) e `frame1` del picture language (il parallelogramma in cui il painter disegna).
- `stop` (`:85`) è una closure sullo stato privato di `play`: l'unico modo di fermare da fuori qualcosa che continua dopo il ritorno. Il flag si controlla all'inizio della callback (`:61`) perché una callback già registrata verrà chiamata comunque. Stesso schema del cleanup di `useEffect` (avvia e restituisci chi annulla); `clearInterval`, `cancelAnimationFrame` e `removeEventListener` fanno lo stesso con un id. Non è un thunk di stream: produce un effetto, non un valore, e chiamarla due volte è innocuo.
- Memoria: `play` riassegna `s` (`:74`), quindi le celle già mostrate diventano irraggiungibili e un `iterate` di un'ora occupa memoria costante. Tenere la testa (dentro `play`, nel chiamante o in una costante di modulo come `primes`) con la memo tiene tutte le celle: 60 elementi al secondo sono 216.000 celle in un'ora. Con `cycle` le celle sono k e restano comunque: tenere la testa è gratis. Per ricominciare si tiene la **ricetta**, non lo stream: `() => iterate(step, painter)`, un thunk un livello più su (lo scambio memoria/lavoro delle tappe 2 e 4).
- Test: `sicp/streams/test/09-animazione.test.ts` (9 test). `fakeClock` (`:11`) tiene una **lista** di callback in attesa, come il browser (più animazioni registrano le loro callback e vengono chiamate nello stesso frame con lo stesso `t`): così il test vede una doppia registrazione (`pending` = 2), che con un posto solo resterebbe nascosta e nel browser raddoppierebbe le callback a ogni frame (1024 dopo 10 frame).
- Spingere e tirare: `requestAnimationFrame` spinge una callback a ogni frame, `play` tira gli elementi dallo stream quando servono. Nel player audio il tirare è `read_packet` (`networking/client_http.c:333`), che FFmpeg chiama quando ha fame di byte e che li prende dal ring buffer, mentre la rete (e gli attori nella versione ad attori) spinge. Con SDL3 il tuo codice spinge anche il PCM (`SDL_PutAudioStreamData`) e il dispositivo lo tira dalla coda di SDL al suo ritmo (con la callback di SDL2 quel pull sarebbe esplicito, come `play`). Il buffer in mezzo è il punto d'incontro.

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
22. In `solve`, perché `integral` deve ricevere l'integrando come thunk? Il ritardo della coda di `cons_stream` non basta?
23. Perché `stream_ref(solve(y => y, 1, 0.001), 1000)` approssima e? Come scala l'errore con dt?
24. Perché Eulero sta sotto e? E con `solve(y => -y, 1, 0.001)`?
25. Come si scrive `zero_crossings` con una sola `stream_map`?
26. Perché `smoothed_zero_crossings` va scritto come composizione? Perché `smooth` controlla lo stream vuoto?
27. Perché `take_gen` deve controllare `done` e non `value === undefined`?
28. Cosa succede se `integers_from_gen` è scritto come `yield n; yield* integers_from_gen(n + 1)`? Perché lo stream `integers_from` non ha lo stesso problema?
29. Si possono scrivere `ones` o `fibs` implicitamente con i generatori?
30. Con `take_gen` scritto con `for...of` e `break`, perché una seconda `take_gen(g, 3)` restituisce `[]`? Come si rimedia?
31. Perché la chiusura automatica di `for...of` (`return()` al `break`) è utile?
32. `map_gen` chiama `next()` su tutti gli iteratori e poi controlla `done`: cosa si perde? Come si "sbircia" un generatore?
33. `filter_gen(x => false, integers_from_gen(1))` e `stream_filter(x => false, integers_from(1))`: che differenza c'è?
34. Percorrendo un milione di elementi con `stream_to_gen(integers_from(1))`, le celle lette sono liberabili? E con `stream_to_gen(primes)`?
35. Se `gen_to_stream` usasse un `cons_stream` senza memo, cosa darebbe la seconda `take(s, 5)` su un generatore 1, 2, 3, …?
36. Dopo `s = gen_to_stream(g)` e `take(s, 3)` qualcuno chiama `g.next()`: cosa dà `take(s, 5)`?
37. Quanto è profonda `sieve_gen` per arrivare al 1000° primo? E lo stream `sieve`?
38. `iterate` scritto con `const next = f(x); return cons_stream(x, () => iterate(f, next))`: cosa cambia? Quale test fallisce?
39. Con `stream_map(n => right_split(p, n), integers_from(0))` invece di `iterate`, quanti passi `beside`/`below` servono fino al livello 10? Costruire un painter costa quanto disegnarlo?
40. In `cycle`, quando chiami `iter(0)` la variabile `stream` vale ancora `the_empty_stream`: perché l'ultima cella torna alla prima?
41. Cosa darebbe `cycle` con una `cons_stream` che valuta subito la coda? Ci sarebbe un `ReferenceError`?
42. Quanta memoria occupa `cycle`? E senza la memo di `cons_stream`?
43. Cosa si intende per "frame" in `play`? Quanti significati ci sono in gioco?
44. Cosa vuol dire "chiedere il frame successivo"? È ricorsione?
45. Perché `fakeClock` tiene una lista di callback in attesa invece di un posto solo?
46. Perché `play` fa avanzare più elementi in un frame invece di mostrare "un elemento ogni volta che passa un periodo" dall'ultimo disegno?
47. Perché `play` restituisce `() => void`? Dove hai già visto lo stesso schema?
48. Quali errori tipici si fanno scrivendo `play` (start, contatore, tempo relativo e assoluto)?
49. `play` riassegna `s`: cosa succede alla memoria con `play(iterate(...))` per un'ora? E se `play` facesse `const first = s`? E con `cycle`?
50. Come si ricomincia un'animazione senza leak?
51. `requestAnimationFrame` spinge, `play` tira: dove trovi la stessa coppia nel player audio?
52. Nella colla, `iterate(p => right_split(p, 2), painter)`: che cosa disegna l'elemento k?
53. Perché in `animation.ts` TypeScript dice che `ctx` può essere `null`, se `script.ts` lancia un errore quando lo è? Quale rimedio scegliere?
