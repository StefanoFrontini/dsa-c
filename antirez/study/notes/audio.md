# Audio player: decodifica, riproduzione, multithreading
Periodo: 2026-06-27 → 2026-07-06 · Turni: 194, 196-242

## Mappa in breve
Seconda metà del progetto "Underground Radio": il client HTTPS/HLS (area networking: FSM HTTP, OpenSSL, parsing
`.m3u8`) ora scarica i segmenti `.ts` di Radio24 e li deve **far suonare**. Il percorso: dimensione dei segmenti (194)
→ ring buffer di byte compressi (196) → FFmpeg con I/O custom che legge dal ring buffer (198-199) → decodifica AAC in
PCM float e push verso SDL3 (204-207) → crash su stack/heap (208-211) → teoria (seek, PCM, campionamento: 200-202,
222-226) → versione **multithread** con network thread + decode thread, mutex e due condition variable (227-234) →
**caccia ai memory leak** con `leaks`/`sample` (235-242). Il codice finale è `networking/client_http.c` (binario
`underground_radio`, vedi `projects/audio-player.md`). Da qui partono l'area actor model (214-221, 243+: download
paralleli, event loop, versione ad attori `networking/actor/client_http_actor.c`) e le idee per il broker MQTT (213).

## Concetti

### Segmenti HLS `.ts`, bitrate e Content-Length
- Cos'è: un segmento HLS è un file **MPEG-TS** (pacchetti fissi da 188 byte, sync byte `0x47`, tabelle PAT/PMT,
  intestazioni PES) che contiene l'audio AAC. Il `BANDWIDTH` della playlist è in **bit/s**: 67171 bit/s × 6,82667 s =
  458.554 **bit** = ~57.300 **byte**; i ~4 KB in più del `Content-Length` (61.288) sono overhead del container.
- Nel tuo codice: `networking/client_http.c:45-46` (commento con i conti), `:570-585` (body del segmento scritto byte
  per byte nel ring buffer finché `content_length` arriva a 0).
- Decisioni: il dimensionamento del ring buffer è partito da questo calcolo (99 segmenti ≈ 5,6 MB, `hls_player.md`).
- Errori/lezione: confondere bit e byte (194). Verifica oggi: la sub-playlist ha **100** segmenti (non 99), ognuno ~61 KB.
- Turni: 194

### Ring buffer di byte compressi (head/tail)
- Cos'è: array circolare di dimensione fissa. `head` = dove il network thread scriverà il prossimo byte; `tail` =
  prossimo byte che **FFmpeg** leggerà (non la posizione di riproduzione nelle casse, 200). Byte disponibili:
  `head >= tail ? head - tail : MAX - tail + head`. Con soli due indici, `head == tail` significa "vuoto": il caso
  "pieno" non è distinguibile e va evitato (o si tiene un contatore).
- Nel tuo codice: struct `:72-77`, `enqueueAudioBuffer`/`dequeueAudioBuffer` `:132-143` (modulo `MAXAUDIOBUFFER`),
  formula `available` in `read_packet` `:343-344` e in `get_data` `:769-770`.
- Decisioni: prima "scarico 99 chunk e poi suono" (196-211), poi finestra di soli 4 chunk con riempimento a soglia (229).
  `MAXAUDIOBUFFER` è passato da 5.667.553 a 15.000.000 byte (207, 211) ed è rimasto così anche quando i dati vivi sono
  diventati ~250 KB: oggi è sovradimensionato ~50 volte.
- Errori/lezione: (196) `initCtx` chiamata a ogni `RECONNECT` azzerava head/tail → separare stato persistente
  (ring buffer, URL) da stato della singola richiesta (`resetParserForNextRequest` `:324-331`); riuso della stessa
  variabile `i` per tre scopi. (207) 99×61 KB = 6,07 MB > 5,67 MB: `head` ha fatto un giro e ha "mangiato" i dati, FFmpeg
  ha visto solo ~400 KB → pochi secondi di audio. `enqueue` non controlla mai il "pieno" (`:132-136`).
- Turni: 196, 200, 207, 211, 229

### Stack vs heap per buffer grandi
- Cos'è: lo stack del main thread ha un limite (8 MB su macOS/Linux, `ulimit -s`; i thread secondari su macOS 512 KB).
  Una variabile locale da 15 MB (`Ctx ctx;`) supera il limite → segfault appena viene toccata. L'heap (`calloc`) non ha
  questo limite; l'accesso, una volta allocata, ha la stessa velocità (cache e prefetch non distinguono stack e heap).
- Nel tuo codice: `main` `:984` (`Ctx *ctx = calloc(1, sizeof(Ctx))`), messaggio `:987`.
- Decisioni: hai provato con 7 MB sullo stack (funzionava "al pelo", 209), poi heap. `calloc` sostituisce anche il
  `memset` a zero del turno 196.
- Errori/lezione: il segfault del 208 è stato causato dal consiglio di 207 (15 MB) mentre `Ctx` era ancora sullo stack.
  Nota: un `calloc` grande su macOS ottiene pagine "pigre"; la memoria fisica cresce solo quando le pagine vengono
  toccate (vedi caccia ai leak).
- Turni: 208, 209, 210

### I/O custom di FFmpeg: `avio_alloc_context` + `read_packet`
- Cos'è: FFmpeg normalmente legge file/URL; con un `AVIOContext` custom gli dai un buffer di lavoro (4096 byte) e una
  **callback** `read_packet(opaque, buf, buf_size)` che FFmpeg chiama quando ha fame di byte. `opaque` è un `void*` che
  FFmpeg ti ripassa intatto: qui è il `Ctx*`. La callback deve restituire i byte copiati, oppure `AVERROR_EOF` per
  "flusso finito": non esiste un codice "aspetta", quindi in un flusso live la callback deve **bloccarsi** finché
  arrivano dati.
- Nel tuo codice: `read_packet` `:333-364` (attesa su condition variable `:350`, copia byte per byte `:353-355`);
  creazione `:824-830`; liberazione corretta del buffer `:975-978` (`av_freep(&avio_ctx->buffer)`).
- Decisioni: hai mappato l'esempio ufficiale `avio_read_callback.c` (`bd.ptr = buffer`) sul ring buffer passando `ctx`
  come opaque (199).
- Errori/lezione: (207) `av_free(avio_ctx_buffer)` a fine programma → `trace trap` (double free): FFmpeg **rialloca**
  il buffer durante il probing, quindi il tuo puntatore originale diventa invalido. Verificato oggi: dopo
  `avformat_find_stream_info` il buffer AVIO è riallocato a 5.032.767 byte (≈ probesize di default 5 MB).
  (229) `return AVERROR_EOF` con ring buffer momentaneamente vuoto avrebbe ucciso lo stream live → sostituito da attesa.
- Turni: 198, 199, 200, 207, 229, 232-233

### Pipeline FFmpeg: container → pacchetti → decoder → frame
- Cos'è: `avformat_open_input` + `avformat_find_stream_info` riconoscono il container (mpegts) e le tracce;
  `av_read_frame` (nome storico) restituisce **AVPacket** compressi; `avcodec_send_packet` li dà al decoder;
  `avcodec_receive_frame` (in loop finché `EAGAIN`) restituisce **AVFrame** PCM. Ogni pacchetto letto va liberato con
  `av_packet_unref`, anche se non è audio.
- Nel tuo codice: `:844-887` (open, find_stream_info, `av_find_best_stream` che sceglie l'AAC, `avcodec_parameters_to_context`,
  `avcodec_open2`); loop `:915-963`; filtro sulla traccia audio `:917`; unref `:962`.
- Decisioni: `av_find_best_stream` invece del codec fisso `AV_CODEC_ID_MP2` dell'esempio `decode_audio.c` (204).
  Lo stream di Radio24 ha 2 tracce: `aac (HE-AACv2) 44100 Hz stereo fltp` e `timed_id3` (metadati).
- Errori/lezione: (204) `streams[-1]`, `ret` ridefinito, packet/frame non allocati. (222) avevi invertito
  frame/packet: prima il pacchetto compresso, poi il frame decompresso. (237-238) senza filtro `stream_index` i pacchetti
  ID3 andavano al decoder AAC; con il filtro, l'`av_packet_unref` finito dentro l'`if` lasciava i pacchetti ID3 non
  liberati.
- Turni: 198, 204, 222, 237, 238

### PCM: formati planar e packed
- Cos'è: **planar** (`AV_SAMPLE_FMT_FLTP`) = un array per canale (`data[0]` = L, `data[1]` = R); **packed/interleaved**
  (`AV_SAMPLE_FMT_FLT`) = un solo array `L R L R…` in `data[0]`. `AVFrame.data` è un **array di puntatori**
  (`uint8_t *data[8]`), per questo "vedi un solo array" ma sono due (223). SDL vuole interleaved.
- Nel tuo codice: interleaving a mano `:943-953` dentro `pcm_buffer[8192]` (`:104`); ramo "già packed" `:954-957`.
- Decisioni: niente `libswresample` (`swr_convert`): conversione manuale FLTP→FLT. Un frame HE-AACv2 = 2048 campioni ×
  2 canali = 4096 float: il vecchio `pcm_buffer[4096]` bastava esattamente, 8192 è solo margine (207).
- Errori/lezione: il ramo `else` assume che "non planar" voglia dire float packed: con S16 o S16P suonerebbe rumore.
  Il codice non controlla che `samples*channels <= 8192`.
- Turni: 204, 222, 223

### Campionamento, quantizzazione, float in [-1, 1]
- Cos'è: microfono (trasduttore) → tensione analogica → **ADC**: campionamento a 44.100 Hz (Nyquist: più del doppio
  della frequenza massima udibile) e quantizzazione (16 bit: -32768..32767). In riproduzione **DAC** + filtro di
  ricostruzione passa-basso + amplificatore + altoparlante. Il decoder AAC di FFmpeg produce float a 32 bit dove ±1,0 è
  il fondo scala (valore nominale, piccoli superamenti sono possibili).
- Nel tuo codice: `spec.format = SDL_AUDIO_F32` `:895`; `SDLBUFFER 352800 * 3` `:47` = 44100 × 2 canali × 4 byte × 3 s.
- Decisioni: la tua obiezione (226) era giusta: FFmpeg qui **decodifica**, non codifica; i float nascono dalla IMDCT
  del decoder, non da una "elevazione" della registrazione.
- Turni: 224, 225, 226

### Output audio con SDL3 (AudioStream, modello push)
- Cos'è: in SDL3 `SDL_OpenAudioDeviceStream(device, &spec, NULL, NULL)` crea uno stream **in pausa**; tu fai push con
  `SDL_PutAudioStreamData` (non bloccante, accoda), e SDL lo consuma al ritmo della scheda audio. `SDL_GetAudioStreamQueued`
  dice quanti byte sono in coda. Lo stream converte formato/frequenza dalla tua `spec` a quella del dispositivo (quindi
  il resampling, se servisse, lo fa SDL).
- Nel tuo codice: `:889-909` (`SDL_InitSubSystem` restituisce `bool` in SDL3 → `if (!…)`,
  `SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK`, `SDL_ResumeAudioStreamDevice`), push `:952`, `:956`.
- Decisioni: SDL3 push invece della callback (pull) di SDL2; niente secondo ring buffer PCM (proposto in `hls_player.md`):
  la coda di SDL fa da buffer PCM.
- Errori/lezione: (205) programma muto e chiuso subito: stream mai avviato e `SDL_DestroyAudioStream` chiamato prima che
  la coda fosse suonata. (206) `SDL_ResumeAudioStream` non esiste → `SDL_ResumeAudioStreamDevice`.
- Turni: 197, 204, 205, 206, 207

### Sincronizzazione velocità di decodifica / velocità di riproduzione (pacing)
- Cos'è: decodificare è molto più veloce del tempo reale (11 minuti in ~200 ms). Senza freno, tutto il PCM finisce nella
  coda SDL (11 min × 352.800 B/s ≈ 232 MB). A dettare il tempo è l'**orologio della scheda audio**; il programma si
  adegua guardando quanto è piena la coda.
- Nel tuo codice: `:911-914` se la coda SDL supera 3 s (`SDLBUFFER`) il decode thread dorme 100 ms (`SDL_Delay`),
  altrimenti legge un altro pacchetto. Effetto domino: coda SDL piena → niente `av_read_frame` → niente `read_packet`
  → `tail` ferma → il network thread resta sopra soglia e dorme.
- Decisioni: (227) Gemini ha fatto notare che non puoi usare una condition variable POSIX sulla coda interna di SDL
  (non controlli il suo mutex) → polling temporizzato. Il `sample` (234, 242) conferma: il decode thread passa >99% del
  tempo in `nanosleep`.
- Errori/lezione: `SDLBUFFER` è scritto a mano per 44,1 kHz stereo float: andrebbe calcolato da `spec`.
- Turni: 205, 207, 224, 227, 234, 242

### Seek: torna alla diretta, ±1 chunk, ±5 secondi (solo teoria)
- Cos'è: FFmpeg non sa nulla di chunk: per saltare basta cambiare **quali byte** dà `read_packet`. Si salva l'offset di
  inizio di ogni chunk (`chunk_offsets[i] = head` prima del download), si sposta `tail` su quell'offset e si svuotano i
  buffer a valle: `avio_flush` (buffer AVIO), `avcodec_flush_buffers` (decoder), `SDL_ClearAudioStream` (coda SDL).
- Nel tuo codice: **non implementato** (nessun `chunk_offsets`, nessuna UI).
- Decisioni: tua intuizione (202): saltare di un chunk (≈6,8 s) è più semplice e sicuro di ±5 s, perché un offset di
  inizio chunk è un punto pulito del container; un salto a byte "a caso" cade a metà di un pacchetto TS/AAC.
- Errori/lezione (da tenere a mente quando lo farai): `tail` va spostata **sotto mutex** (il decode thread la sta
  usando), conviene anche `avformat_flush` sul demuxer, e il ring buffer deve ancora contenere quei byte.
- Turni: 201, 202, 214

### Multithread produttore-consumatore: mutex + due condition variable
- Cos'è: un mutex protegge `head`, `tail` e i dati; due condition variable: "ci sono dati" (sveglia il consumatore) e
  "c'è spazio/serve rete" (sveglia il produttore). Regole: controllare il predicato **dopo** aver preso il mutex, sempre
  in un `while` (risvegli spuri), `pthread_cond_wait` va chiamata con lo **stesso** mutex che hai in mano, e chi produce
  fa `signal`, non `wait`. Mai fare I/O di rete tenendo il mutex.
- Nel tuo codice: globali `:127-130`; network thread `get_data` `:759-809` (attesa `while (available > THRESHOLD)`
  `:774-783`, `fetch` fuori dal lock `:788`, signal ai dati `:799-801`); consumatore `read_packet` `:337-358`
  (wait `:350`, signal alla rete `:357`); lock per singolo byte in scrittura `:579-581`; `pthread_create` `:1012-1013`.
- Decisioni: 2 thread invece del pool/event loop (216, 227); un solo mutex condiviso al posto di
  `audio_buffer_mutex` + `network_mutex` (233). ThreadSanitizer oggi non segnala data race (25 s di esecuzione).
- Errori/lezione, in ordine: (228) `pthread_create(..., (void *)&ctx)` con `ctx` già puntatore → `Ctx**` → segfault;
  (229) `fetch` dentro il mutex (audio bloccato durante il download) e `AVERROR_EOF`; (230) `tail` modificata fuori
  lock, check `head == tail` fuori lock (segnale perso), `signal` a ogni byte; (231) `sleep(1)` al posto di una condvar;
  (232) `wait` invece di `signal` in `read_packet` (gli altri due problemi indicati da Gemini, "unlock doppio" e
  `available` vecchia dopo la wait, nel tuo codice non c'erano: lock e unlock sono bilanciati e il `while(1)` ricalcola
  `available` a ogni giro);
  (233) `pthread_cond_wait(&cv, &audio_buffer_mutex)` tenendo `network_mutex`, cond non inizializzata.
  Residuo: l'attesa iniziale di `decode` usa `if` invece di `while` (`:815`).
- Turni: 214, 216, 217, 227, 228, 229, 230, 231, 232, 233, 234

### Riempimento a soglia e finestra sulla playlist live
- Cos'è: invece di scaricare tutta la playlist, il network thread scarica solo i **segmenti nuovi** (max 4) quando i
  byte disponibili scendono sotto `THRESHOLD` (50.000 byte ≈ 6 s di audio), ricordando l'ultimo numero di sequenza visto.
- Nel tuo codice: `THRESHOLD` `:48`; `last_sequence` `:105`; parsing del numero `:606-607`; logica "nuovi chunk"
  `:615-640` (tetto 4 `:628-630`, `audioCounter = 100 - chunks_to_fetch` `:631`); "No new chunks" `:616-620`.
- Decisioni: ogni ciclo riscarica master + sub-playlist e apre una connessione TLS per richiesta (`Connection: close`).
  Gemini (230) consiglia di salvare l'URL della sub-playlist e riscaricare solo quella.
- Errori/lezione: assunzioni fragili: esattamente 100 segmenti (`url_count == 100`), numero di sequenza letto dagli
  ultimi 10 caratteri della riga (vedi bug in `projects/audio-player.md`).
- Turni: 229, 230, 234

### Caccia ai memory leak: cosa era e come è stato trovato
- Cos'è: un leak è memoria non più raggiungibile e mai liberata. "Memoria che cresce" in Monitoraggio Attività **non** è
  per forza un leak: può essere memoria raggiungibile che cresce, o pagine già allocate che vengono toccate per la prima
  volta (page fault su memoria "pigra").
- Cronologia reale (con le versioni del codice):
  1. fino a 231 `initSSL` era dentro `fetch` (`INIT_CONNECTION`): un `SSL_CTX` nuovo a ogni connessione, mai liberato,
     con caricamento dei certificati da disco (visibile nel `sample` del 231: `X509_STORE_set_default_paths_ex`,
     footprint 48 MB). Spostato nel `main` al 232 (`:993`).
  2. (235) l'ultima connessione di ogni `fetch` restava aperta → pulizia dopo `fetch` (`:789-797`).
  3. (236) seguendo il consiglio del 235 avevi commentato la pulizia in `RECONNECT` → un `SSL` + socket persi a ogni
     richiesta. Ripristinata (`:390-406`).
  4. (237-239) filtro traccia audio + `av_packet_unref` fuori dall'`if` (`:962`): fix reale per i pacchetti ID3.
     `SSL_SESS_CACHE_OFF` (`:159`) e `av_frame_unref` (`:959`) sono innocui ma **non** erano leak (vedi errori di Gemini).
  5. (240-241) misurare invece di indovinare: `leaks <pid>` → `0 leaks for 0 total leaked bytes`.
  6. (241-242) la crescita residua è il ring buffer da 15 MB (e, verificato oggi, il buffer AVIO da ~5 MB) che vengono
     toccati progressivamente: ~8 KB/s di audio → il ring buffer impiega ~30 minuti a fare il primo giro.
- Verifica di oggi (4 minuti): numero di blocchi `malloc` stabile (~29.850, `heap <pid>`), 0 leak, footprint fisico
  12 → 17 MB. Strumenti: `leaks <pid>` (o `leaks --atExit -- ./prog`), `sample <pid>`, `heap <pid>`, `footprint <pid>`,
  Instruments (Leaks/Allocations), compilare con `-g`.
- Turni: 231, 235, 236, 237, 238, 239, 240, 241, 242

### Leggere un profilo `sample`
- Cos'è: `sample <pid>` fotografa gli stack di tutti i thread ogni 1 ms; il numero accanto a ogni funzione = quante
  fotografie la vedevano. Un thread sano di un player passa quasi tutto il tempo in attesa
  (`__psynch_cvwait` = condvar, `nanosleep` = `SDL_Delay`).
- Nel tuo codice: 231 → network thread in `sleep` + caricamento certificati in `fetch`; 234/242 → `get_data` 100% in
  `_pthread_cond_wait`, `decode` >99% in `SDL_SYS_DelayNS`, main in `pthread_join`.
- Turni: 231, 234, 242

### Decodifica hardware vs software
- Cos'è: la libreria FFmpeg (libavcodec) **non** fa fallback automatico hardware→software: l'accelerazione (`hwaccel`,
  es. VideoToolbox) va chiesta esplicitamente ed è pensata per il **video**. Per l'audio si usa la CPU: decodificare
  HE-AAC costa pochissimo. `aac_at` è il decoder AAC di AudioToolbox di Apple (un'API di sistema, non necessariamente
  un circuito dedicato).
- Nel tuo codice: `av_find_best_stream` `:862-863` sceglie il decoder software nativo `aac`.
- Turni: 219

### Architettura del player nel browser
- Cos'è: rete e UI in JS (`fetch`), demux/decodifica in un Web Worker (C → WebAssembly, es. `ffmpeg.wasm` o il tuo
  codice), riproduzione con un `AudioWorklet` (thread audio dedicato) oppure decodifica con WebCodecs (`AudioDecoder`).
  Wasm evita pause del garbage collector e va vicino alla velocità nativa.
- Nel tuo codice: nessuno (idea futura; ripresa nell'area actor model, turno 249).
- Turni: 218

### Download paralleli: thread pool vs event loop (rimando)
- Cos'è: per N chunk non servono N thread: K worker su una coda protetta da mutex, oppure un solo thread con socket non
  bloccanti e `epoll`/`kqueue`. Con download paralleli i chunk arrivano fuori ordine → "matrice di scatole" (uno slot
  per chunk) al posto del ring buffer lineare.
- Nel tuo codice: non implementato. Trattato nell'area `actor-model` (`notes/actor-model.md`).
- Turni: 214, 215, 216, 217

### Build multipiattaforma (pkg-config, Makefile)
- Cos'è: `pkg-config --cflags/--libs openssl sdl3 libavcodec libavformat libavutil` restituisce i flag giusti su Linux,
  Mac Intel (`/usr/local`) e M1 (`/opt/homebrew`); su Linux servono i pacchetti `-dev`.
- Nel tuo codice: `networking/Makefile:1-26` (ramo speciale per `/opt/homebrew`).
- Turni: 197, 203, 206

## Possibili errori o imprecisioni di Gemini
- **197, 204, 205, 207, 208** — `SDL_AUDIO_DEVICE_DEFAULT_OUTPUT`: in SDL3 la costante è
  `SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK` (quella che usavi già). Al 205 Gemini ha "corretto" il tuo codice giusto con quello
  sbagliato. Stessi turni: `SDL_ResumeAudioStream` non esiste (errore di compilazione al 206) e `SDL_InitSubSystem(...) < 0`
  è sbagliato in SDL3, dove la funzione restituisce `bool`.
- **200** — "se il ring buffer è vuoto `read_packet` restituisce 0 o un codice di blocco e FFmpeg aspetta": falso.
  0/`AVERROR_EOF` significa fine dello stream; per aspettare bisogna bloccarsi dentro la callback. Gemini stesso lo
  segnala come bug al 229 (dopo averlo suggerito al 199).
- **201-202** — il flush per il seek è incompleto: manca `avformat_flush` sul demuxer e soprattutto la protezione con il
  mutex di `tail` (il decode thread la usa in parallelo).
- **207** — "allargare `pcm_buffer` a 8192 per evitare stack smashing": 2048 campioni × 2 canali = 4096 float, il buffer
  da 4096 bastava. 8192 è solo margine.
- **211** — "con `MAXDATASIZE = 1` facevi una syscall per ogni byte": con TLS, `SSL_read` legge un intero record dal
  socket e poi serve i byte dal suo buffer. Il costo era l'overhead di una chiamata per byte, non una syscall per byte.
- **214, 221** — "ogni thread consuma 2-8 MB di stack, 99 thread ≈ 1 GB di RAM": lo stack è memoria virtuale riservata,
  quella fisica conta solo le pagine usate (e su macOS i thread secondari hanno 512 KB di default).
- **218 vs 219** — al 218 WebCodecs `AudioDecoder` "usa il decoder hardware del chip M1, la CPU dorme"; al 219 Gemini dice
  che l'accelerazione hardware è per il video e quasi mai per l'audio. Si contraddice; la decodifica audio nel browser è
  di norma software. Al 219 inoltre `aac_at` viene presentato come "decoder hardware".
- **222** — i float sono "tassativamente" tra -1 e +1: è il range nominale, un decoder può produrre valori leggermente
  oltre (clipping a valle).
- **225** — la "scalinata" del DAC viene chiamata *aliasing*: in riproduzione si parla di immagini spettrali
  (*imaging*); l'aliasing si evita con il filtro anti-aliasing **prima** dell'ADC, che Gemini non cita.
- **226** — "i dati viaggiano come coefficienti con la virgola": l'AAC trasmette coefficienti MDCT **quantizzati interi**
  (più scalefactor, codifica Huffman); i float nascono nel decoder.
- **231** — vede `initSSL` dentro `fetch` solo come problema di prestazioni: è anche un leak (un `SSL_CTX` per
  connessione mai liberato; footprint 48 MB nel `sample`).
- **235** — consiglia di togliere la pulizia da `RECONNECT`: seguendolo hai creato il leak "trovato" al 236.
- **237** — "la session cache TLS 1.3 di OpenSSL accumula migliaia di sessioni nel `SSL_CTX`": lato client OpenSSL non
  memorizza sessioni nella cache interna per default (modo predefinito `SSL_SESS_CACHE_SERVER`) e la cache comunque è
  limitata. `SSL_SESS_CACHE_OFF` è innocuo ma non era la causa. Inventato anche "FFmpeg accumula errori di parsing → micro-leak".
- **239** — "manca `av_frame_unref` → leak di decine di MB al minuto": falso, `avcodec_receive_frame` chiama
  `av_frame_unref` sul frame prima di riempirlo. La riga aggiunta (`:959`) è innocua.
- **241** — "0 leaks quindi non perdi nemmeno un byte": `leaks` trova solo blocchi irraggiungibili, non la crescita di
  memoria raggiungibile (qui la conferma viene da `heap` stabile). Il plateau "45-48 MB come il picco del 231" usa un
  riferimento sbagliato: quel 48 MB era la versione con il leak di `SSL_CTX`.
- **hls_player.md** — "i segmenti sono 99" (oggi sono 100); "Race Condition = crash inevitabile" (spesso è corruzione
  silenziosa); "SDL3 bussa con una callback" (tu usi lo stream push).

## Domande aperte / cose non chiarite
- Pausa/play: cosa fa il network thread durante una pausa lunga? (Pause policy di `hls_player.md`: flush e ritorno al
  live edge se i segmenti sono scaduti sul CDN). Mai implementata.
- Uscita pulita: nessun flag `quit`; la pulizia in `get_data` (`:804-808`) e in `decode` (`:967-979`) è irraggiungibile,
  `free(ctx)` dopo `pthread_exit` (`:1026-1028`) non viene mai eseguita.
- Quanto deve essere grande davvero il ring buffer con la finestra a 4 chunk? (sopra soglia + 4×62 KB ≈ 300 KB).
- `SDLBUFFER` e `THRESHOLD` dipendono da frequenza e bitrate: come calcolarli dalla `spec` e dalla playlist?
- Il resampling (se il dispositivo è a 48 kHz) lo fa SDL3: mai discusso; `libswresample` non è stato usato.
- TLS senza verifica del certificato (`SSL_set_verify` non chiamata) e senza SNI: discusso nell'area networking?
- Il buffer AVIO cresciuto a ~5 MB dal probing: ridurre `probesize`/`analyzeduration` accorcerebbe anche l'avvio?

## Argomenti a margine (non legati a progetti: titolo + turni + 1 riga)
- Installazione su Kubuntu (apt `-dev`, SDL3 da sorgente) — 203 — pacchetti di sviluppo e `pkg-config`.
- Cache locality vs prefetch — 212 — la località è una proprietà del codice, il prefetch una strategia dell'hardware.
- Cache locality nel broker MQTT — 213 — alberi dei topic su array, hash con open addressing, fan-out senza copie.
- XState, actor model, Erlang/BEAM — 220-221 — vedi `notes/actor-model.md`.

## Domande di autoverifica
1. Perché 67171 bit/s × 6,83 s non coincide con il `Content-Length` di un segmento `.ts`? Da dove vengono i byte in più?
2. Nel ring buffer, cosa rappresenta `tail`: la posizione di ascolto o altro? Chi la fa avanzare e in quale funzione?
3. Con soli `head` e `tail`, come distingui buffer vuoto e buffer pieno? Cosa fa oggi `enqueueAudioBuffer` se il buffer è pieno?
4. Perché `Ctx ctx;` con un buffer da 15 MB va in segfault e `calloc` no? L'accesso all'heap è più lento?
5. Cosa significano `opaque`, `buf` e `buf_size` in `read_packet`? Cosa succede se restituisci `AVERROR_EOF` in uno stream live?
6. Perché a fine programma devi liberare `avio_ctx->buffer` e non il puntatore che hai passato ad `avio_alloc_context`?
7. Spiega la differenza tra AVPacket e AVFrame e l'ordine di `av_read_frame`, `avcodec_send_packet`, `avcodec_receive_frame`.
8. Che differenza c'è tra FLTP e FLT? Perché `decoded_frame->data` "sembra un solo array"?
9. Cosa rappresenta `352800` in `SDLBUFFER`? Come lo ricaveresti per 48 kHz mono?
10. Chi impone la velocità di riproduzione e come il decode thread si adegua? Cosa succederebbe senza il controllo su `SDL_GetAudioStreamQueued`?
11. Perché `pthread_cond_wait` va in un `while` e il predicato va controllato dopo aver preso il mutex? Descrivi il "segnale perso" del turno 230.
12. Perché non si fa la `fetch` tenendo il mutex? E perché non si può usare una condition variable sulla coda di SDL?
13. Ricostruisci la caccia ai leak: quali erano leak veri, quali fix erano inutili e perché la memoria continuava comunque a salire?
14. Per "torna alla diretta" quali buffer vanno svuotati, in che ordine, e quali problemi di concorrenza ci sono?
15. Il numero di sequenza viene letto con `atol(line_buf + l_idx - 10)`: cosa succede quando la sequenza arriva a 10.000.000?
