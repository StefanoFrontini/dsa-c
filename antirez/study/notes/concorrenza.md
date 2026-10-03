# Thread, parallelismo e benchmark
Periodo: 2026-05-05 → 2026-07-06 · Turni: 103, 112-137, 179, 214-217, 227-229

## Mappa in breve
- Parte da un'intuizione algebrica (turno 103): visto che "a capo" è un'operazione associativa (monoide), una canzone enorme si può spezzare in due, parsare le metà in parallelo e poi unire i due risultati (MapReduce).
- Diventa `chord-pratt/chord-pratt-threads.c`: 2 pthread, split su `\n`, reduce nel main, refcount da sistemare dopo il merge (turni 114-122).
- Poi si passa a misurare: `clock_gettime(CLOCK_MONOTONIC)`, script bash + `bc` per media e deviazione standard, CSV (turni 120-126, 136).
- Lo stress test a 200.000 righe porta allo stack overflow e alle versioni iterative di `evalAST`/`freeObject` (turni 128-135, dettagli in `pratt-alberi.md`).
- Nella seconda parte (audio player) i thread servono per un altro motivo: non per andare più veloci ma per non bloccare l'audio mentre `recv` aspetta la rete (turno 179). Arrivano mutex e condition variable (esempi LLNL in `networking/`, poi `client_http.c`, turni 227-229).
- Collegamenti: `algebra-fp.md` (monoide), `memoria-c.md` (ownership, mmap), `pratt-alberi.md` (DFS iterativa), `actor-model.md` (thread pool vs event loop, kqueue), `audio.md` (ring buffer).

## Concetti

### Thread POSIX: create, join e passaggio del contesto
- Cos'è: `pthread_create(&t, attr, fn, arg)` fa partire `fn(arg)` su un nuovo thread. `pthread_join(t, &ret)` aspetta che quel thread finisca (è la "barriera"). Tutti i thread vedono lo stesso heap e le stesse globali, ma ognuno ha il **suo stack**. L'unico canale di ingresso è un `void *arg`, quindi in pratica gli passi il puntatore a una struct di contesto.
- Nel tuo codice:
  - `chord-pratt/chord-pratt-threads.c:798-805`: `parse(void *arg)` fa il cast a `CpCtx *` e scrive il risultato in `ctx->ast`. Il risultato quindi torna indietro tramite il contesto e non tramite il valore di ritorno: `pthread_exit(NULL)` va bene, ma bastava un semplice `return NULL`.
  - `chord-pratt-threads.c:843-856`: `ctx1`/`ctx2` stanno sullo stack del main e i thread li ricevono con `&ctx1`. Funziona perché il main fa join prima che quelle variabili spariscano.
  - `networking/client_http.c:1012-1013`: lì il contesto è già un puntatore all'heap, quindi si passa `(void *)ctx` **senza `&`**.
- Errori/bug: turno 228. `pthread_create(..., (void *)&ctx)` con `ctx` già di tipo `Ctx *` passa un `Ctx **`, e il thread va in segfault appena legge `ctx->parser...`. La regola: l'argomento deve avere esattamente il tipo a cui il thread fa il cast.
- Dettaglio verificato su questa macchina: lo stack del main è di 8 MB, quello di un thread secondario di **512 KB** (`pthread_get_stacksize_np`). Una ricorsione profonda dentro un worker esplode prima che nel main.
- Turni: 112, 113, 114, 228

### Split dell'input su un confine di riga
- Cos'è: non puoi tagliare il buffer a metà esatta in byte (`[Cm|aj7]`). Si parte dalla metà e si va avanti fino al primo `\n`. Il thread 2 deve ripartire da uno stato "pulito" del lexer.
- Nel tuo codice: `chord-pratt-threads.c:777-794`. Il `\n` diventa `0` per chiudere `buf1` (riga 789). `buf2` è una copia di `len - index` byte, così porta con sé anche il terminatore `buf[len]=0` messo dal main (riga 829). Entrambi i contesti partono con `initContext` → `STATE_START_SONG` (righe 846-847).
- Decisioni:
  - Turno 114: inizialmente il thread 2 partiva con `initContext2` (`STATE_EMPTY_LINE`). La radice sinistra diventava una LINE e `evalAST` infilava le righe dentro una riga.
  - Turno 118: la soluzione è far produrre una SONG a entrambi e fare un merge "a concatenazione".
- Errori/bug:
  - Turni 114-115: Stefano pensava che il `\0` del main venisse copiato. Con `memcpy(..., len-(index+1))` invece restava fuori, e `malloc` non azzera la memoria. Corretto al turno 119.
  - **Bug ancora presente (verificato)**: se dopo la metà non c'è nessun `\n` (file di una riga, file vuoto, ultima riga più lunga di metà file), `bufs.buf2` resta `NULL` e il thread 2 fa segfault in `advanceLexer` (`chord-pratt-threads.c:312`, `p` NULL; exit 139 con `--multi`).
  - **Differenza di output (verificata)**: il `0` messo al posto del `\n` fa vedere al thread 1 un finto EOF. Il lexer inietta la WORD vuota di fine file (`STATE_EMPTY_CHORD_ENDING`, righe 330-333), quindi l'ultima riga della prima metà ha una `<word>empty_chord/empty_lyric</word>` in più rispetto al single-thread (`diff` dell'XML su `test.txt` e `test2.txt`). L'elemento "neutro" iniettato a EOF non è davvero neutro per la lista di parole.
- Turni: 112, 114, 115, 118, 119, 122

### MapReduce grazie al monoide
- Cos'è: se l'operazione che unisce i pezzi è associativa (`(A∘B)∘C = A∘(B∘C)`), puoi calcolare i pezzi in modo indipendente (map) e combinarli alla fine (reduce). Nel tuo caso ∘ è la concatenazione delle liste di righe.
- Nel tuo codice: la map sono i due `parse` in parallelo (`chord-pratt-threads.c:851-852`). Il reduce è `chord-pratt-threads.c:861-866`: le LINE di `result2` vengono travasate in `result1`.
- Decisioni:
  - Turno 118: `listPushObj(result1, result2)` mette una SONG dentro una SONG (inserimento). Il vero operatore del monoide invece è la concatenazione, cioè lo "svuotare il secondo cesto nel primo".
  - Turno 127: non è il Pratt in sé a rendere il problema parallelizzabile. È il fatto che il linguaggio non ha stato tra una riga e l'altra (vedi errori di Gemini).
- Errori/bug: turno 119, `(*result2->list.ele) + i` è aritmetica su `PObj *` e non su `PObj **`. Corretto in `result2->list.ele[i]`.
- Turni: 103, 112, 118, 119, 127

### Ownership dopo il merge di due AST
- Cos'è: dopo il reduce ogni LINE del thread 2 ha due "padri": la SONG `result2`, che resta raggiungibile da `ctx2.ast`, e la SONG `result1`. Il refcount deve rispecchiarlo. Quindi si fa un `retain` per ogni riga trasferita, e non sul contenitore.
- Nel tuo codice:
  - `chord-pratt-threads.c:865` `retain(line)`. Poi `release(ctx1.ast); release(ctx2.ast);` alle righe 879-880.
  - Conteggio di una LINE del thread 2: 1 (nodo INFIX) → 2 (`evalAST`, riga 766) → 3 (reduce). Scende a 2, poi a 1, poi a 0 con le due release. `leaks --atExit` dà 0 leak su test2, su 200k e su 600k `--multi`.
- Decisioni/alternative:
  - Turno 114: `retain(result2)` dopo `listPushObj(result1,result2)`.
  - Turno 119: `retain(result2)` sul contenitore lascia `result2` a refcount 1 per sempre, quindi leak. La scelta finale è il retain sulle singole righe.
- Lezione:
  - Turno 116: Stefano ha ragione sul fatto che `release(ctx1.ast)` raggiunge `result2`. Il problema però è la doppia proprietà: senza retain, il secondo albero libererebbe di nuovo la stessa memoria (double free).
  - Turno 117: i nodi INFIX del thread 2 sono "impalcatura" che punta verso il basso. `result1` non li vede, quindi senza `release(ctx2.ast)` restano in memoria (leak).
- Turni: 114, 116, 117, 118, 119, 121, 122

### Race condition e memoria condivisa
- Cos'è: c'è una race quando due thread accedono alla stessa memoria, almeno uno scrive e non c'è sincronizzazione. In C è undefined behavior, non solo "un valore sbagliato".
- Nel tuo codice:
  - Il parser è *embarrassingly parallel*: ogni thread ha il suo `CpCtx`, il suo buffer e i suoi nodi. Condivisi ci sono solo `rulesTable`, in sola lettura, e `malloc`, che è thread-safe. `cc -fsanitize=thread` su `chord-pratt-threads.c --multi` (test2, stress_test) **non segnala nessuna race**.
  - `networking/mutex_example.c:67-69`: la somma condivisa è protetta da un mutex. Togliendo lock e unlock (copia in scratchpad), TSan riporta `data race ... Write of size 8 ... dotprod mutex_race.c:68`. Il risultato resta 400 per caso: la race c'è anche se l'output sembra giusto.
- Lezione: la race non si vede dall'output, serve TSan. Su macOS TSan **si blocca** se il `main` termina con `pthread_exit(NULL)`, come fanno gli esempi LLNL: sotto TSan conviene usare `return 0`.
- Turni: 112, 113, 114, 214, 229

### Misurare le prestazioni: tempo monotono, wall-clock e CPU time
- Cos'è:
  - `clock()` misura il **tempo CPU** sommato su tutti i thread: con 2 thread può sembrare che il multi sia più lento.
  - Per lo speedup serve il **wall-clock** con un orologio **monotono** (`CLOCK_MONOTONIC`, che non salta con NTP o con l'ora legale).
  - `time ./prog` mostra real, user e sys: real è il wall-clock, user+sys è il tempo CPU.
- Nel tuo codice: `chord-pratt-threads.c:838-874` (multi) e `892-911` (single). Il timer esclude la lettura del file e la `release` finale. Nel multi include split, create/join, `evalAST` delle due metà e il reduce.
- Decisioni: turno 121, scelta tra `#if THREADS` e modalità a runtime con argomento `--multi` (riga 813). La versione a runtime permette di avere un solo binario per lo script.
- Errori/bug: turno 121, `#define THREADS 0` + `#ifdef THREADS` è sempre vero, perché `#ifdef` controlla solo che la macro esista. Serve `#if THREADS`.
- Da sapere (verificato):
  - Su questo Mac `clock_getres(CLOCK_MONOTONIC)` = 1 µs. Su `test.txt` i tempi (10 µs) sono rumore.
  - La `release` non è misurata ma pesa: a 600k righe il processo impiega 2,56 s in totale contro circa 1,09 s misurati. Liberare l'AST costa più che costruirlo.
  - La memoria di picco è di circa 470 MB per un input di 11 MB.
- Turni: 120, 121, 122

### Script bash + bc: media e deviazione standard
- Cos'è: bash fa solo aritmetica intera, quindi per i decimali si passa la stringa a `bc -l`. Il ciclo `for` lancia il binario, `$(...)` cattura l'output, `grep` isola il numero. La deviazione standard dice quanto sono stabili le misure.
- Nel tuo codice: `chord-pratt/run_test.sh:12-49`, una funzione `run_benchmark` con `local` (refactoring suggerito al turno 125). Il CSV va in `benchmarks.csv`.
- Decisioni: turno 124, prima `grep "Execution Time:"` e poi `grep -Eo` sul numero, per non catturare altri decimali. `printf %.9f` rimette lo zero iniziale che `bc` omette.
- Errori/bug:
  - Turni 128 e 134: "Parse error: bad token / bad expression" **viene da `bc`**, non dal parser. Il programma C era crashato, `cur_output` era vuoto e `bc` riceveva `"0 + "`. Verificato: `echo "0.1 + " | bc -l` dà *bad expression*, `echo " + " | bc -l` dà *bad token*. Lo script non controlla che il binario sia uscito bene.
  - Lo script usa `./a.out`, ma l'`a.out` attuale in `chord-pratt/` è del 7 maggio e **non contiene** `--multi` né "Execution Time". Rilanciare `run_test.sh` così com'è produrrebbe errori di `bc`: va ricompilato prima.
  - La deviazione standard divide per N (popolazione) e non per N-1 (campione). Il primo run (warm-up) non viene scartato, anche se Gemini l'aveva consigliato al turno 120.
- Turni: 123, 124, 125, 128, 134

### Risultati dei benchmark
- I tuoi (M1, 8 GB):

  | righe | single (s) | multi (s) | speedup |
  |---|---|---|---|
  | 100k | 0,1342 ± 0,028 | 0,0834 ± 0,009 | 1,61 |
  | 100k (`old_benchmarks.csv`) | 0,1323 ± 0,006 | 0,0856 ± 0,008 | 1,55 |
  | 200k | 0,2750 ± 0,008 | 0,1715 ± 0,007 | 1,60 |
  | 600k (`benchmarks.csv`) | 0,8440 ± 0,051 | 0,5649 ± 0,039 | 1,49 |

  Fonti: turno 126 (prima riga 100k), turno 136 (200k e 600k).
- Rieseguiti qui (i7-6700K Intel, 4 core / 8 thread, 10 run, stesso script con path del binario cambiato):

  | flag | righe | single (s) | multi (s) | speedup |
  |---|---|---|---|---|
  | -O0 | 100k | 0,312 ± 0,010 | 0,221 ± 0,007 | 1,41 |
  | -O0 | 600k | 2,02 ± 0,31 | 1,37 ± 0,05 | 1,47 |
  | -O2 | 100k | 0,282 ± 0,014 | 0,171 ± 0,010 | 1,65 |
  | -O2 | 600k | 1,77 ± 0,18 | 1,07 ± 0,22 | 1,66 |

  L'Intel è circa 2,3× più lento in assoluto, ma lo speedup è nello stesso intervallo (1,4-1,7). Il trend "multi più veloce di circa il 35-40%" è confermato.
- Fasi misurate (-O0, 100k, `--multi`):

  | fase | tempo (s) |
  |---|---|
  | split | 0,0005 |
  | parse nei thread | 0,256 |
  | `evalAST` seriale nel main | 0,060 |
  | reduce | 0,006 |

  Spostare `evalAST` dentro i thread (variante in scratchpad) migliora solo del 5% circa (0,221 → 0,210). Il grosso della perdita rispetto a 2× sta nella fase parallela stessa: parse a ~75% di efficienza, probabilmente per la contesa su `malloc` e la banda di memoria (ipotesi, non misurata).
- Turni: 126, 136

### Legge di Amdahl e algoritmi parallelizzabili
- Cos'è: se una frazione *s* del lavoro resta seriale, lo speedup massimo con N thread è `1 / (s + (1-s)/N)`, e non supera mai `1/s`. Speedup S = Ts/Tp, efficienza E = S/N.
- Nel tuo codice: le parti seriali sono split, `evalAST` delle due metà (righe 858-859, **nel main, dopo il join**) e reduce (861-866).
- Tassonomia del turno 127, corretta:
  - (1) Problemi divisibili senza stato condiviso, come le righe indipendenti con unione associativa.
  - (2) Problemi con stato condiviso protetto da lock: la sezione critica torna seriale.
  - (3) Problemi con dipendenza passo-passo (catene di dipendenze).

  Il punto chiave è il **linguaggio/problema**, non la tecnica: un recursive descent sullo stesso linguaggio a righe si parallelizza allo stesso modo.
- Turni: 120, 126, 127, 136

### Il bug delle 200.000 righe (stack overflow)
- Cos'è: l'albero del Pratt è una lunga "spina" a sinistra (gli operatori sono associativi a sinistra). `eval`/`freeObject` ricorsivi fanno una chiamata per riga, e oltre gli 8 MB di stack del main il programma va in segfault.
- Nel tuo codice: `evalAST` iterativo con stack esplicito pre-allocato (`chord-pratt-threads.c:737-770`). `freeObject` iterativo sulla spina sinistra (`:361-413`, la vecchia versione ricorsiva è commentata alle righe 414-440).
- Verificato qui: rimettendo la `freeObject` ricorsiva (copia in scratchpad), 200k righe in single-thread vanno in **exit 139**, mentre con `--multi` passano (ogni metà ha 100k). Con la versione attuale 200k e 600k girano in entrambe le modalità, senza leak.
- Errori lungo la strada:
  - Turno 132: `PObj *stack = xmalloc(sizeof(PObj)*i)` e `acc = xrealloc(acc, i)`, cioè strutture al posto di puntatori.
  - Turno 133: `if(count=0)`, un'assegnazione: con `count` azzerato si alloca 0 byte e si va in heap overflow.
  - Turno 135: la vera causa residua era `freeObject` ricorsiva.
- Turni: 128, 129, 130, 131, 132, 133, 134, 135 (approfondimento: `pratt-alberi.md`)

### mmap per file grandi
- Cos'è: `mmap` mappa il file nello spazio di indirizzi. Le pagine vengono caricate on demand (page fault) e, essendo file-backed e pulite, il kernel può scartarle se manca memoria. L'alternativa portabile è il buffer a finestra: leggi un chunk, parsi fino all'ultimo `\n`, sposti il resto con `memmove` e rileggi.
- Nel tuo codice: oggi c'è `fread` di tutto il file in `buf` (`chord-pratt-threads.c:824-830`). mmap non è stato implementato.
- Limiti verificati/ragionati:
  - Il tuo lexer si ferma su `c == 0` e `split` scrive `0` dentro il buffer. Con mmap il file non finisce con `\0` e serve `MAP_PRIVATE` + `PROT_WRITE`.
  - Soprattutto, l'AST occupa circa 470 MB per 11 MB di input: mmap farebbe risparmiare il 2%.
- Turni: 137 (vedi anche `memoria-c.md#mmap`)

### Mutex
- Cos'è: `pthread_mutex_lock`/`unlock` delimitano una **sezione critica**, in cui entra un thread alla volta. La regola d'oro: dentro il lock solo operazioni brevi in memoria, mai I/O di rete.
- Nel tuo codice:
  - `networking/mutex_example.c:58-69`: ogni thread somma in locale (`mysum`) e prende il lock **una sola volta** per aggiornare `dotstr.sum`. È lo schema giusto: tanto lavoro privato, sezione critica minima.
  - `networking/client_http.c:579-581`: invece qui il lock viene preso e rilasciato **per ogni singolo byte** accodato nel ring buffer. È corretto ma costoso: meglio scrivere un blocco e prendere il lock una volta.
- Decisioni/bug:
  - Turno 229: Gemini segnala `fetch(ctx)` dentro il mutex (bloccava l'audio per tutto il download). Nella versione attuale `fetch` è fuori dal lock (`client_http.c:784-788`).
  - Turno 216: nel thread pool il mutex protegge il contatore "prossimo chunk da scaricare".
- Turni: 113, 214, 216, 227, 229

### Condition variable
- Cos'è: permette a un thread di **dormire finché una condizione su dati condivisi diventa vera**, senza consumare CPU. Si usa sempre con lo stesso mutex che protegge quei dati:

  ```
  lock;
  while (!cond) wait(cv, m);
  ... ;
  unlock
  ```

  `pthread_cond_wait` rilascia il mutex in modo atomico e lo riprende al risveglio. Il `while` serve perché esistono risvegli spuri e perché un altro thread può "rubare" la condizione prima.
- Nel tuo codice:
  - `networking/condition_variables.c:60-67`: `watch_count` aspetta con `while (count < COUNT_LIMIT)`. Il `while` copre anche il caso in cui la soglia sia già stata raggiunta prima che inizi ad aspettare (signal perso).
  - `inc_count` fa signal tenendo il lock (`:33-36`).
  - Nel player: `read_packet` aspetta con `while(1){ if (available>0) break; pthread_cond_wait(...) }` (`client_http.c:337-351`) e poi segnala `network_threshold_cv` (`:357`). Il network thread aspetta finché il buffer è sopra soglia (`:774-783`).
- Errori/bug:
  - Turno 229: `if` al posto di `while` attorno a `pthread_cond_wait`. È ancora presente in `decode()` (`client_http.c:815-816`). L'impatto è basso perché `read_packet` poi ricontrolla.
  - Il network thread segnala `audio_buffer_threshold_cv` solo **alla fine** di tutta la `fetch` (`:799-801`), quindi un `read_packet` a buffer vuoto aspetta l'intero download anche se i byte sono già arrivati.
  - Turno 227: SDL non espone un mutex POSIX sulla sua coda, quindi per la coda SDL si fa polling temporizzato.
- Turni: 214, 227, 229

### `recv` bloccante: perché un thread di rete separato
- Cos'è: `recv` blocca il thread finché non arrivano byte. Con un solo thread, un buco di rete ferma anche la decodifica e la UI. Con un network thread separato si blocca solo lui, mentre l'audio thread continua a consumare il ring buffer. Qui i thread servono a **disaccoppiare tempi diversi** (rete imprevedibile contro audio a ritmo fisso), non a calcolare più in fretta.
- Nel tuo codice: `client_http.c:760-806` (`get_data`, produttore) e `:812-` (`decode`, consumatore), con il ring buffer protetto da `audio_buffer_mutex`.
- Turni: 179, 227 (vedi `audio.md`)

### Thread pool vs event loop
- Cos'è: per scaricare 99 chunk non servono 99 thread. Le due alternative:
  - **Thread pool**: K worker che pescano un indice da una "lavagna" protetta da mutex e scrivono ognuno nel proprio slot. L'ordine è garantito dall'indice e non dall'arrivo.
  - **Event loop**: un solo thread con socket non bloccanti e `kqueue` (macOS) o `epoll` (Linux), cioè l'analogo di `Promise.all`.
- Nel tuo codice: non implementato. Il player attuale ha 2 thread fissi (`client_http.c:1000-1017`).
- Decisione (turno 216): partire dal thread pool per fare pratica con i mutex, poi passare all'event loop in vista del broker MQTT.
- Turni: 214, 215, 216, 217 (approfondimento: `actor-model.md`)

## Possibili errori o imprecisioni di Gemini
- **Turno 113**:
  - Consiglia i thread C11 (`<threads.h>`) di Beej "compilando con `-lpthreads`". Su macOS `<threads.h>` **non esiste** (verificato: `fatal error: 'threads.h' file not found`), quindi bisogna usare i pthread.
  - Il flag si chiama `-lpthread` e su macOS non serve.
- **Turno 125**: "bc senza `-l` tronca `$DIFF^2` a 0, quindi la deviazione standard è sempre 0". Nel tuo script è falso. `DIFF` eredita la scale 20 dalla `MEAN` calcolata con `-l`, e `^` mantiene la scale della base. Verificato: `.00708301000000000000^2` dà lo stesso risultato con e senza `-l`. Mettere `-l` resta comunque una buona abitudine.
- **Turno 126**: costruisce una spiegazione ("il multi è 3 volte più stabile perché lo scheduler non lo interrompe") su un solo run. In `old_benchmarks.csv` (stesso file da 100k) è il contrario: single ±0,0058, multi ±0,0082. Inoltre, tra le parti seriali dimentica la `evalAST` delle due metà fatta nel main dopo il join.
- **Turno 127**:
  - "Il recursive descent è intrinsecamente sequenziale / P-completo; con il primo codice sarebbe stato teoricamente impossibile". Non è vero: si parallelizza perché il linguaggio è fatto di righe indipendenti, e qualunque parser può lavorare sulle due metà tagliate a `\n`. La P-completezza riguarda i problemi, non una tecnica di parsing.
  - "Game of Life intrinsecamente sequenziale" è sbagliato: dentro una generazione ogni cella è indipendente, ed è l'esempio classico di calcolo parallelo/GPU.
- **Turno 131**: attribuisce il blocco a `realloc` +1 in O(N²), con "1,25 miliardi di copie" e swap. Smentito: la versione esatta del turno 130, ricompilata, gira 50k righe in 0,15 s e 600k in 1,98 s senza bloccarsi.
- **Turno 134**: dice che nel codice c'è `while (stack->list.len)` + `listPopObj(stack)` su un `PObj **`. Nel codice incollato al turno 133 quel ciclo usa già `j`: il bug è inventato. La causa vera, `freeObject` ricorsiva, arriva al turno 135.
- **Turno 136**: "l'efficienza scende a 600k perché il reduce cresce, Amdahl". Il reduce cresce in proporzione all'input, quindi la frazione seriale resta costante e Amdahl prevede lo stesso speedup. Misurato qui: il reduce vale circa il 2% sia a 100k sia a 600k. La differenza 1,60 → 1,49 è compatibile con il rumore o con effetti di memoria.
- **Turno 137**: "mmap: zero modifiche al codice, il SO libera automaticamente le vecchie pagine".
  - Il buffer mappato non ha il `\0` finale su cui si basa il lexer, e `split` scrive nel buffer.
  - Le pagine vengono scartate solo sotto pressione di memoria.
  - Non dice che l'AST pesa circa 40 volte l'input (≈470 MB per 11 MB), quindi il guadagno di mmap è marginale.
- **Turno 214**: "99 thread consumerebbero quasi 1 GB di RAM (2-8 MB di stack l'uno)". Lo stack è memoria virtuale riservata e le pagine vengono allocate solo quando servono. Su macOS lo stack di un thread secondario è di 512 KB (verificato). Il motivo vero per non usare 99 thread è il carico sul server e la gestione, non la RAM.
- **Turni 215-216**:
  - Il codice d'esempio usa `epoll`, che esiste solo su Linux; su macOS serve `kqueue`.
  - "4 thread per non saturare la CPU dell'M1": i download sono I/O-bound, e il numero di worker dipende dalla rete e dal server, non dalla CPU.
  - Il design "scarica tutti i 99 chunk, poi suona" contraddice lo streaming live.
- **Turno 229**: "leggere un int a 32 bit senza lock: il valore può corrompersi a livello di CPU". Un int allineato non si legge "a metà" né su x86 né su ARM. Il problema reale è che una data race in C è undefined behavior (il compilatore può riordinare o tenere il valore in un registro) e che il valore letto può essere vecchio. La conclusione (usare il lock) resta corretta.

## Domande aperte / cose non chiarite
- Qual era davvero la causa del blocco "gira a vuoto per minuti" del turno 131? Non è riproducibile con il codice del turno 130. Forse un `a.out` vecchio, o un problema nello script.
- Perché lo speedup si ferma a 1,4-1,7 con 2 thread su macchine con 4+ core? Ipotesi da verificare con Instruments: contesa nell'allocatore (`malloc` per ogni token/nodo), banda di memoria, `evalAST` seriale.
- Con N thread (non 2) come scala? Lo split oggi è fisso a metà; generalizzarlo a N richiede N confini di riga e un reduce a N vie.
- Conviene misurare anche la `release`, che a 600k righe costa più del parsing?
- Il benchmark dovrebbe scartare il primo run e usare la deviazione standard campionaria (N-1)?
- `client_http.c`: il `while(1)` del network thread non termina mai, quindi `pthread_join` e la pulizia finale (`:1016-1026`) sono irraggiungibili. Inoltre `network_threshold_cv` non viene mai distrutta.

## Argomenti a margine
- Risorse per i thread: Beej's Guide to C (cap. multithreading, C11 threads), guida IPC di Beej (processi, `fork`), tutorial LLNL POSIX Threads (da qui vengono `mutex_example.c` e `condition_variables.c`) · turno 113. Processi e thread: `fork` copia lo spazio di memoria, i thread lo condividono.
- MIT 6.824 Distributed Systems: MapReduce è la prima lezione · turno 119.
- Imparare bash da sviluppatore: Learn X in Y minutes, devhints · turno 123.
- Generare uno stress test concatenando un file con un ciclo bash (`cat >>`) · turno 122.
- Elaborazione streaming con un buffer a finestra scorrevole (`memmove` del residuo e refill) · turno 137.
- Flag del linker macOS `-Wl,-stack_size,0x2000000` per avere uno stack più grande nel main · turno 128.

## Domande di autoverifica
1. Cosa fa `pthread_join` e cosa succederebbe se il main usasse `ctx1.ast` senza averlo chiamato?
2. Perché a `pthread_create` si passa `ctx` e non `&ctx` quando `ctx` è già un `Ctx *`? Che tipo arriva al thread nel secondo caso?
3. Perché `clock()` non va bene per misurare lo speedup di un programma multithread? Cosa misurano real, user e sys di `time`?
4. Perché `#define THREADS 0` seguito da `#ifdef THREADS` compila comunque il ramo multithread?
5. Perché lo split cerca un `\n` invece di tagliare a `len/2`? Cosa succede oggi nel tuo codice se il file è una sola riga?
6. In `split`, perché `memcpy(buf2, buf+index+1, len-index)` copia anche il terminatore e la versione con `len-(index+1)` no?
7. Perché nel reduce si fa `retain(line)` per ogni riga e non `retain(result2)`? Ricostruisci il refcount di una LINE del thread 2 dalla creazione al free.
8. Qual è la differenza tra inserire `result2` in `result1` e concatenare le due liste? Quale delle due è l'operazione del monoide?
9. Lo script stampa "Parse error: bad token": chi lo emette e cosa ti dice sul binario C?
10. Con speedup 1,6 su 2 thread, qual è l'efficienza? Se la parte seriale fosse il 10%, quale sarebbe lo speedup massimo con infiniti thread?
11. Perché il parser a righe è parallelizzabile mentre un linguaggio con commenti `/* ... */` su più righe non lo sarebbe con lo stesso split? Che cosa dovrebbe sapere il thread 2?
12. Perché la versione multi produce una `<word>` vuota in più rispetto alla single? Come la correggeresti senza toccare il lexer del single-thread?
13. Perché `pthread_cond_wait` va messo dentro un `while` e non dentro un `if`? Cosa fa al mutex mentre il thread dorme?
14. Nel player, perché fare la `fetch` di rete dentro il mutex del ring buffer blocca l'audio? Cosa deve stare dentro la sezione critica?
15. Per 99 download paralleli: thread pool con K worker o event loop con kqueue? Quali sono i costi e i rischi di ciascuno, e perché 99 thread sono una cattiva idea anche se la RAM non è il problema principale?
