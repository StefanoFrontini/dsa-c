# Audio player HLS "Underground Radio" (versione multithread)  — stato: working
Path: networking/client_http.c (1031 righe) → binario `networking/underground_radio` · Ultima modifica: 2026-07-07
(commit `client_http_38`, 539b880; la logica è quella di fine conversazione, turno 242 del 06/07)

Nota sul path: la scheda prevista indicava `networking/client_http_3.c`, ma quel file (405 righe, 18/06) è un vecchio
client HTTP in chiaro verso `example.com` senza audio. Il `Makefile` (`networking/Makefile:22-23`) compila
`client_http.c` in `underground_radio`. Il binario nel repo è **x86_64** (non arm64), compilato il 06/07 alle 18:16,
cioè durante la caccia ai leak (turno 241): coincide con questo sorgente e funziona anche oggi.

## Cosa fa
Player della diretta di Radio24 (HLS su HTTPS, Akamai) in C puro:
1. **network thread** (`get_data` `:759`): scarica master playlist → sub-playlist `playlist-64000.m3u8` (100 segmenti)
   → solo i segmenti nuovi (max 4) e scrive i byte `.ts` in un ring buffer da 15 MB; poi dorme su una condition
   variable finché i byte disponibili restano sopra 50.000;
2. **decode thread** (`decode` `:811`): FFmpeg legge dal ring buffer tramite `read_packet` (`:333`), demux MPEG-TS,
   decodifica HE-AACv2 → PCM float planar, interleaving a mano e push in uno `SDL_AudioStream` (SDL3), tenendo la coda
   sotto 3 secondi.
Nessuna interfaccia: niente pausa, seek, diretta o uscita (si chiude con Ctrl-C).

## Come si compila / si esegue   (comandi verificati da te)
```sh
BUILD=/private/tmp/claude-501/.../scratchpad/build/audio
cd networking
cc -W -Wall -O2 -g $(pkg-config --cflags openssl sdl3 libavcodec libavformat libavutil) client_http.c \
   $(pkg-config --libs openssl sdl3 libavcodec libavformat libavutil) -o $BUILD/underground_radio
perl -e 'alarm 30; exec @ARGV' -- $BUILD/underground_radio < /dev/null
```
- Anche `make` funziona su questo Mac Intel (`make -n` genera percorsi `/usr/local/Cellar/...`), ma scrive il binario nel repo.
- Librerie trovate: openssl 3.6.5, sdl3 3.4.10, libavcodec 62.28, libavformat 62.12, libavutil 60.26.
  `libswresample` non serve (interleaving manuale).
- Leak su un processo che non termina: `leaks <pid>`, `heap <pid>`, `footprint <pid>`, `sample <pid>` da un secondo terminale.

## Esito verifica   (output reale abbreviato, warning, leaks, test passati/falliti)
- Compilazione: **0 warning** con `-W -Wall -O2`.
- Esecuzione 30 s (exit 142 = SIGALRM, previsto: il programma non termina mai da solo):
  ```
  Stream #0:0[0x100]: Audio: aac (HE-AACv2), 44100 Hz, stereo, fltp, 63 kb/s
  Stream #0:1[0x101]: Data: timed_id3
  [INFO] Start downloading static file (160 byte)        <- master playlist
  [INFO] Start downloading static file (5455 byte)       <- sub-playlist, 100 segmenti
  Chunks to fetch: 4 ... 03973/seg64000-07944575.ts (61476 byte) ... 07944578.ts
  [SUCCESS] All audio chunks copied to Ring Buffer!
  [NETWORK] Buffer pieno (245528 byte). Mi metto in pausa...
  [SUCCESS] FFmpeg ha agganciato il Ring Buffer con successo!
  [AUDIO] Motore SDL3 avviato (2 canali @ 44100 Hz). Riproduzione in corso...
  [NETWORK] Buffer pieno (233240 byte) ... (53016 byte)   <- scende di 4096 a ogni read_packet
   Fetching data...  Previous last sequence was: 7944578  Last sequence is: 7944581  Chunks to fetch: 3
  ```
  Scarica la playlist, riempie il buffer, decodifica e riparte a soglia. (Il suono non è stato verificato.)
- Esecuzione di 4 minuti: 12 fetch, ogni volta 3 segmenti nuovi, mai "No new chunks".
  `heap <pid>`: ~29.850 blocchi malloc stabili, stesse dimensioni (blocco da 14.744 KB = `Ctx`, blocco da 4.928 KB =
  buffer AVIO). `leaks <pid>`: **0 leaks for 0 total leaked bytes**. Footprint fisico 12 → 17 MB: pagine del ring buffer
  e del buffer AVIO toccate per la prima volta, non leak (vedi `notes/audio.md#caccia-ai-memory-leak-cosa-era-e-come-è-stato-trovato`).
- Build di prova (copia nella build dir): dopo `avformat_find_stream_info` il buffer AVIO è riallocato a 5.032.767 byte.
  Conferma la spiegazione del `trace trap` del turno 207.
- ThreadSanitizer (`-fsanitize=thread`, 25 s con riproduzione avviata): **0 warning**.
- Il binario precompilato del repo (x86_64) eseguito per 15 s: stesso comportamento.

## Cosa funziona
- Pipeline completa HTTPS → HLS → ring buffer → FFmpeg custom I/O → decoder AAC → SDL3.
- Produttore/consumatore con un mutex e due condition variable, `fetch` fuori dal lock, attese in `while`
  (`:339-351`, `:774-783`).
- Pacing sulla coda SDL (`:911-914`): CPU quasi a zero (decode >99% in `nanosleep`, rete in `cond_wait`).
- Liberazione corretta delle connessioni (`:390-406`, `:789-797`), filtro della traccia audio e `av_packet_unref`
  sempre eseguito (`:917`, `:962`): nessun leak.
- Deduplica dei segmenti tramite `last_sequence` (`:616-631`).

## Cosa manca / bug noti   (con file:riga)
- **Bomba a tempo sul numero di sequenza** `:606-607`: `atol(line_buf + l_idx - 10)` legge solo le ultime 7 cifre di
  `seg64000-07944643.ts`. Quando la sequenza arriverà a 10.000.000 (stima: metà marzo 2027, un segmento ogni 6,83 s)
  verrà letto 0 → `chunks_to_fetch` negativo → `audioCounter = 100 - chunks_to_fetch` enorme → `urls_buf[audioCounter]`
  fuori dai limiti in `setAudioRequest` (`:201`) → crash o comportamento indefinito. Meglio cercare l'ultimo `-` con
  `strrchr` e usare `strtol`.
- Assume una playlist di **esattamente 100** segmenti (`:615`, `urls_buf[100]` `:103`). Con meno righe non scarica
  niente e `get_data` ricomincia subito a interrogare il server in un loop stretto (handshake TLS a raffica).
  Lo stesso succede se viene chiamata `fetch` prima che esca un segmento nuovo (`return` a `:619`, nessuna attesa).
- Ring buffer sovradimensionato: `MAXAUDIOBUFFER 15000000` (`:45-46`) contro ~250 KB di dati vivi. La memoria fisica
  cresce per ~30 minuti; `enqueueAudioBuffer` (`:132-136`) non controlla mai se il buffer è pieno.
- Lock/unlock del mutex **per ogni byte** ricevuto (`:579-581`, ~61.000 per segmento) e copia byte per byte in
  `read_packet` (`:353-355`): meglio copiare a blocchi (`memcpy` in due tratti per il giro).
- TLS senza verifica: `SSL_CTX_set_default_verify_paths` (`:161`) carica le CA ma non c'è `SSL_set_verify`/`SSL_set1_host`,
  né SNI (`SSL_set_tlsext_host_name`) → nessuna protezione da man-in-the-middle. `servinfo` non liberato se
  `SSL_connect` fallisce (`:273-277`).
- Ogni ciclo riscarica anche la master playlist e apre una connessione per richiesta (`Connection: close`, `:174`).
- `av_read_frame` con errore (`:915`) viene ignorato e il loop riparte senza pausa (possibile busy loop); errore di
  `avcodec_send_packet` → `break` (`:923`): il decode thread termina ma il programma resta vivo e muto.
- Attesa iniziale del decode con `if` invece di `while` (`:815`); in caso di errore `free(ctx)` (`:839` ecc.) mentre
  l'altro thread usa ancora `ctx`.
- Valori scritti a mano: `SDLBUFFER` (`:47`) vale solo per 44,1 kHz stereo float; ramo "packed" (`:954-957`) presuppone
  float; nessun controllo che `samples*channels <= 8192` (`:945-950`).
- Nessuna uscita: cleanup irraggiungibile (`:804-808`, `:967-979`), `free(ctx)` dopo `pthread_exit` (`:1026-1028`).
- Roadmap non completata (vedi `networking/gemini-audio-player.md:2758-2800` e `networking/hls_player.md`): UI da
  terminale (play/pausa, chunk ±1, diretta, uscita, status line) del turno 214, seek tramite `chunk_offsets` (201-202),
  pause policy, download paralleli (thread pool/event loop, 216-217). Il passo successivo è stato riscrivere il player
  ad attori in `networking/actor/client_http_actor.c` (scheda `audio-player-actor.md`, oggi non compila).

## Concetti applicati   (rimanda a notes/audio.md#...)
- `notes/audio.md#segmenti-hls-ts-bitrate-e-content-length`
- `notes/audio.md#ring-buffer-di-byte-compressi-headtail`
- `notes/audio.md#stack-vs-heap-per-buffer-grandi`
- `notes/audio.md#io-custom-di-ffmpeg-avio_alloc_context--read_packet`
- `notes/audio.md#pipeline-ffmpeg-container--pacchetti--decoder--frame`
- `notes/audio.md#pcm-formati-planar-e-packed`
- `notes/audio.md#output-audio-con-sdl3-audiostream-modello-push`
- `notes/audio.md#sincronizzazione-velocità-di-decodifica--velocità-di-riproduzione-pacing`
- `notes/audio.md#multithread-produttore-consumatore-mutex--due-condition-variable`
- `notes/audio.md#riempimento-a-soglia-e-finestra-sulla-playlist-live`
- `notes/audio.md#caccia-ai-memory-leak-cosa-era-e-come-è-stato-trovato`

## Prossimo passo consigliato   (1-3 passi concreti e piccoli)
1. Correggere il parsing della sequenza (`:606-607`) con `strrchr(line, '-')` + `strtol`, e sostituire
   `url_count == 100` con "fine della playlist" (`content_length == 0`) tenendo l'ultimo URL visto.
2. Ridurre `MAXAUDIOBUFFER` a ~1 MB, aggiungere il controllo "pieno" in `enqueue` e scrivere/leggere a blocchi con un
   solo lock per `SSL_read`. Poi verificare con `footprint <pid>` che la memoria resti piatta.
3. Aggiungere un flag `quit` (tasto `q`, come nel turno 214) per far uscire entrambi i thread e raggiungere la pulizia,
   così da poter usare `leaks --atExit`.
