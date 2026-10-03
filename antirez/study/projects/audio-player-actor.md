# Audio player HLS ad attori (event loop single thread)  — stato: broken
Path: networking/actor/client_http_actor.c (1454 righe) · Ultima modifica: 2026-07-17 (commit `client_http_48`, 63b0987)

È un lavoro in corso (in-progress), fermo dal 17/07. Lo segno **broken** perché oggi **non compila** (9 errori).
Storia: `client_http_39`…`_42` (7-11/07) erano solo lo scheletro degli attori, 233-301 righe, e il `_42` compilava;
con `_43` (12/07) è stato incollato il codice di rete/decoder della versione multithread e da lì non compila più.

## Cosa fa
Obiettivo: riscrivere il player Radio24 (HLS su HTTPS → ring buffer → FFmpeg → SDL3) come **actor model in un solo
thread**: un event loop con `poll()` su stdin + socket, timer asincrono, animazione da terminale a 30 FPS, attori con
mailbox e funzione di transizione. Oggi contiene:
- lo scheletro degli attori: tipi, mailbox, 4 funzioni di transizione con rami vuoti (`:46-141`, `:225-437`);
- il codice HTTP/TLS/HLS della versione multithread, ancora **bloccante** (`fetch()` `:607-999`, `initConnection`
  `:1212`);
- i corpi dei vecchi thread `get_data` (`:1001`) e `decode` (`:1045`), non più chiamati (codice morto);
- un `main` con event loop: tasti `a`/`d`/`q`, spinner, controllo di un timer di retry, drain delle mailbox (`:1338-1454`).

## Come si compila / si esegue
```
B=<scratchpad>/build/actor
cc -W -Wall -g networking/actor/client_http_actor.c \
   $(pkg-config --cflags --libs libavformat libavcodec libavutil libswresample sdl3 openssl) -o $B/client_http_actor
# → 9 errori, 6 warning: non produce l'eseguibile
```
Per vederlo girare ho creato una copia con patch minime (fuori dal repo, `$B/patched.c`): spostata l'enum
`ResourceType` prima di `Parser`, aggiunti i prototipi, la macro `errExit`, `enqueueAudioBuffer`/`dequeueAudioBuffer`/
`setAudioRequest` copiate da `networking/client_http.c:132-202` e uno stub vuoto di `handleNonBlockingIOaudio`.
```
perl -e 'alarm 20; exec @ARGV' -- $B/patched < /dev/null              # stdin non tty
(sleep 2; printf q; sleep 5) | script -q /dev/null $B/patched          # tty finto, tasti simulati
```

## Esito verifica
Errori di compilazione (clang, macOS):
```
:190  unknown type name 'ResourceType'            (enum definita a :202, dopo l'uso)
:323  call to undeclared function 'sendMessage'   (+ :434 conflicting types)
:602  undeclared 'dequeueAudioBuffer'   :660 'setAudioRequest'   :821 'enqueueAudioBuffer'
:625  undeclared 'initConnection'       :1380 'errExit'          :1439 'handleNonBlockingIOaudio'
warning: variabili inutilizzate :1368-1371, :1419-1420; con la patch anche :1219 decode() void* senza return
```
Esecuzione della copia con patch:
- stdin = /dev/null: `poll` ritorna subito (EOF) a ogni giro, lo spinner gira a ~30 FPS per 20 s, nient'altro
  (nessuna connessione di rete viene mai aperta);
- con tty, nessun tasto: **0 frame in 6 s** → il timeout di `poll` è in µs passati come ms (attesa fino a 33 s);
- con tty, premo `q`: il programma **non esce** e va in spin (120 frame in 4 s): stdin compare due volte in `pfds`
  (`pfds[1].fd` = sockfd = 0) e macOS segnala l'evento solo su `pfds[1]`, che il codice non legge;
- correggendo solo `pfds[1].fd = -1`: `q` esce (exit 0), `a` arriva al Player senza crash (nessun effetto);
- `initConnection` da sola (test a parte): `client: connect: Operation now in progress` ×4, `failed to connect`
  → il socket `O_NONBLOCK` rompe la connect scritta in modo bloccante;
- togliendo `O_NONBLOCK`, `fetch()` funziona contro il server reale: master playlist → sub playlist → 4 chunk
  `seg64000-0794457x.ts` nel ring buffer, `[SUCCESS] All audio chunks copied to Ring Buffer!`. La parte di rete
  ereditata è sana, è solo ancora sincrona.
- leaks: non misurato (il programma non arriva a una fine pulita: nessuna `free` e nessuna uscita dalle FSM).

## Diagramma attori → messaggi
Stato attuale (── implementato, ·· dichiarato ma ramo vuoto o mai inviato):
```
              tastiera (stdin, poll)                    timer retry (mai avviato)
             'a'│        'd'│         'q'→ esce             │ EV_TIME_ELAPSED
                ▼           ▼                                ▼
         ┌──────────┐  ┌────────────────────┐ EV_FETCHER_DOWNLOAD ┌──────────┐
         │  Player  │  │ Network coordinator│────────────────────►│ Fetcher  │ mailbox mai svuotata
         │ STOPPED  │  │ IDLE               │   (solo se arriva    │ IDLE     │ (ACTOR_NUM=3)
         │ (usa FSM │  │ EV_START_RADIO ··  │    EV_START_RADIO,   └──────────┘
         │  Buffer!)│  │ nessuno lo invia   │    che nessuno invia)     ·· fetch() bloccante
         └──────────┘  └────────────────────┘                             non collegata
        EV_TOGGLE_PLAY_PAUSE    EV_BACK_TO_LIVE_STREAMING (nessun case)
                               ┌──────────┐
                               │  Buffer  │ EV_CHUNK_READY / EV_CHUNK_REQUEST: rami vuoti
                               │  EMPTY   │ il ring buffer vero sta in Ctx, non nell'attore
                               └──────────┘
```
Architettura discussa con Gemini (243-258) a cui tendere:
```
 stdin ──EV_TOGGLE──────────────────────────────► Player ──(SDL_PutAudioStreamData)──► SDL3
 stdin ──EV_BACK_TO_LIVE─► Network coordinator          ▲  │EV_CHUNK_REQUEST (coda SDL bassa)
 main  ──EV_START_RADIO──►  IDLE→MASTER→SUB→CHUNK       │  ▼
                            ⇅ RETRY ◄─EV_TIME_ELAPSED─ timer      Buffer (possiede ring buffer)
                  EV_FETCHER_DOWNLOAD │ ▲ EV_DOWNLOAD_OK/ERR      ▲ EV_CHUNK_READY(ptr,len)
                                      ▼ │                         │
                                    Fetcher ◄── POLLIN socket ────┘ (FSM HTTP riprendibile)
```

## Cosa funziona
- Struttura dati degli attori: `Event`, `Mailbox` ring buffer, `Actor` con `receive` puntatore a funzione (`:108-141`).
- Event loop: `poll` su stdin con timeout calcolato sul frame (`:1393-1400`), spinner a frame fisso (`:1424-1428`),
  modo non canonico + `atexit` (`:1313-1330`, `:1374-1375`), mappa tasti → messaggi (`:1405-1415`), drain mailbox
  (`:1442-1449`), timer con scadenza assoluta controllato dopo poll (`:1432-1436`).
- Il codice HTTP/HLS/TLS di `fetch()` funziona (in versione bloccante).

## Cosa manca / bug noti
1. Non compila: enum `ResourceType` dopo l'uso (`:190`/`:202`); mancano prototipi (`sendMessage`, `initConnection`)
   e le funzioni `enqueueAudioBuffer`, `dequeueAudioBuffer`, `setAudioRequest`, `handleNonBlockingIOaudio`, `errExit`.
2. `poll(pfds, 2, timeToNextFrame)` in µs invece di ms (`:1400`, `FRAME_TIME` `:44`; il commento "~33.333 us" è
   sbagliato: sono 33 333 µs = 33 ms).
3. `pfds[1].fd = ctx->conn.sockfd` (`:1387`) = 0 → stdin monitorato due volte; usare -1 e aggiornarlo dopo la connect.
4. `read(STDERR_FILENO, &key, 1)` (`:1404`) invece di `STDIN_FILENO`; ritorno non controllato.
5. `createPlayerActor` usa `transitionFnBuffer` (`:407`).
6. `case STATE_PLAYER_BUFFERING` senza `break` → `default` → `exit(1)` (`:253-265`).
7. `ACTOR_NUM 3` (`:23`) ma 4 attori: Fetcher mai servito (`:1442`); `createActorList` alloca `sizeof(Actor)*3`
   invece di `sizeof(Actor*)*4` (`:426`).
8. `transitionFnNetwork`: stati `DOWNLOADING_MASTER/SUB/CHUNK` → `exit(1)`; nessuno stato RETRY; `EV_START_RADIO`
   non cambia stato e nessuno lo invia; `EV_BACK_TO_LIVE_STREAMING`, `EV_TIME_ELAPSED` senza case (`:315-348`).
9. Tutti i rami di Player, Buffer, Fetcher sono vuoti (`:225-379`).
10. `sendMessage` senza controllo di mailbox piena (`:434-437`).
11. `O_NONBLOCK` prima di una `connect`/`SSL_connect` scritte in stile bloccante → connessione sempre fallita
    (`:1244-1256`, `:1268`); `F_SETFL` senza `F_GETFL`.
12. `fetch()` (`:607`) è un `while` bloccante con stato in variabili locali; `decode()` (`:1142`) è un `while(1)`;
    `read_packet` (`:588-599`) e `get_data` (`:1015-1023`) fanno attese attive: incompatibili con l'event loop.
13. Socket `POLLIN` gestito con un blocco vuoto (`:1418-1422`); il drain loop `SSL_read`/`SSL_pending` è solo un
    commento (`:513-555`).
14. Il timeout di `poll` ignora il timer; `startTimerRetry` mai chiamata (`:1331`); `gettimeofday` non monotono (`:1297`).
15. Nessun handler SIGINT (terminale senza eco dopo Ctrl-C); nessuna `free`/chiusura in uscita (`:1453`).
16. Thread e mutex: **non** convivono più (zero `pthread_*`), ma i corpi dei thread restano come codice morto.

Differenze rispetto all'architettura discussa con Gemini (243-266):
- Gemini: 3 attori (Network, Buffer, Player); tu (257-258): coordinator + sotto-attori e 5 macro-stati con RETRY.
  Codice: 4 attori con Fetcher come unico sotto-attore; stati diversi (DOWNLOADING generico, niente RETRY).
- Gemini (245): il Buffer è l'unico proprietario di head/tail. Codice: buffer in `Ctx`, scritto da `fetch()` e letto da
  `read_packet()`; `local_ctx` sempre NULL.
- Gemini (259): timeout di poll = tempo al prossimo timer. Codice: solo tempo al prossimo frame.
- Gemini (260) per GUI SDL: `poll(0)` + `SDL_Delay(1)`. Codice: TUI con poll bloccante a timeout dinamico (261-262),
  `SDL_Delay(1)` commentato (`:1450`): scelta migliore di quella proposta.
- Gemini (261): su `POLLIN` del socket invia `EV_DATI_RICEVUTI` al Network. Codice: blocco vuoto.
- Gemini (264): drain loop `SSL_read` non bloccante. Codice: incollato come commento, non usato.

## Concetti applicati
Sezioni di `notes/actor-model.md`: "Actor model: stato privato, mailbox, comportamento", "Attori con o senza thread",
"Event loop di dispatch e mailbox come ring buffer", "Macchina a stati come comportamento", "Macro-stati vs micro-stati",
"Stati del network coordinator e RETRY non bloccante", "Timer asincroni in C", "poll() e l'event loop",
"Tastiera in modo non canonico", "OpenSSL dentro un event loop".

## Prossimo passo consigliato
Passi in ordine per completarlo:
1. **Farlo compilare** senza cambiare logica: spostare l'enum, aggiungere prototipi, copiare le 3 funzioni del ring
   buffer da `client_http.c`, definire `errExit`, cancellare `get_data`/`decode` morti (o metterli sotto `#if 0`).
2. **Sistemare il loop**: timeout in ms (`timeToNextFrame / 1000`), `pfds[1].fd = -1`, `read(STDIN_FILENO)`,
   `ACTOR_NUM 4`, `sizeof(Actor *)`, `receive` del Player, `break` mancante, controllo mailbox piena. Test: lo spinner
   gira a 30 FPS, `q` esce, `a` stampa la transizione del Player.
3. **Collegare il Network coordinator**: inviare `EV_START_RADIO` all'avvio, aggiungere stato RETRY, timer
   nel timeout di poll (con `CLOCK_MONOTONIC`).
4. **Rete non bloccante**: `connect` con `EINPROGRESS` + `POLLOUT`, `SSL_connect` con WANT_READ/WANT_WRITE, poi
   trasformare `fetch()` in una FSM riprendibile (locali → campi di `Parser`) chiamata a ogni `POLLIN` con il drain
   loop di `SSL_read`; a fine risorsa il Fetcher invia `EV_DOWNLOAD_OK`/`ERR` al coordinatore.
5. **Buffer e Player**: il Buffer possiede il ring buffer (`local_ctx`); il Player, a ogni giro, se
   `SDL_GetAudioStreamQueued` è sotto soglia e il Buffer ha almeno un chunk `.ts` completo, decodifica un numero
   limitato di pacchetti (mai attese dentro `read_packet`); `a` = `SDL_PauseAudioStreamDevice`/`Resume`.
6. Uscita pulita (free, `SSL_free`, handler SIGINT) e `leaks --atExit`.
