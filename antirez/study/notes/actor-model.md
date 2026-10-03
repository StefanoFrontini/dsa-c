# Actor model, event loop e architetture event driven
Periodo: 2026-06-30 → 2026-07-16 · Turni: 214-217, 220-221, 243-269

## Mappa in breve
Parte dal player HLS multithread (thread rete + thread audio + mutex/condvar, vedi area networking/thread) e dalla domanda
"come scarico 99 chunk in parallelo senza 99 thread?" (214-217). Da lì: thread pool vs event loop, poi XState/Erlang
(220-221) e l'idea di riscrivere il player come **attori** (243-245). Scoperta chiave: un attore non è un thread; con un
solo thread l'actor model *è* un event loop e i mutex spariscono (244-245). Segue una lunga parentesi browser
(246-256: queueMicrotask, attori funzionali con closure, React + pub/sub, renderer actor, web components) e il ritorno
al C (257-266): macchine a stati interne vs messaggi, stati del network coordinator, timer asincroni, `poll()` come
cuore dell'event loop, tastiera in modo non canonico, OpenSSL + poll, epoll/kqueue, bitmask di `revents`.
Chiusura su Chrome (processi, Mojo, web worker, wasm: 267-269). Il codice risultante è
`networking/actor/client_http_actor.c` (non compila, vedi `projects/audio-player-actor.md`) e l'esercizio
`networking/actor/poll_input.c`. Collegamenti: broker MQTT futuro (epoll/attori), progetto web JS+Worker+WASM (249).

## Concetti

### Thread pool vs event loop per I/O parallelo
- Cos'è: per scaricare N risorse in parallelo non serve un thread per risorsa. **Thread pool**: K thread fissi pescano
  da una coda di lavoro protetta da mutex. **Event loop**: un solo thread, socket non bloccanti, il kernel
  (`poll`/`epoll`/`kqueue`) dice quale socket è pronto. È l'equivalente C di `Promise.all` (che non crea thread).
- Nel tuo codice: il thread pool non è stato scritto; la versione multithread è `networking/client_http.c:1012-1013`
  (2 thread + `pthread_mutex_t` a `:127`). L'event loop compare solo in `networking/actor/client_http_actor.c:1393`.
- Decisioni: hai scelto di partire dal thread pool per fare pratica con i thread (216) e poi passare all'event loop
  "per il broker MQTT". Con il pool i chunk arrivano fuori ordine → serve una "matrice di scatole" indicizzata per
  chunk invece del ring buffer lineare (216-217). Tua intuizione (215): l'event loop vale anche lato client, con una
  finestra scorrevole di 10 connessioni.
- Errori/lezioni: un `connect()` non bloccante ritorna `EINPROGRESS`, non è un errore (215) — lezione **non** applicata
  nel codice attuale, vedi bug in `initConnection` (`client_http_actor.c:1244-1256`).
- Turni: 214-217

### Actor model: stato privato, mailbox, comportamento
- Cos'è: un attore = **stato privato** + **mailbox** (coda FIFO di messaggi) + **comportamento**. Riceve un messaggio
  alla volta e può solo: cambiare il proprio stato, inviare messaggi ad altri attori, creare attori.
  Formula: `(stato, evento) → (nuovo stato, messaggi da inviare)`. "Share nothing": nessuno tocca lo stato altrui.
- Nel tuo codice: `client_http_actor.c:108-141` (`Event {type, payload}`, `Mailbox {events[32], head, tail}`,
  `Actor {stati, receive, mailbox, local_ctx}`), `TransitionFn` a `:121`, `sendMessage` a `:434`.
- Decisioni: 3 attori proposti da te (243: ring buffer, play/stop, rete) → nel codice 4 attori
  (Network coordinator, Buffer, Player, Fetcher) + un "attore" fittizio `ACTOR_LIST` usato come array (`:423`).
  Payload come `void *` per passare la *proprietà* di un buffer senza copiarlo (243).
- Errori/lezioni: `local_ctx` è sempre `NULL` e il ring buffer audio sta in `Ctx->parser.audio_buf` (`:191`),
  condiviso tra `fetch()` e `read_packet()`: lo stato **non** è ancora privato dell'attore Buffer.
  Gli stati dei 4 attori sono in una `struct` (non in una `union`) dentro ogni `Actor` (`:128-135`): ogni attore porta
  con sé anche gli stati degli altri.
- Turni: 220, 243, 245

### Attori con o senza thread (1:1, single thread, M:N)
- Cos'è: l'attore è un concetto logico; il thread è il modo di eseguirlo. Tre mappature: un thread per attore
  (mailbox con mutex), **tutti gli attori in un thread** (event loop, zero mutex), **M:N** (K thread + scheduler, come
  BEAM/Akka).
- Nel tuo codice: hai scelto il single thread; infatti in `client_http_actor.c` non ci sono `pthread_*`, ma restano
  i corpi dei vecchi thread `get_data` (`:1001`) e `decode` (`:1045`) come codice morto con attese attive.
- Decisioni: single thread perché elimina i mutex e il context switch (244-245). Nota: SDL3 riproduce l'audio su un
  suo thread interno, quindi il processo non è mai davvero single thread; `SDL_PutAudioStreamData` è thread safe.
- Turni: 243-245

### Event loop di dispatch e mailbox come ring buffer
- Cos'è: il `main` diventa un ciclo: 1) raccogli eventi dal mondo (I/O, timer) e trasformali in messaggi;
  2) svuota le mailbox chiamando `receive(actor, ev)` per ogni messaggio. Inviare = scrivere in `events[head]` e
  avanzare `head` modulo N; leggere = `tail`. `head == tail` significa vuota.
- Nel tuo codice: invio a `client_http_actor.c:434-437`, drain a `:1442-1449`.
- Errori/lezioni:
  - manca il controllo "mailbox piena": al 33° messaggio non letto `head` raggiunge `tail` e la coda sembra **vuota**,
    cioè perdi 32 messaggi in silenzio (anche nel codice di Gemini, 245);
  - il ciclo di drain usa `ACTOR_NUM` = 3 (`:23`) ma gli attori sono 4: la mailbox del Fetcher (`ele[3]`) non viene
    mai svuotata, quindi `EV_FETCHER_DOWNLOAD` (`:322`) non arriverebbe mai;
  - `createActorList` alloca `sizeof(Actor) * 3` invece di `sizeof(Actor *) * 4` (`:426`): funziona per caso perché
    `Actor` è grande.
- Turni: 245

### Macchina a stati come comportamento (switch stato × evento)
- Cos'è: la funzione di transizione è una griglia: `switch (stato)` esterno, `switch (ev.type)` interno. Un evento non
  previsto in quello stato viene ignorato. È lo stesso modello di XState.
- Nel tuo codice: `transitionFnPlayer` `:225`, `transitionFnBuffer` `:268`, `transitionFnNetwork` `:315`,
  `transitionFnFetcher` `:350`. Tutti i rami sono vuoti tranne `IDLE + EV_START_RADIO` (`:319-327`) che invia a
  Fetcher ma non cambia stato.
- Errori/lezioni (bug reali):
  - `createPlayerActor` assegna `receive = transitionFnBuffer` (`:407`): copia-incolla, il Player esegue la FSM del Buffer
    leggendo un `bufferState` mai inizializzato (`xmalloc`);
  - in `transitionFnPlayer` il `case STATE_PLAYER_BUFFERING` non ha `break` (`:253-261`) → cade nel `default` → `exit(1)`;
  - `transitionFnNetwork` gestisce solo `IDLE` e `DOWNLOADING`: gli stati `DOWNLOADING_MASTER/SUB/CHUNK` (`:57-59`)
    finiscono nel `default` → `exit(1)`.
  - `Event ev` dichiarato dentro il case (`:322`) nasconde il parametro `ev` (shadowing, `-Wshadow` lo segnala).
- Turni: 220, 245, 257

### Macro-stati vs micro-stati: FSM interna e pipeline di attori
- Cos'è: la tua domanda (257): `fetch()` cambia stato (INIT_CONNECTION → SENDING_REQUEST → HEADER_…) senza eventi
  esterni; è lecito? Sì: un attore può avere una FSM interna. Il problema non è la FSM, è che `fetch()` è un
  `while` **bloccante** che fa tutto in una chiamata. Alternativa "pura": Network Coordinator + sotto-attori
  (Connector, HeaderParser) che rispondono con `CONN_OK`, `HEADER_OK`…
- Nel tuo codice: `ParserState` `:76-95`, `fetch()` `:607-999` è ancora la versione bloccante della vecchia
  architettura; le variabili di avanzamento (`k_idx`, `chunk_size`, `i`, `audioCounter`, `:609-619`) sono **locali**,
  quindi il parser non può interrompersi e riprendere.
- Decisioni: hai creato un attore `Fetcher` (`:74`, `:350`) come primo sotto-attore, invece di Connector/HeaderParser.
- Lezione: in C la via pratica (nginx, Redis) è una FSM **riprendibile** per connessione: tutto lo stato nella struct,
  a ogni `POLLIN` consumi i byte disponibili e ritorni al loop. Gli attori-per-fase sono eleganti ma costosi.
- Turni: 257

### Stati del network coordinator e RETRY non bloccante
- Cos'è: macro-stati HLS proposti da te: IDLE, MASTER_PLAYLIST_DOWNLOADING, SUB_PLAYLIST_DOWNLOADING,
  CHUNK_AUDIO_DOWNLOADING, RETRY. Il retry non usa `sleep(5)` (bloccherebbe tutto) ma un timer che, scaduto,
  invia `TIME_ELAPSED` al coordinatore. In RETRY uno `STOP` utente porta subito a IDLE.
- Nel tuo codice: `NetworkState` `:54-60` (manca RETRY, c'è un generico DOWNLOADING); `EV_TIME_ELAPSED` `:103`.
- Errori/lezioni: un `TIME_ELAPSED` "vecchio" può arrivare dopo uno STOP e un nuovo errore: serve un id/generazione
  del timer o la cancellazione esplicita.
- Turni: 258

### Timer asincroni in C
- Cos'è: il timer è solo un timestamp di scadenza. Prima di `poll()` calcoli `timeout = scadenza - adesso` (o -1 se
  non ci sono timer); dopo `poll()` controlli i timer scaduti e mandi `EV_TIME_ELAPSED` all'attore bersaglio.
- Nel tuo codice: `AsyncTimer` `:143-147`, `startTimerRetry` `:1331` (mai chiamata), controllo scadenza
  `:1432-1436`, `getCurrentTimeUs` `:1295`.
- Errori/lezioni: il timeout di `poll()` considera solo il frame dell'animazione, non il timer; `gettimeofday` è
  l'orologio di sistema (salta se cambi ora/NTP): per i timer usa `clock_gettime(CLOCK_MONOTONIC)`.
  Alternativa macOS: `kqueue` con `EVFILT_TIMER` (l'ident è un id del timer, non un socket).
- Turni: 259

### poll() e l'event loop
- Cos'è: `poll(fds, n, timeout_ms)` addormenta il thread finché un fd è pronto **o** scade il timeout. Con -1 attende
  per sempre, con 0 controlla e ritorna. L'event loop è: `timeout = min(prossimo timer, prossimo frame)` →
  `poll` → gestisci fd pronti → timer → mailbox. È questo che rende "non bloccante" un loop che di fatto si blocca:
  si blocca solo quando non c'è niente da fare.
- Nel tuo codice: `client_http_actor.c:1385-1400`; esercizio `networking/actor/poll_input.c:45-74`.
- Decisioni: per una GUI SDL Gemini ha proposto `poll(…, 0)` + `SDL_Delay(1)` (260); tu hai scelto la TUI da terminale
  (261) con `poll` bloccante e timeout dinamico calcolato sul frame rate (262): scelta corretta.
- Errori/lezioni (bug reali, verificati):
  - `poll(pfds, 2, timeToNextFrame)` riceve **microsecondi** (`FRAME_TIME` = 33333 µs, `:44`) ma il timeout di poll è
    in **millisecondi**: attende fino a 33 s invece di 33 ms (`:1400`). Verificato: senza input 0 frame in 6 s;
  - `pfds[1].fd = ctx->conn.sockfd` (`:1387`) vale 0 (`calloc`) → monitori **due volte stdin**. Su macOS l'evento è
    stato riportato solo sulla seconda voce, quindi `pfds[0].revents` resta 0, il tasto non viene mai letto, `poll`
    ritorna subito a ogni giro (spin) e `q` non esce. Usa `fd = -1` (poll lo ignora) finché il socket non esiste;
  - su macOS `poll()` non supporta i device: su `/dev/tty` ritorna `POLLNVAL` (man page, sezione BUGS); su stdin
    (pty del terminale) funziona.
- Turni: 260, 262

### Tastiera in modo non canonico (termios)
- Cos'è: il terminale di default è **canonico** (consegna la riga solo con Invio). Togliendo `ICANON` (e `ECHO`) con
  `tcsetattr` ogni tasto arriva subito su stdin, quindi `poll` si sveglia al singolo tasto.
- Nel tuo codice: `client_http_actor.c:1313-1330`, `atexit(ripristina_terminale)` `:1375`, lettura tasti `:1402-1415`.
- Errori/lezioni:
  - `read(STDERR_FILENO, &key, 1)` (`:1404`): legge da **stderr** invece che da stdin. Funziona per caso nel terminale
    (fd 2 è lo stesso tty aperto in lettura/scrittura), si rompe se stderr è rediretto; il ritorno non è controllato e
    `key` resta non inizializzato;
  - `atexit` non viene eseguito su Ctrl-C (SIGINT) o crash: serve anche un handler di segnale, altrimenti il terminale
    resta senza eco.
- Turni: 214, 261

### OpenSSL dentro un event loop (SSL_read, SSL_pending)
- Cos'è: sul socket TLS usi `SSL_read`, non `read`. Trappola: OpenSSL legge un record TLS intero e può tenere byte già
  decifrati nel suo buffer: il socket del kernel è vuoto, `poll` dorme, ma i dati ci sono. Soluzione: socket
  `O_NONBLOCK` e **drain loop** `SSL_read` finché `SSL_ERROR_WANT_READ` (controllando `SSL_pending`);
  `SSL_ERROR_WANT_WRITE` → monitorare `POLLOUT`.
- Nel tuo codice: il drain loop è incollato **come commento** in `readChunk` (`:513-555`), non usato. Il socket è messo
  `O_NONBLOCK` (`:1244`) ma `connect`/`SSL_connect` sono scritti in stile bloccante → verificato: `connect: Operation now
  in progress` su tutti gli indirizzi, "failed to connect". `F_SETFL` sovrascrive i flag: meglio `F_GETFL | O_NONBLOCK`.
- Turni: 264

### poll vs epoll/kqueue/IOCP
- Cos'è: `poll` copia l'array a ogni chiamata e scorre tutti gli fd (O(n)); `epoll` (Linux) e `kqueue` (BSD/macOS)
  tengono l'insieme nel kernel e restituiscono solo i pronti; IOCP (Windows) è a completamento. Con 2-3 fd `poll` è
  la scelta giusta ed è portabile Mac/Linux. Le librerie (libuv) scelgono il backend con `#ifdef`.
- Nel tuo codice: `poll` sia in `client_http_actor.c` sia in `poll_input.c`.
- Turni: 215, 265, 267

### Bitmask di revents
- Cos'è: `revents` è uno `short` in cui ogni bit è un flag (`POLLIN`=0x1, `POLLHUP`=0x10, `POLLNVAL`=0x20).
  `revents & POLLIN` vale 0 (falso) o il valore del bit (vero, qualunque numero ≠ 0). Più flag possono essere
  attivi insieme (POLLIN|POLLHUP: connessione chiusa ma byte ancora da leggere).
- Nel tuo codice: `poll_input.c:57-63`, `client_http_actor.c:1402`, `:1418`.
- Lezione: in `poll_input.c` `read` che ritorna 0 (EOF) non chiude l'fd: su un file regolare il programma gira per
  sempre (verificato: 3 milioni di righe in 3 s). Su macOS chiudere lo scrittore di una FIFO non ha prodotto
  `POLLHUP` nel test (poll rimasto bloccato).
- Turni: 264, 266

### Actor model nel browser: event loop JS e queueMicrotask
- Cos'è: il browser ha già un event loop single thread, quindi gli attori JS non hanno bisogno di lock. `send()`
  accoda e programma il `dispatch` con `queueMicrotask`: chi invia non esegue il ricevente in modo sincrono (niente
  ricorsione A→B→A sullo stack). Microtask = eseguiti subito dopo il codice corrente, **prima** del rendering;
  `setTimeout(0)` = task successivo.
- Decisioni: hai notato che `queueMicrotask` è la coda delle Promise (247): corretto.
- Lezione: un ping-pong infinito di microtask non fa stack overflow ma **blocca la pagina** (il rendering non avviene
  mai finché la coda microtask non si svuota).
- Turni: 246-247

### Attori in stile funzionale (closure al posto di classi/struct)
- Cos'è: `creaAttore(statoIniziale, transizione)` restituisce `send`; stato e mailbox vivono nella closure (in C li
  passi esplicitamente con `Actor *self`). La transizione `(stato, evento) → nuovoStato` si testa senza browser.
- Lezione: nella versione di Gemini se la transizione lancia un'eccezione la mailbox resta non vuota e `send` non
  riprogramma più il drain (`if (mailbox.length === 1)`): l'attore resta bloccato per sempre. La transizione pura
  restituisce solo lo stato: i messaggi in uscita sono side-effect (meglio restituire `{stato, effetti}`).
- Turni: 248-249

### Pub/sub + actor model (React, useSyncExternalStore)
- Cos'è: ingresso = actor model (`send` nella mailbox), uscita = observer/pub-sub (`subscribe`, notifica dei listener
  quando lo stato cambia). React diventa solo vista: `useSyncExternalStore(actor.subscribe, actor.getSnapshot)`,
  niente `useState` per la logica. È lo schema di XState/Redux/Zustand.
- Turni: 252-253

### Renderer actor in vanilla JS (centralizzato, granulare, con o senza web components)
- Cos'è: senza React qualcuno deve toccare il DOM. Opzione 1: un `attoreRenderer` unico. Opzione 2: un attore per
  pezzo di UI, ognuno iscritto all'attore core. I web components sono solo il "guscio"; si può fare lo stesso
  legando closure a elementi trovati per id (256).
- Lezione: i *customized built-in* (`extends: 'button'` + `is="..."`) non funzionano in Safari; meglio
  `class extends HTMLElement` o la versione con closure. `dispatchEvent` è **sincrono**: non è una mailbox.
- Turni: 254-256

### requestAnimationFrame vs poll con timeout
- Cos'è: entrambi danno un "battito" per ridisegnare senza bruciare CPU. rAF è legato al refresh del monitor (vsync),
  si ferma nelle tab nascoste e riceve un timestamp; il tuo `poll` usa un timer software fisso (30 FPS) e gira anche
  se il terminale è nascosto. Nel browser I/O, microtask e rendering stanno in code diverse; in C tutto passa da
  un'unica `poll`.
- Nel tuo codice: `drawAnimation` `client_http_actor.c:1303`, gestione frame `:1394-1428`.
- Turni: 262-263

### Erlang/Elixir, XState e framework ad attori
- Cos'è: in Erlang l'actor model è il linguaggio: `spawn`, `!`, `receive`; processi BEAM leggeri (KB), scheduler
  preemptive per riduzioni, supervisori ("let it crash"). XState v5: tutto è un attore (`createActor`, `spawn`).
  Elm/TEA: Model + Msg + update, gemello single thread dell'attore.
- Turni: 220-221, 250

### Chrome: processi, Mojo, web worker e WebAssembly
- Cos'è: Chrome è multiprocesso (browser, network service, GPU, renderer per sito) che comunicano con messaggi IPC
  (Mojo); dentro, i thread si passano task con `PostTask` invece di condividere stato. Il network service usa
  `epoll`/`kqueue`/IOCP. Un **dedicated worker** è un thread nello stesso processo renderer; **wasm non è un thread**:
  gira sul thread che lo chiama (per averlo in parallelo lo metti in un worker; i "wasm threads" sono worker +
  `SharedArrayBuffer`). MojoJS non è disponibile alle pagine web.
- Collegamento: il progetto web (249) = UI nel main thread, rete in un worker, decoder FFmpeg/WASM in un altro worker,
  `postMessage` come mailbox, Transferable per non copiare i buffer.
- Turni: 249, 267-269

## Possibili errori o imprecisioni di Gemini
- 214: "99 thread = quasi 1 GB di RAM": lo stack (512 KB-8 MB) è memoria **virtuale riservata**, le pagine vengono
  usate solo se toccate. Anche "la memoria si corrompe se head e tail si toccano nello stesso ciclo di clock" è
  impreciso: un ring buffer 1 produttore/1 consumatore può essere lock-free con atomiche; il problema è visibilità/
  ordinamento della memoria.
- 215: registra il socket con `EPOLLIN|EPOLLET` mentre il testo dice di attendere `EPOLLOUT` per la connect; ignora che
  anche l'handshake TLS va fatto in modo non bloccante.
- 221: "poche migliaia di thread poi il sistema crasha" e "in Erlang non esistono thread del SO": la BEAM usa thread
  del SO (uno scheduler per core); Linux regge decine di migliaia di thread.
- 245: loop con `SDL_Delay(1)` come anti-100% CPU e `send_message` senza controllo di coda piena (overflow = mailbox
  che sembra vuota).
- 247: presenta `queueMicrotask` come sicuro anche per rimbalzi infiniti; in realtà bloccano rendering e input.
  Il minimo di 4 ms di `setTimeout` scatta solo dopo 5 livelli di annidamento, non sempre.
- 249: "il worker gira su un core separato" (decide il SO). Non dice che il browser decodifica già AAC/HLS
  (WebCodecs/MSE): FFmpeg in WASM è una scelta didattica, non necessaria.
- 250: "Wind.js" non è un framework ad attori (era una libreria per l'asincronia); il rollback transazionale di Tarant
  non è verificato. Lo Shadow DOM isola DOM/CSS, non lo stato JS. `dispatchEvent` è sincrono, non una mailbox.
  "Figma fa girare il motore C++/WASM in un Web Worker": affermazione non verificata (le fonti pubbliche di Figma
  descrivono il motore WASM + WebGL senza parlare di worker), da non prendere come dato.
- 253: "nel pub/sub tutti i callback partono in parallelo": `forEach` li chiama in sequenza, sullo stesso thread.
- 254: "React rigenera l'intero albero del Virtual DOM a ogni cambio": rifà il render solo del sottoalbero del
  componente che cambia.
- 255: usa `className` in HTML (è JSX) e built-in personalizzati (`is=`) non supportati da Safari; "l'Event Loop salta
  l'attore corrotto" è inventato (vedi bug dell'attore bloccato sopra).
- 258: il codice JS mescola stato stringa e stato oggetto (`{fase:'RETRY'}` non entra mai in `case 'RETRY'`) e il
  timer legge `evento.payload` mentre il messaggio ha i campi al primo livello → TypeError.
- 259: "kqueue EVFILT_TIMER passandogli il descrittore del socket" (l'ident è un id del timer); "al giro successivo
  l'event loop annullerà il timer" non corrisponde a nessun codice mostrato; usa `gettimeofday` (non monotono).
- 260: "se poll blocca 5 s l'app macOS crasha" (mostra la rotellina, non crasha) e "poll(0)+SDL_Delay(1) è la soluzione
  industriale / è come fa il browser": browser e GUI si bloccano nell'attesa del SO con timeout calcolato
  (`SDL_WaitEventTimeout`), non con un tick da 1 ms (~1000 risvegli al secondo).
- 261: "atexit ripristina il terminale anche in caso di uscita improvvisa": non su segnali (Ctrl-C) né crash.
- 265: "epoll è O(1) e callback hardware": il costo di `epoll_wait` cresce con gli eventi pronti; la callback è del
  kernel (wait queue), non hardware.
- 267: l'uso di `io_uring` in Chrome non è verificato.
- 268: "Chromium vieta quasi del tutto i mutex" (esiste `base::Lock`, sono solo sconsigliati) e un "Media Decoder
  Process" standard non esiste su tutte le piattaforme.
- 269: "postMessage tra main e worker usa Mojo dietro le quinte": per un dedicated worker nello stesso processo è un
  passaggio di task tra thread; affermazione dubbia.

## Domande aperte / cose non chiarite
- Come far convivere FFmpeg (che *tira* i byte con `read_packet` e si aspetta di bloccarsi) con un event loop che non
  deve mai bloccare? (decodificare solo quando il buffer contiene un chunk `.ts` completo?)
- Chi possiede il ring buffer audio: l'attore Buffer (`local_ctx`) o il `Ctx` condiviso? Chi libera il `payload`?
- Che politica per mailbox piena: scartare, `exit`, coda dinamica?
- Vale la pena in C spezzare l'HTTP in attori Connector/HeaderParser o basta una FSM riprendibile per connessione?
- `Connection: close` a ogni richiesta (handshake TLS ogni volta): passare a keep-alive?
- Il player è davvero single thread visto che SDL3 ha un thread audio interno?

## Argomenti a margine
- Progetto video player: HLS su Ubuntu Oracle con ffmpeg + nginx, YouTube usa DASH con URL firmati (yt-dlp), A/V sync
  con PTS e master clock audio — turno 251.
- Simulatore domotico ad attori (sensore, controllore FSM, attuatore, UI) come esercizio — turno 220.
- Figma come esempio di app con motore C++ in WASM — turni 249-250.

## Domande di autoverifica
1. Quali sono le tre parti di un attore e cosa può fare un attore quando riceve un messaggio?
2. Perché con tutti gli attori in un solo thread non servono mutex sulla mailbox?
3. In una mailbox ring buffer con `head`/`tail`, come distingui "vuota" da "piena"? Cosa succede nel tuo `sendMessage` al 33° messaggio non letto?
4. Cosa restituisce `revents & POLLHUP` se `revents` vale `POLLIN|POLLHUP`? E perché è "vero"?
5. Che differenza c'è tra `poll(fds, n, -1)`, `poll(fds, n, 0)` e `poll(fds, n, 33)`? In che unità è il timeout?
6. Perché in modo canonico `poll` su stdin non si sveglia quando premi `a`? Quali flag di `termios` togli?
7. Perché `queueMicrotask` evita lo stack overflow nel ping-pong tra due attori, ma non evita il blocco della pagina?
8. Trova nel tuo `client_http_actor.c` il motivo per cui il messaggio `EV_FETCHER_DOWNLOAD` non verrebbe mai elaborato.
9. Come calcoli il timeout di `poll` quando hai sia un timer di retry sia un'animazione a 30 FPS? Perché usare `CLOCK_MONOTONIC`?
10. Spiega la trappola `SSL_read` + `poll`: dove stanno i byte quando `poll` dorme? Che ruolo hanno `SSL_ERROR_WANT_READ` e `SSL_pending`?
11. Cosa ritorna `connect()` su un socket `O_NONBLOCK` e come capisci che la connessione è riuscita?
12. Perché `fetch()` così com'è non può stare dentro l'event loop? Quali variabili devono passare da locali a campi di struct?
13. Quando conviene `epoll`/`kqueue` rispetto a `poll`? Perché per il player basta `poll`?
14. Web worker, wasm, processo renderer: chi è un thread, chi è un processo, chi nessuno dei due?
15. Disegna `(stato, evento) → (nuovo stato, messaggi)` per il Network coordinator con RETRY, incluso lo STOP durante l'attesa e il `TIME_ELAPSED` vecchio.
