# Memoria in C: ownership e reference counting
Periodo: 2026-03-14 → 2026-06-30 · Turni: 1-2, 24-26, 31, 37, 48, 74-83, 96-99, 110-111, 114-118, 137, 168, 207-212 (+ chat separata `snake/snake-gemini-chat.md`, senza numero di turno)

## Mappa in breve
- È il filo che attraversa tutti i progetti: toyforth (turni 1-2) → shunting-yard (24-26) → chord parser (37, 48) → Pratt (74-99) → parser multithread (110-118) → audio player (168, 207-212).
- Le tre domande da farsi sempre: **dove vive questo dato** (stack, heap, area statica, `.rodata`)? **quanto deve vivere** (oltre la funzione che lo crea?)? **chi lo possiede** (chi deve chiamare `release`/`free`)?
- Evoluzione: prima *tutto nell'heap con refcount* (toyforth, shunting-yard), poi *token per valore e contesto sullo stack* (Pratt, turni 75-83), poi *strutture enormi nell'heap con `calloc`* (player, turni 208-210).
- Collegamenti: la parte parsing (Pratt, shunting-yard, monoidi) e quella thread/MapReduce sono aree a parte. La caccia ai leak del player (turni 236-242, con `leaks`) la copre un'altra area: qui c'è solo il metodo.
- Strumenti verificati su questo Mac: `leaks --atExit -- ./prog`, `ulimit -s` (8 MB sul main thread), thread secondari a 512 KB.

## Concetti

### Puntatore sullo stack, oggetto nell'heap
- Cos'è: `TfObj *a` è una variabile locale di 8 byte che contiene un indirizzo. Quando la funzione ritorna, sparisce il **puntatore** ma non l'**oggetto** nell'heap. Il C non ha un distruttore automatico: se nessuno chiama `free`/`release`, l'oggetto resta "orfano" (leak).
- Nel tuo codice: `toyforth/toyforth.c:426-441`: `a` e `b` escono da `ctxStackPop` e vengono liberati con `release(a); release(b);`. `chord-pratt/chord-pratt.c:740`: `PObj *result = evalAST(ctx.ast)` è solo un puntatore che "guarda" un nodo dell'albero.
- Decisioni: nel turno 111 hai tolto `retain(result)`/`release(result)` dal main. Era giusto, ma il motivo non è che "il puntatore è sullo stack": `result` è un **osservatore** di un nodo posseduto da `ctx.ast`, che viene liberato a cascata (`chord-pratt/chord-pratt.c:748`).
- Errori/lezione: la domanda del turno 1 ("i puntatori sullo stack vengono distrutti, perché devo fare release?") è il malinteso classico. Distruggere il puntatore ≠ liberare la memoria.
- Turni: 1, 111

### Reference counting: retain / release
- Cos'è: ogni oggetto ha un contatore `refcount` = numero di proprietari. Nasce a 1 (`createObject`). `retain` = +1 (un nuovo proprietario), `release` = -1 e, se arriva a 0, `freeObject` libera l'oggetto e fa `release` sui figli (distruzione a cascata).
- Nel tuo codice: `toyforth/toyforth.c:93-98` (nasce a 1), `toyforth/toyforth.c:129-137` (retain/release con `assert(refcount > 0)`), `toyforth/toyforth.c:113-127` (freeObject ricorsivo). Stesso schema in `math-shunting-yard/math-shunting-yard.c:88-111` e `chord-pratt/chord-pratt.c:396-433`. Versione "antirez" con header nascosto prima della stringa: `pls.c:17-81` (`ps_retain`/`ps_release` + `magic` 0xDEADBEEF per beccare use-after-free).
- Decisioni: `freeObject` deve chiamare `release` sui figli, **non** `freeObject` (bug trovato nel turno 74 e ancora presente su `NEGATION` nel turno 77). Altrimenti salti il conteggio e liberi figli condivisi.
- Errori/lezione: **bug ancora presente** in `toyforth/toyforth.c:115-120`: il caso `LIST` fa `release` degli elementi ma non `free(o->list.ele)`. Verificato con `leaks`: anche aggiungendo la pulizia finale restano 2 leak (gli array `ele`). In shunting-yard e chord-pratt il `free(o->list.ele)` c'è.
- Turni: 1, 2, 24, 74, 77

### Trasferimento vs condivisione dell'ownership
- Cos'è: quando sposti un puntatore da una struttura a un'altra, ti chiedi se il vecchio proprietario lo tiene ancora.
  - **Trasferimento** (pop da A e push in B, oppure `create` + push): il refcount non cambia, quindi niente retain/release.
  - **Condivisione** (A lo tiene e anche B lo tiene): serve `retain`.
- Nel tuo codice:
  - trasferimento: `toyforth/toyforth.c:442` `ctxStackPush(ctx, createIntObject(result))`, `math-shunting-yard/math-shunting-yard.c:566-573` (dallo stack alla coda).
  - condivisione: `toyforth/toyforth.c:413-415` `exec` fa `retain(word)` perché `word` resta anche nella lista `prg`. `chord-pratt/chord-pratt-threads.c:862-866` REDUCE: `listPushObj(result1, line); retain(line);`.
- Decisioni: nel turno 24 nel main facevi `release(popped)` + enqueue senza retain. Così il refcount non contava la coda e si arrivava a un double free. La correzione è spostare senza toccare il contatore.
- Errori/lezione:
  - Turno 116: unire `result2` dentro `result1` senza `retain` dà *doppia proprietà non contata*. Se poi rilasci entrambi gli alberi, hai un double free; se non rilasci `ctx2.ast`, hai un leak dei nodi INFIX (turno 117).
  - Regola pratica: fai sempre **retain prima di release** quando sposti. Il tuo `release(popped); ...; retain(popped)` del turno 24 funzionava solo perché `parsed` teneva un'altra referenza.
- Turni: 2, 24, 26, 114, 116, 117, 118

### Convenzione "retain fuori dalle utility"
- Cos'è: le funzioni di basso livello (`listPush`, `ctxStackPush`, `ctxEnqueue`) muovono solo puntatori e non toccano mai il refcount. È il chiamante a decidere se fa un trasferimento (niente) o una condivisione (`retain`). Così ogni `retain` si vede nel codice di alto livello.
- Nel tuo codice: commento-contratto in `toyforth/toyforth.c:213-214` e `math-shunting-yard/math-shunting-yard.c:136-137` ("It is up to the caller..."). `math-shunting-yard/math-shunting-yard.c:359-366` `evalInfix` fa `retain(o)` **prima** di `ctxEnqueue`/`ctxStackPush`.
- Decisioni: nel turno 24 le push erano asimmetriche (alcune facevano retain dentro, altre no). Nel turno 25 hai scelto la convenzione "sempre fuori".
- Errori/lezione: c'è un'eccezione rimasta. `ctxStackPush` rilascia internamente le parentesi (`math-shunting-yard/math-shunting-yard.c:299` e `:306`). Funziona, ma rompe la convenzione: documentalo nel commento della funzione.
- Turni: 24, 25, 26

### Wrapper xmalloc / xrealloc
- Cos'è: `malloc` può restituire NULL. I wrapper `xmalloc`/`xrealloc` controllano il risultato e chiudono il programma con un messaggio. Il resto del codice non deve più controllare NULL (stile antirez/Redis `zmalloc`).
- Nel tuo codice: `toyforth/toyforth.c:71-87`, copiato in tutti i parser. Nel player invece `calloc` + controllo esplicito (`networking/client_http.c:984-989`).
- Decisioni: per programmi CLI "muori subito" va bene. Per una libreria (es. il futuro modulo wasm) conviene restituire un errore al chiamante.
- Errori/lezione: negli esercizi base `malloc` non è controllato (`bst.c:11`, `struct.c:10` sì, `tac.c:30`).
- Turni: 1 (codice), presente in tutti i turni con codice C

### Chi possiede e chi osserva (borrowing)
- Cos'è: un puntatore può essere *proprietario* (deve liberare) o *osservatore* (usa, ma non libera e non deve sopravvivere al proprietario). Il refcount conta solo i proprietari.
- Nel tuo codice: `chord-pratt/chord-pratt.c:671-686`: `evalAST` restituisce un nodo **già dentro** l'albero (il primo accumulatore a sinistra) e lo modifica (`listPushObj(a, b); retain(b);`). L'AST diventa un grafo con nodi condivisi (refcount 2), e `release(ctx.ast)` libera tutto. Verificato con `leaks`: 0 leak.
- Decisioni: `retain(result)` serve solo se vuoi che `result` sopravviva a `release(ctx.ast)` (turno 111).
- Errori/lezione: `evalAST` **muta** l'AST. Se la chiami due volte duplichi le righe. È un effetto collaterale da documentare.
- Turni: 111, 116, 117

### Regola di scelta stack vs heap
- Cos'è:
  - Stack: veloce, automatico, ma muore con la funzione e ha un limite (8 MB per il main su macOS, 512 KB per i pthread secondari: verificato).
  - Heap: vive finché fai `free`, la dimensione può essere decisa a runtime, ma va gestito a mano.
  - Regola: **(1)** deve sopravvivere alla funzione? Allora va nell'heap (o nel chiamante). **(2)** È grande o di dimensione ignota? Allora va nell'heap. **(3)** Altrimenti stack.
- Nel tuo codice:
  - stack: `Token curToken` per valore dentro il Lexer (`math-pratt/math-pratt.c:58`), `pCtx ctx` nel main (`math-pratt/math-pratt.c:646`).
  - heap: i nodi AST, perché vivono oltre `parseInfix`, e `Ctx` del player da 15 MB (`networking/client_http.c:984`).
- Decisioni: turno 75, i nodi AST restano nell'heap (lifetime più lungo della funzione che li crea). Turno 82, il contesto (pochi byte) va sullo stack del main.
- Errori/lezione: "stack per il controllo, heap per i dati grossi" (turno 209).
- Turni: 75, 82, 209, 210

### Dangling pointer verso lo stack
- Cos'è: restituire o salvare l'indirizzo di una variabile locale (`return &x`, `ctx->table = local_table`, `token.str.ptr = str_locale`). Dopo il `return`, quella zona dello stack viene riusata: il puntatore "pende".
- Nel tuo codice: è il motivo per cui la tabella non va allocata sullo stack di `createContext` (turno 78) e per cui una stringa del lexer non può stare in un `char buf[256]` locale di `readChord` (turno 110).
- Decisioni: tabella → `static` globale. Stringhe → heap (`chord-pratt/chord-pratt.c:182-196`), oppure buffer dentro la struct del token, oppure zero-copy.
- Errori/lezione: un array fisso **dentro** una struct vive dove vive la struct (turno 78, soluzione 2).
- Turni: 75, 78, 110

### La trappola della copia per valore
- Cos'è: `Token t = ctx->lexer.curToken;` copia l'intera struct. Se poi modifichi `t`, l'originale resta uguale. Per modificare l'originale ti serve un puntatore (`Token *t = &ctx->lexer.curToken;`) o l'accesso diretto ai campi.
- Nel tuo codice: bug del turno 77 in `readSymbol`. Ora è corretto, con assegnazione diretta (`math-pratt/math-pratt.c:146-171`).
- Errori/lezione: in TypeScript gli oggetti si passano per riferimento, in C le struct si copiano. Il bug si vede solo per i simboli perché `readNumber` scriveva già direttamente nei campi.
- Turni: 77

### Tabelle statiche (lookup O(1))
- Cos'è: dati costanti noti a compile time (la grammatica del Pratt) in un array `static` globale con *designated initializers* `[TOKEN_PLUS] = {...}`. Vive nel segmento dati per tutto il programma: zero malloc, accesso per indice.
- Nel tuo codice: `math-pratt/math-pratt.c:103-112`, `chord-pratt/chord-pratt.c:165`. Prima c'era un array dinamico con `xrealloc` e una ricerca lineare (turno 74).
- Decisioni: i prototipi di `parsePrefix`/`parseInfix` vanno messi **prima** della tabella, e la tabella prima dei getter. Spostarla in fondo al file (consiglio di Gemini, turno 79) causa `undeclared identifier` (turno 80).
- Errori/lezione: "statico" in C ha due significati: *durata statica* (keyword `static`, vive per sempre) e *dimensione fissa* (`[9]`). Non confonderli (turno 78).
- Turni: 74, 78, 79, 80, 81

### Contesto sullo stack + initContext ("caller allocates, callee initializes")
- Cos'è: il chiamante dichiara `pCtx ctx;` sullo stack e passa `&ctx` a una funzione che lo inizializza. Niente malloc e niente `free(ctx)`: si libera solo ciò che il contesto possiede nell'heap (l'AST).
- Nel tuo codice: `math-pratt/math-pratt.c:569-580` (`initContext` con `memset` + `freeContext` che rilascia solo l'AST) e main `:646-647`. Stesso pattern in `chord-pratt/chord-pratt.c:383-389,728-730`. Due contesti sullo stack, uno per thread: `chord-pratt/chord-pratt-threads.c:843-847`.
- Decisioni: `memset(ctx, 0, sizeof *ctx)` per non lasciare campi con valori spazzatura (es. `curToken`). Alternativa idiomatica: `pCtx ctx = {0};`.
- Errori/lezione: funziona finché il contesto è piccolo. Con un ring buffer da 15 MB dentro (player) lo stesso pattern fa crashare il programma (vedi sotto).
- Turni: 82, 83

### String literal in memoria read-only
- Cos'è: `"emptyChord"` viene messa dal compilatore in `.rodata` (sola lettura), non sullo stack né nell'heap. **Scriverci** → crash (SIGBUS, exit 138 su questo Mac). **Farne `free`** → undefined behavior: l'allocatore di macOS se ne accorge e fa abort (SIGABRT, exit 134). Verificato con un test in build/.
- Nel tuo codice: per avere una stringa vuota "liberabile" usi `xmalloc(1)` + `ptr[0] = 0` (`chord-shunting-yard/chord-shunting-yard.c:162-163`, `chord-pratt/chord-pratt.c:478-479`). Così `freeObject` può sempre fare `free(o->str.ptr)` senza casi speciali.
- Decisioni: alternative valide: `strdup("")`, oppure un flag "owned" che dice se liberare. `char arr[] = "..."` invece **copia** la stringa sullo stack (modificabile).
- Errori/lezione: un oggetto deve avere memoria **omogenea**: o sempre heap (e quindi sempre `free`), o mai.
- Turni: 37

### malloc non azzera la memoria e il terminatore `\0`
- Cos'è: `malloc` restituisce byte con valori casuali. Se copi `n` caratteri con `memcpy` senza copiare o scrivere il `\0`, la stringa non è terminata e il lexer legge oltre il buffer. `calloc` azzera, `malloc` no.
- Nel tuo codice: `chord-pratt/chord-pratt-threads.c:777-793` `split()`. Ora la `memcpy` copia `len - index` byte, terminatore originale compreso (fix del turno 115). `issue1.c:38-47`: il dump mostra byte spazzatura dopo la stringa nella struct sullo stack.
- Errori/lezione: **bug ancora presente** in `split()`. Se dopo la metà del file non c'è nessun `\n`, `bufs->buf2` resta NULL e `initContext(&ctx2, NULL)` va in crash. Verificato: file di una riga con `--multi` dà exit 139 (segfault). Il fix spetta all'area parser/thread.
- Turni: 114, 115

### Zero-copy / string view
- Cos'è: invece di copiare ogni token, salvi solo `ptr` + `len` dentro il buffer del file, che vive per tutto il parsing. Per stamparlo: `printf("%.*s", (int)len, ptr)`. Elimina la "double allocation" (malloc nel lexer + malloc nel nodo AST).
- Nel tuo codice: **non adottato**. `readLyric`/`readChord` fanno `xmalloc` (`chord-pratt/chord-pratt.c:182-196`), `advanceLexer` fa `free` (`:290-295`) e `createLyricObject` ricopia (`:485-497`).
- Decisioni: alternativa a stringhe nello stack, che richiedono un limite rigido di lunghezza (turno 110, opzione 2).
- Turni: 110

### Arena allocator
- Cos'è: una sola grande `malloc`, poi le allocazioni sono "sposta un offset in avanti" (O(1)), e alla fine una sola `free` per tutto. È adatto quando tutti gli oggetti muoiono insieme (AST). Non si può liberare un singolo oggetto.
- Nel tuo codice: **mai implementato** (nessun `arena` nel repo). Gemini l'ha proposto più volte (turni 75, 76, 79).
- Decisioni: nel tuo chord-pratt `evalAST` condivide i nodi (refcount 2), quindi il refcount ha ancora senso. Con un'arena diventerebbe inutile, perché si libera tutto insieme.
- Errori/lezione: il codice d'esempio del turno 76 ignora l'**allineamento**. Una vera arena arrotonda l'offset a `_Alignof(max_align_t)`.
- Turni: 75, 76

### Buffer sullo stack nella ricorsione
- Cos'è: `char newPrefix[1024]` dentro una funzione ricorsiva occupa 1 KB per **ogni livello**. Un albero profondo porta a uno stack overflow. Soluzione: un solo buffer condiviso, passato per puntatore, con *backtracking*: salvi `len`, appendi, ricorri e poi ripristini `buf[len] = '\0'`.
- Nel tuo codice: `math-pratt/math-pratt.c:284-337` (`printAST_recursive` con `sharedBuf`, `memcpy` di `appendLen + 1` per copiare anche il `\0`) e il buffer unico in `printTree` (`:359`).
- Decisioni: `memcpy` al posto di `snprintf`, quindi controllo dei limiti fatto a mano (`:317-325`). Attenzione: `"│"` in UTF-8 è 3 byte, quindi usa sempre `strlen`.
- Errori/lezione: il limite 1024 è scritto a mano nella funzione ricorsiva. Meglio passare anche la `size` del buffer.
- Turni: 96, 97, 98, 99

### Buffer grandi sullo stack → stack overflow
- Cos'è: lo stack del main è 8 MB (`ulimit -s` = 8192). Una struct con `char data[15000000]` dichiarata come variabile locale fa crashare il programma prima della prima istruzione. Nei thread secondari il limite di default è **512 KB**: un array locale di 1 MB in un pthread fa crashare il programma (verificato).
- Nel tuo codice: `networking/client_http.c:72-76` `AudioBuffer { char data[MAXAUDIOBUFFER]; }` dentro `Ctx`. Ora `Ctx *ctx = calloc(1, sizeof(Ctx))` (`networking/client_http.c:984`). Nello snake, `GameContext` (5.9 KB) è sullo stack del main (`snake/snake.c:693`) e `a_star` mette `minHeap` (7.2 KB) + `g_score` (1.8 KB) sullo stack (`snake/snake.c:443-446`): ok su Mac, troppo per uno stack da 2 KB sul Pico.
- Decisioni: turno 209, i 7 MB sullo stack "funzionano" ma sono una bomba a orologeria. Turno 210, nell'heap con `calloc`: l'accesso costa uguale (stessa RAM e cache), l'allocazione è più lenta ma avviene una volta sola.
- Errori/lezione: il segfault del turno 208 l'ha causato il consiglio del turno 207 (portare il buffer a 15 MB senza notare che `Ctx` era sullo stack).
- Turni: 207, 208, 209, 210

### Dimensionare i buffer (piccolo e fisso vs grande e dinamico)
- Cos'è:
  - Buffer di **ricezione** piccolo e fisso (4-16 KB), riusato a ogni `recv`/`SSL_read`.
  - Buffer di **destinazione** nell'heap: con `realloc` se devi accumulare tutto, oppure un ring buffer a dimensione fissa se fai streaming.
  - Buffer per righe e URL: 512 byte, non 60.
- Nel tuo codice: `networking/client_http.c:36-46` (`MAXDATASIZE 8192`, `MAXLINE 512`, `MAXAUDIOBUFFER 15000000`), `:86` `char url[512]`. Le versioni precedenti `client_http_3.c:29` (`MAXDATASIZE 1`) e `client_http_2.c:28` (1000) mostrano l'evoluzione.
- Decisioni: turno 211, `MAXLINE` e `url` portati da 60 a 512 (gli URL con token di query si sarebbero troncati).
- Turni: 168, 170, 211

### Cache locality e prefetch
- Cos'è:
  - *Località*: proprietà del **tuo** accesso ai dati. Spaziale: accedi a indirizzi vicini. Temporale: riusi gli stessi dati a breve distanza.
  - *Prefetch*: l'**hardware** (o il compilatore) carica in anticipo le linee di cache quando riconosce un pattern sequenziale.
  - Array contiguo = località alta. Linked list = nodi sparsi, quindi cache miss.
- Nel tuo codice: ring buffer audio contiguo (`networking/client_http.c:72-76`). Snake: corpo passato da linked list a ring buffer `int body[GRID_CELLS]` (`snake/snake.c:43-49`) e coda BFS su array (`snake/snake.c:87-91`).
- Turni: 210, 212 (+ chat snake)

### Generics in C: macro vs `void *`
- Cos'è: per non duplicare `listPush`/`listPushObj` puoi usare una macro `do { ... } while(0)` con `sizeof(*(l)->list.ele)` (duck typing a compile time) oppure una lista generica `void **` (una sola funzione, ma perdi il controllo dei tipi).
- Nel tuo codice: duplicazione ancora presente (`chord-pratt/chord-pratt.c:499-503` `listPushObj`).
- Errori/lezione: con la macro, un tipo sbagliato produce un **warning** (`-Wincompatible-pointer-types`), non un errore (verificato con clang 17): compila lo stesso.
- Turni: 48

### "Chi alloca, dealloca" (anche con wasm)
- Cos'è: la memoria va liberata dallo stesso modulo e con la stessa logica che l'ha allocata. Da JS non si chiama `_free` sulla radice di un AST, ma si esporta `release()`, che libera a cascata.
- Nel tuo codice: è il motivo di `freeContext`/`release(ctx.ast)` in ogni main dei parser.
- Turni: 29, 31

### mmap: un file trattato come memoria
- Cos'è: `mmap` proietta un file nello spazio di indirizzamento. Le pagine si caricano su richiesta (page fault). Con `MAP_SHARED` + `PROT_WRITE` scrivere nell'array significa scrivere sul file.
- Nel tuo codice: `mmap_test.c:28-66` (ftruncate → mmap → memcpy → msync → munmap), `stdio3.c:8` (sola lettura, senza `munmap`/`close`).
- Decisioni: alternativa a "fread di tutto il file in un buffer" per file enormi (turno 137), insieme allo *sliding window* con `memmove` del residuo.
- Turni: 137

### run_id: azzerare senza memset
- Cos'è: invece di `memset(visited, 0, ...)` a ogni ricerca, incrementi un contatore `run_id` e consideri "visitato" solo `visited[i] == run_id`. Il costo del reset passa da O(N) a O(1). I dati vecchi restano in memoria ma non contano.
- Nel tuo codice: `snake/snake.c:268-279` (BFS) e `snake/snake.c:440-457` (A*). `g_score` (`:446`) non è inizializzato, ma viene letto solo se `visited == run_id` (`:487-489`).
- Errori/lezione: BFS e A* hanno **due** `static int run_id` diversi ma condividono lo stesso `ctx->visited`. Se un giorno li usi entrambi nella stessa partita, un numero già usato dall'altro algoritmo dà falsi "visitato". Meglio un solo contatore nel contesto.
- Turni: chat snake (nessun numero di turno)

## Possibili errori o imprecisioni di Gemini
- **Turno 37**: "`.rodata` è bloccato a livello hardware, se fai `free()` il sistema va in crash". Impreciso. La protezione hardware scatta quando **scrivi** (SIGBUS). `free` su un puntatore non ottenuto da malloc è undefined behavior, e su macOS lo intercetta l'allocatore (abort, exit 134), come per un array sullo stack. Anche "l'unica soluzione corretta è xmalloc(1)" è esagerato: vanno bene anche `strdup("")` o un flag di ownership.
- **Turni 74, 76, 82, 115, 210**: ripete che `malloc` "chiede memoria al sistema operativo" o fa una syscall. In realtà `malloc` è codice della libc in user space e chiede pagine al kernel solo ogni tanto (blocchi grandi o arena da allargare). Nel turno 82 "zero chiamate di sistema" con lo stack vale anche per una malloc di 40 byte. Nel turno 210, per `calloc` da 15 MB "cerca una zona di RAM libera e continua" è sbagliato: la memoria è contigua solo virtualmente e le pagine fisiche arrivano al primo accesso.
- **Turno 79**: consiglia di spostare `rulesTable` in fondo al file come "standard architetturale" contro "dipendenze circolari". Errato: i prototipi bastavano, e lo spostamento ha causato gli errori del turno 80 (Gemini lo ammette).
- **Turno 76**: l'`arenaAlloc` d'esempio non gestisce l'allineamento (con tipi misti i puntatori risultano disallineati: undefined behavior). E "nessun memory leak possibile" è esagerato.
- **Turno 207**: porta `MAXAUDIOBUFFER` a 15 MB senza accorgersi che `Ctx` era sullo stack. È la causa diretta del segfault del turno 208.
- **Turni 208-209**: "il limite dello stack è 8 MB" vale solo per il main thread. I pthread su macOS partono con 512 KB (verificato), cosa rilevante per il player multithread.
- **Turno 211**: con `MAXDATASIZE = 1` "una syscall per ogni byte". Con `SSL_read` OpenSSL legge interi record TLS nel suo buffer interno: le chiamate per byte sono chiamate di libreria (lente), non syscall.
- **Turno 137**: con `mmap` "zero modifiche al codice". Falso per i tuoi parser. Il file mappato non è garantito terminato da `\0`, e `split()` scrive nel buffer (`chord-pratt/chord-pratt-threads.c:789`), quindi con `PROT_READ` il programma va in crash e con `MAP_SHARED` modificheresti il file: serve `MAP_PRIVATE`. Inoltre "libera automaticamente le vecchie pagine" succede solo sotto pressione di memoria.
- **Turno 116**: "`result2 = evalAST(...)` viene creato nell'heap". Contraddice il turno 111: `evalAST` restituisce un nodo già esistente (`chord-pratt/chord-pratt.c:671-686`). Le conclusioni (double free / leak) restano valide.
- **Turno 99**: "`fprintf` delega la gestione del buffer al sistema operativo". Il buffer di `FILE*` sta nella libc (user space), il kernel riceve solo le `write()`.
- **Turno 48**: "se passi un Token a una lista di CsObj il compilatore crasherà". Con clang 17 è solo un warning: il binario viene prodotto.
- **Turno 83**: `memset` a 0 rende i puntatori NULL su tutte le piattaforme comuni, ma lo standard non lo garantisce. La forma idiomatica è `pCtx ctx = {0};`. Precisazione minore.

## Domande aperte / cose non chiarite
- toyforth: `freeObject` non libera `list.ele` e il main non libera nulla (`toyforth/toyforth.c:115-120,468-480`). Mai sistemato perché il progetto si è fermato al 16/03.
- Arena allocator: mai provato. Ha senso per chord-pratt, visto che `evalAST` condivide i nodi? O conviene un'arena per il parsing e una copia per il risultato?
- Zero-copy nel lexer chord-pratt: deciso di non farlo o rimandato? (turno 110)
- `evalAST` che muta l'AST: va bene o serve una versione che costruisce un nuovo albero?
- Player multithread: quali buffer locali finiscono negli stack da 512 KB dei pthread? (`pthread_attr_setstacksize` mai discusso.)
- Snake sul Pico: `GameContext` e `minHeap` andrebbero `static` (stack da 2 KB). Mai fatto.

## Argomenti a margine
- WebAssembly: memoria lineare, `memory.grow`, `_free` esportata (turni 29-31): in wasm non esiste una `free` nativa, l'allocatore è compilato dentro il modulo.
- Lettura di file enormi a chunk: sliding window + `memmove` del residuo (turno 137).
- Embedded senza OS: `malloc` esiste (newlib), ma si usano allocazione statica e memory pool (chat snake).
- Buffer overflow didattico: `matrix.c`/`matrix_dump.c`, layout delle variabili sullo stack (nessun turno: esercizi del gennaio 2026).

## Domande di autoverifica
1. In `basicMathFunctions` di toyforth, perché fai `release(a)` anche se `a` è una variabile locale che sparirà al `return`?
2. Che differenza c'è tra `char *s = "ciao";` e `char s[] = "ciao";`? Cosa succede se fai `s[0] = 'C'` in ciascun caso?
3. Perché `exec` in toyforth fa `retain(word)` mentre `basicMathFunctions` pusha `createIntObject(result)` senza retain?
4. Qual è la regola in due domande per decidere stack o heap? Applicala a: un Token del lexer, un nodo AST, il contesto del parser, il ring buffer audio.
5. Cosa stampa (o rompe) `Token t = ctx->lexer.curToken; t.type = TOKEN_PLUS;`? Come lo correggi?
6. Perché una tabella allocata come array locale in `createContext` e salvata in `ctx->table` è un bug? Quali sono le due alternative corrette?
7. In `split()` del parser multithread, quanti byte devi copiare con `memcpy` per portarti dietro il `\0`? E cosa succede se il file non contiene un `\n` dopo la metà?
8. Spiega con i refcount cosa succede se fai `listPushObj(result1, result2)` senza `retain` e poi `release(ctx1.ast); release(ctx2.ast);`.
9. `printAST_recursive` con buffer condiviso: perché serve rimettere `sharedBuf[originalLen] = '\0'` dopo le chiamate ricorsive?
10. Il programma con `Ctx ctx;` (15 MB) va in segfault prima di stampare qualsiasi cosa: perché? Perché con 7 MB funziona, e perché in un thread secondario non funzionerebbe nemmeno con 1 MB?
11. Trova il leak in `freeObject` di toyforth. Come lo confermi con `leaks`?
12. Cosa guadagni e cosa perdi passando da refcount + `freeObject` ricorsivo a un arena allocator? Perché l'arena deve preoccuparsi dell'allineamento?
13. Cache locality e prefetch sono la stessa cosa? Perché il ring buffer dello snake è più veloce della linked list anche senza contare le malloc?
14. Nella BFS dello snake, perché `run_id` sostituisce `memset(visited, 0, ...)`? Che bug latente nasce dall'avere due `static int run_id` separati?
15. `mmap` del file da parsare al posto di `fread`: quali due assunzioni del tuo parser chord-pratt si rompono?
