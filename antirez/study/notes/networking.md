# Networking: socket, HTTP, TLS, HLS
Periodo: 2026-05-27 → 2026-06-30 (+ 2026-08-05 per Shoutcast) · Turni: 137-203 (citati anche 275-277)

## Mappa in breve
- Parte da una domanda sul parser di accordi (leggere a pezzi invece che tutto in RAM, turni 137-138) e arriva al progetto "Underground Radio": un player HLS di Radio 24 in C.
- La strada è a tappe (roadmap del turno 164): `old.c` (getaddrinfo) → `echo_client.c` (socket/connect/send/recv) → client HTTP in chiaro su example.com → parser dell'header come macchina a stati + body chunked (testato con `mock_server.py`) → HTTPS con OpenSSL verso Akamai → playlist HLS → segmenti `.ts` in un ring buffer → decodifica FFmpeg + SDL3 (area audio).
- Tutto il codice di rete è in `networking/client_http.c` (oggi è anche il player completo: il Makefile lo compila come `underground_radio`). `client_http_2.c` (non compila) e `client_http_3.c` (solo header) sono versioni intermedie del parser: vedi projects/http-client.md. Il player è descritto in projects/audio-player.md (area audio).
- Il libro di riferimento consigliato al turno 195 (UNP di Stevens) è all'origine di `networking/daytime_client/` (luglio, nessun turno dedicato).
- Collegamenti: area **audio** (FFmpeg/SDL3, `read_packet`, thread audio), area **concorrenza** (mutex/condition variable del ring buffer), il parser di accordi (lexer/Pratt, turno 151) e, più avanti, il server asincrono / broker MQTT (argomenti a margine).

## Concetti

### Leggere a pezzi: buffer scorrevole vs mmap
- Cos'è: un file troppo grande per la RAM si può leggere a blocchi di dimensione fissa, spostando in testa al buffer la riga rimasta a metà (`memmove`) e poi riempiendo il resto (*refill*), oppure si può mappare con `mmap`: è il kernel a caricare le pagine quando le tocchi. `mmap` funziona solo con file che esistono già e hanno una dimensione nota. Non funziona con un socket, cioè con un flusso infinito che arriva dalla rete.
- Nel tuo codice: è lo stesso schema del byte stream iterator. `recv_buf` ha una dimensione fissa (`MAXDATASIZE`) e si riempie di nuovo solo quando hai consumato tutti i byte (`networking/client_http.c:285-323`).
- Decisioni: per la radio scarti `mmap` e scegli lo streaming con ring buffer (turno 138).
- Turni: 137, 138

### Architettura del player: thread di rete, ring buffer, decoder, thread audio
- Cos'è: un thread di rete (I/O bound) scrive i byte compressi nel ring buffer. Il thread audio (vincolato al tempo reale) li legge, li decodifica in PCM e li passa alla scheda audio. Il ring buffer fa da ammortizzatore (*jitter buffer*) tra la rete, che arriva a raffiche, e l'audio, che consuma a ritmo costante.
- Nel tuo codice: `get_data` è il thread di rete (`networking/client_http.c:759-809`), `decode` il thread audio (`:811-980`). Vengono creati con `pthread_create` (`:1012-1013`). La sincronizzazione usa `audio_buffer_mutex` e due condition variable (`:127-130`).
- Decisioni: thread separati perché `recv`/`SSL_read` sono bloccanti. Con un thread solo, un blocco della rete fermerebbe l'audio e la UI (turno 179). Gli appunti dell'architettura sono in `networking/hls_player.md` (turno 163).
- Turni: 138, 146, 163, 179

### Il browser come player e WebAssembly
- Cos'è: Chrome/Safari sanno già riprodurre HLS. Un modulo C compilato in wasm (Emscripten) non può aprire socket veri: Emscripten traduce i socket in WebSocket/fetch. L'architettura sensata nel browser: JS scarica i dati, il C/wasm decodifica e il ring buffer vive nella memoria lineare di wasm.
- Nel tuo codice: niente ancora. Resta un'idea per dopo, come la UI di telemetria con riempimento del buffer, KB/s e chunk corrente (turno 158).
- Decisioni: prima il player nativo per macOS. Il "compilo in wasm senza cambiare una riga" non è realistico, perché rete e audio vanno riscritti (turno 141).
- Turni: 139, 140, 141, 155, 158

### getaddrinfo, hints e la lista addrinfo
- Cos'è: `getaddrinfo(host, porta, &hints, &res)` risolve il nome e restituisce una **lista concatenata** di `struct addrinfo` (famiglia, tipo di socket, `ai_addr`). `hints.ai_family = AF_UNSPEC` chiede sia IPv4 sia IPv6, `hints.ai_socktype = SOCK_STREAM` solo TCP. `inet_ntop` converte l'indirizzo in stringa. La lista va liberata con `freeaddrinfo`.
- Nel tuo codice: `networking/old.c:20-51` stampa tutti gli IP. Oggi per Akamai restituisce 2 IPv6 + 2 IPv4, per example.com 2 + 2 (verificato).
- Errori/lezione: `socktype` a 0 con il servizio "http" restituisce anche `SOCK_DGRAM`, ma questo dice solo cosa c'è in `/etc/services`, non cosa supporta davvero il server (vedi errori di Gemini).
- Turni: 142, 143

### Più indirizzi IP e failover
- Cos'è: più record A/AAAA servono a bilanciare il carico (DNS round robin) e a dare ridondanza (CDN Akamai). TCP **non** passa da solo a un altro IP: una connessione è legata alla quaterna (IP e porta locali, IP e porta remoti). Il failover lo scrivi tu: chiudi il socket, ne apri uno nuovo verso l'IP successivo e richiedi lo stesso segmento.
- Nel tuo codice: il ciclo "prova il prossimo IP" esiste solo al momento della connessione (`networking/echo_client.c:37-68`, `networking/client_http.c:228-263`). Se invece cade una connessione già aperta, `fetch` va in `CONNECTION_ERROR`/`HEADER_ERROR` e chiama `exit(1)` (`networking/client_http.c:737-751`). Il failover a metà stream non c'è ancora.
- Errori/lezione: un `connect()` verso un IP che non risponde **non** fallisce in pochi microsecondi, ma resta bloccato fino al timeout del kernel. Durante il test di `daytimetcpcli_IPv6` verso NIST è rimasto bloccato oltre 10 s. Per un failover rapido servono un connect non bloccante con `poll` e un timeout.
- Turni: 143, 144, 145

### socket / connect / send / recv
- Cos'è: `socket()` crea il descrittore, `connect()` fa il three-way handshake TCP, `send()` spinge byte nel buffer del kernel (può inviarne meno di quanti chiesti), `recv()` restituisce **quello che c'è** (da 1 byte al massimo richiesto). Restituisce 0 se il peer ha chiuso e -1 in caso di errore.
- Nel tuo codice: `networking/echo_client.c:53-94` (un solo `send` e un solo `recv`). La versione con le funzioni wrapper alla Stevens è `networking/daytime_client/daytimetcpcli.c:14-24,55-61`.
- Errori/lezione: con tcpbin.com il client restava appeso perché il messaggio non aveva `\n`. Il server lavora a righe (turno 164, verificato: senza `\n` va in timeout, con `\n` torna l'eco, `networking/echo_client.c:77`). Il daytime server scrive 1 byte per `write` (`networking/daytime_client/daytimetcpsrv.c:36-44`) e il client conta quante `read` servono. In locale escono 1 o 2 letture per 26 byte: TCP è un flusso, i confini delle `write` non si conservano.
- Turni: 142, 149, 164, 168, 169

### recv bloccante e lettura a pezzi
- Cos'è: `recv` va **sempre** in un ciclo, perché un messaggio può arrivare spezzato in un numero qualsiasi di pezzi. Il buffer di ricezione è piccolo e fisso. I dati vengono elaborati mentre arrivano (streaming) invece di accumularli con `realloc`. `recv` blocca il thread finché non arrivano byte; senza timeout (`SO_RCVTIMEO`, `poll`) può restare bloccata per sempre.
- Nel tuo codice: `MAXDATASIZE 10` e ciclo `while (recv > 0)` nel turno 169. Il parser poi è stato testato con `MAXDATASIZE 1`, 3, 1000 e 8192 (verificato: stesso output). Oggi è `MAXDATASIZE 8192` (`networking/client_http.c:36`).
- Errori/lezione: `numbytes` era un `size_t`, così -1 diventava 18446744073709551615 e il controllo `== 0` non bastava. Ora `readChunk` usa un `int` e il controllo `res <= 0` (`networking/client_http.c:286-304`, turni 172 e 192). Manca ancora un timeout: in metro `SSL_read` può bloccarsi per sempre senza errore (non c'è FIN né RST).
- Turni: 168, 169, 170, 179

### Struttura di una richiesta HTTP/1.1 e header
- Cos'è: request line `GET /path HTTP/1.1\r\n`, poi gli header `Chiave: valore\r\n`, una riga vuota `\r\n` e il body opzionale. `Host` è l'unico header obbligatorio in HTTP/1.1 (virtual hosting). `Connection: close` fa chiudere il socket al server a fine risposta. `User-Agent` è facoltativo (SHOULD). Con un body servono `Content-Length` e `Content-Type`.
- Nel tuo codice: `setRequest` e `setAudioRequest` (`networking/client_http.c:166-202`), con UA `UndergroundRadio/1.0` e `Connection: close`.
- Decisioni: `Connection: close` per semplicità, quindi una nuova connessione TCP + TLS per ogni playlist e ogni segmento (`RECONNECT`, `networking/client_http.c:390-406`). Il keep-alive non c'è ancora.
- Errori/lezione: Akamai **non** richiede lo User-Agent: risponde 200 anche senza (verificato con curl e nc).
- Turni: 149, 165, 166, 167

### Parser dell'header come macchina a stati che consuma byte (byte stream iterator)
- Cos'è: invece di aspettare `\r\n\r\n` e usare `strstr`, il parser è una FSM che legge **un byte alla volta** da `getNextByte`. Questa restituisce il byte successivo di `recv_buf` e chiama `readChunk` (cioè `recv`/`SSL_read`) quando il buffer è finito. Così il parser non sa nulla dei confini dei pacchetti e funziona anche con buffer da 1 byte. È un parser "pull" (il parser chiama la rete). I parser "push" come `llhttp` invece ricevono i buffer dal chiamante.
- Nel tuo codice: `getNextByte` (`networking/client_http.c:312-323`). Stati `HEADER_STATUS_LINE` (codice di stato dai byte 9-11, `:447-470`), `HEADER_KEY` (riga vuota = fine header, `:472-506`), `HEADER_VALUE` (riconosce Content-Length e Transfer-Encoding, `:507-539`), `HEADER_CRLF` (`:541-554`), `HEADER_DONE` (sceglie il tipo di body, `:555-568`).
- Decisioni: hai scartato il recursive descent (turno 151: la grammatica HTTP è piatta). Hai scartato l'accumulatore `header_accumulator` + `strstr` (turno 170) e la versione con stati `_ACC` duplicati (turno 171, "spaghetti code": ne restano tracce come codice morto in `networking/client_http_3.c:160-225`, che nella forma finale usa già `getNextByte`, `:227-238`). Hai adottato il byte stream iterator suggerito al turno 171.
- Errori/lezione (turno 172, bug reali nella tua versione): un `recv_idx++` di troppo in `HEADER_VALUE` saltava un byte su due, il `\n` finale dell'header restava non letto, `readChunk` non intercettava -1. Il codice FSM di Gemini al turno 152 invece leggeva `buffer[i+1]`/`buffer[i+2]` oltre la fine del buffer: funziona solo se l'header sta tutto in memoria.
- Limiti rimasti: si salta un solo spazio dopo `:` (`:533`) e gli spazi finali non vengono tolti. `Transfer-Encoding: gzip, chunked` non viene riconosciuto (confronto esatto, `:521-522`). Le risposte con solo `\n` (senza `\r`) vengono rifiutate (verificato con il mock). Un 3xx (redirect) non è gestito. Uno status >= 400 chiude il programma (`:459-460` → `exit(1)`).
- Turni: 150, 151, 152, 170, 171, 172

### Content-Length vs Transfer-Encoding: chunked
- Cos'è: il body ha una lunghezza nota (`Content-Length: N`, leggi esattamente N byte) oppure arriva a pezzi: `<dimensione esadecimale>\r\n<dati>\r\n` ... `0\r\n` + eventuali trailer + `\r\n`. È un meccanismo standard di HTTP/1.1 (RFC 9112 §7.1), non un "formato proprietario". Se ci sono entrambi gli header vince `chunked`.
- Nel tuo codice: `BODY_CHUNKED_SIZE` (`networking/client_http.c:674-704`, `strtol(...,16)`), `BODY_CHUNKED_HTML` (consuma subito il `\r\n` dopo i dati, `:706-728`), `BODY_CONTENT_LENGTH` (playlist e segmenti, `:570-672`). Akamai risponde sempre con Content-Length (playlist da 160 byte, segmenti da circa 61 KB). example.com e il mock rispondono chunked (`210` esadecimale = 528 byte, turno 167).
- Errori/lezione: nel turno 173 il `\r` dopo il chunk veniva perso e il `\n` finiva nel `size_buffer`. Funzionava solo perché `strtol` salta gli spazi iniziali. Con il mock il byte `\r` dentro `Wi\rki` viene stampato davvero: il terminale mostra `ki` perché `\r` riporta il cursore a inizio riga (turno 175; verificato con `cat -v`: `Wi^Mki`). Le estensioni `5;foo=bar` funzionano per caso (`strtol` si ferma al `;`). I trailer dopo lo `0` non vengono letti.
- Turni: 150, 167, 173, 174, 175, 192

### Test locale con mock server
- Cos'è: un server Python che risponde sempre con la stessa risposta chunked, con pause tra un chunk e l'altro. Serve per testare il parser in modo ripetibile, senza dipendere da internet.
- Nel tuo codice: `networking/mock_server.py` (porta 8080) e l'output atteso in `networking/output.txt`. La versione del client che parla con il mock è quella del commit `0de4502` (client_http_19). Ricompilata oggi produce un output identico a `output.txt` (byte per byte, `\r` escluso). Funziona anche con `MAXDATASIZE` 3/1000/8192 e non ha leak.
- Turni: 174, 175

### OpenSSL: SSL_CTX, SSL_connect, SSL_read/SSL_write, SNI, verifica
- Cos'è: TLS sta tra TCP e HTTP. `SSL_CTX` contiene la configurazione condivisa (`TLS_client_method()`, CA di sistema). `SSL` rappresenta una singola connessione: `SSL_set_fd` la lega al socket già connesso, `SSL_connect` fa l'handshake, poi `SSL_write`/`SSL_read` sostituiscono `send`/`recv` e `SSL_shutdown` + `SSL_free` + `close` chiudono. L'handshake moderno (TLS 1.3) usa uno scambio Diffie-Hellman effimero: il certificato serve a **firmare**, non a cifrare il segreto. Poi si usa la crittografia simmetrica (AES-GCM).
- Nel tuo codice: `initSSL` (`networking/client_http.c:145-164`), handshake in `initConnection` (`:270-277`), `SSL_read` in `readChunk` (`:286`), `SSL_write` (`:432`). Akamai oggi negozia TLSv1.3 con `TLS_AES_256_GCM_SHA384` (verificato con s_client).
- Errori/lezione: **il certificato non viene verificato**. `SSL_CTX_set_default_verify_paths` carica le CA, ma senza `SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL)` e `SSL_set1_host` l'handshake riesce con qualsiasi certificato. Verificato con la stessa configurazione: `SSL_connect=1` con expired, self-signed e wrong.host di badssl.com. Manca anche la **SNI** (`SSL_set_tlsext_host_name`). Con Akamai funziona lo stesso perché il server restituisce comunque il suo certificato `a248.e.akamai.net`, ma molti server senza SNI danno un certificato sbagliato o rifiutano. Il `-1` di `SSL_read` va interpretato con `SSL_get_error` (`:290-302`).
- Turni: 149, 169, 185, 187, 189, 190, 191, 192, 193

### Dove sono gli header di sistema; pkg-config
- Cos'è: `#include <x.h>` cerca nei percorsi di sistema. Su Linux (Kubuntu) sono `/usr/local/include`, `/usr/include` e quelli interni di gcc, con le librerie in `/usr/lib/x86_64-linux-gnu` (multiarch). Il linker trasforma `-lssl` in `libssl.so` o `libssl.a`. I pacchetti `-dev` installano header e link. `pkg-config --cflags --libs openssl sdl3 libavcodec ...` restituisce i flag giusti per ogni macchina.
- Su macOS **non esiste `/usr/include`** (verificato): gli header di sistema stanno nell'SDK (`xcrun --show-sdk-path` → `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk`), quelli di Homebrew in `/usr/local` (Intel) o `/opt/homebrew` (M1).
- Nel tuo codice: `networking/Makefile` usa pkg-config, con un caso a parte per `/opt/homebrew`. Su questo Mac Intel `make -n` produce il comando corretto e la build compila senza warning.
- Turni: 185, 186, 187, 188, 197, 203

### HLS: master playlist, sub-playlist, segmenti .ts
- Cos'è: HTTP Live Streaming. Una **master playlist** (`playlist.m3u8`) elenca le varianti (`#EXT-X-STREAM-INF:BANDWIDTH=...,CODECS=...`). Una **media playlist** (`playlist-64000.m3u8`) elenca i segmenti, con `#EXT-X-MEDIA-SEQUENCE` (numero del **primo** segmento), `#EXT-X-TARGETDURATION`, `#EXTINF:durata` e gli URL relativi alla cartella della playlist. I segmenti `.ts` sono MPEG-TS: pacchetti da 188 byte che iniziano con `0x47`. Tutto viaggia su HTTP, quindi su TCP.
- Dati reali (verificati il 2026-10-03): Radio 24 ha un'unica variante `BANDWIDTH=67171, CODECS="mp4a.40.5"`, cioè **niente ABR**. La sub-playlist ha **100** segmenti da 6.82667 s (circa 11.4 min, non 99), `TARGETDURATION:7`, URL del tipo `03973/seg64000-07944541.ts`. Un segmento pesa 61476 byte = 327 × 188 (61288 = 326 × 188 nel turno 194). Lo stream audio secondo ffprobe è AAC HE-AACv2 a 44.1 kHz stereo, più una traccia `timed_id3`. Lo stesso flusso è disponibile anche in **HTTP sulla porta 80** (200 OK).
- Nel tuo codice: parser a righe dentro `BODY_CONTENT_LENGTH` (`networking/client_http.c:587-643`). Le righe `#` vengono ignorate, la prima riga `.m3u8` porta a `SUB_PLAYLIST`, le righe `.ts` vanno in `urls_buf`. Il numero di sequenza si estrae con `atol(line_buf + l_idx - 10)` (`:606-607`).
- Decisioni: parser a righe invece di una FSM per la playlist (turno 163). Si scaricano solo gli ultimi ≤4 segmenti (`:627-631`). Il vecchio piano di scaricarli tutti e 99 (turni 162 e 196) è stato abbandonato.
- Errori/lezione: per 61288 byte ti aspettavi circa 458554. È la confusione tra bit e byte: 458554 bit / 8 = 57319 byte. Il resto è overhead del contenitore TS (turno 194). L'estrazione del numero con l'offset fisso `-10` è fragile: dipende dalla lunghezza del nome, e con `l_idx < 10` legge prima dell'inizio del buffer. Anche `url_count == 100` è un numero fisso (`:615`): se la playlist ha meno di 100 righe non si scarica nulla.
- Turni: 142, 153, 154, 159, 160, 161, 162, 193, 194

### Adaptive bitrate (ABR)
- Cos'è: il server offre più varianti con bitrate diversi e segmenti allineati nel tempo. Il client misura la banda (byte scaricati / tempo) e cambia variante segmento per segmento.
- Nel tuo codice: non applicabile. Radio 24 ha una sola variante. Il codice prende la prima `.m3u8` della master (`networking/client_http.c:596-601`).
- Turni: 158, 159, 160, 161

### Ring buffer: head/tail e dimensionamento
- Cos'è: un array circolare. `head` è dove scrive il produttore (la rete), `tail` è dove legge il consumatore (in pratica la callback `read_packet` di FFmpeg, non le casse: turno 200). Gli indici avanzano modulo la dimensione. Con `head == tail` non si distingue il buffer vuoto da quello pieno: serve un contatore o uno slot lasciato libero. Dimensionamento: bitrate × secondi / 8. Ad esempio 67171 bit/s × 675 s / 8 ≈ 5.6 MB (turno 162).
- Nel tuo codice: `AudioBuffer` da 15 MB (`networking/client_http.c:45-46,72-77`), allocato nell'heap con `calloc` (`:984`). Prima era `Ctx ctx;` sullo stack (turno 196): 5.6 MB solo per il buffer, un rischio di stack overflow. `enqueueAudioBuffer`/`dequeueAudioBuffer` lavorano un byte alla volta e senza controllo di pieno (`:132-143`). Il mutex viene preso **per ogni byte** (`:578-582`). `read_packet` aspetta su una condition variable quando il buffer è vuoto (`:333-364`). La rete si ferma quando ci sono più di `THRESHOLD` = 50000 byte (circa 6 s, `:48,774-783`).
- Decisioni / mismatch: il buffer è pensato per 11 minuti di time-shift, ma la logica attuale (soglia di 50 KB + al massimo 4 segmenti per giro) ne usa al massimo circa 300 KB. Il 98% dei 15 MB resta inutilizzato. Il trabocco `head` > `tail` oggi non succede solo grazie a questa soglia.
- Turni: 138, 146, 162, 163, 196, 200

### Time-shifting e "Vai alla diretta"
- Cos'è: se la rete cade puoi scegliere tra **recuperare** i segmenti persi (resti indietro rispetto alla diretta, ma non perdi nulla) e **saltare** all'ultimo segmento (resti in diretta, ma con un buco). La finestra della playlist (circa 11 min) è il limite massimo del recupero: oltre, i segmenti restituiscono 404. Il bottone "Vai alla diretta" svuota il buffer (flush), riscarica la playlist e si aggancia al live edge. Saltare a livello di segmento è sicuro: si atterra su un byte `0x47` pulito.
- Nel tuo codice: il cap a 4 segmenti (`networking/client_http.c:627-631`) è di fatto una politica di **salto verso la diretta**. Se perdi più di 4 segmenti, quelli intermedi vengono saltati. `chunk_offsets` e `current_playing_chunk` (turni 201-202) non sono ancora implementati.
- Errori/lezione: "Torna indietro di un chunk" con `tail = chunk_offsets[prev]` funziona solo se il produttore non ha già sovrascritto la zona prima di `tail`, che in un ring buffer è spazio libero. Gemini non lo segnala. Spostare `tail` va fatto sotto mutex.
- Turni: 147, 148, 162, 163, 201, 202

### HLS vs Shoutcast/ICY
- Cos'è: Shoutcast/Icecast trasmette un unico flusso HTTP infinito (risposta `HTTP/1.0 200`/`ICY 200`) con i metadati "in-band" ogni `icy-metaint` byte. È più semplice da leggere, ma non ha una finestra DVR: dopo una disconnessione riparti dalla diretta.
- Dati reali (verificati): `http://shoutcast.radio24.it:8000/` → `HTTP/1.0 200 OK`, `content-type: audio/aacp`, `icy-br:48`, `icy-sr:32000`, `icy-metaint:16384`.
- Turni: 275, 276, 277

## Possibili errori o imprecisioni di Gemini
- **Turno 142**: con `ai_socktype=0` e il servizio "http", la comparsa di `SOCK_DGRAM` "significa che il servizio supporta UDP". No: `getaddrinfo` legge `/etc/services`, dove http è registrato sia per tcp sia per udp. Non interroga il server.
- **Turno 143**: il passaggio al secondo IP "nel giro di qualche microsecondo". Vale solo se il server risponde con RST. Se l'host non risponde, `connect()` resta bloccato fino al timeout del kernel (decine di secondi; nel test NIST IPv6 oltre 10 s). Serve un connect non bloccante con timeout.
- **Turno 144**: "quando la connessione salta, `recv()` restituisce ≤ 0". In metro non arrivano né FIN né RST: `recv`/`SSL_read` resta bloccata a tempo indeterminato senza `SO_RCVTIMEO`/`poll`/keepalive.
- **Turno 145**: dettagli su VLC probabilmente inventati. Secondo Gemini "mantiene 3 s di PCM decodificato" e "access_output" sarebbe un modulo di input. Il `network-caching` di VLC (default 1000 ms) bufferizza dati **compressi**, e `access_output` è per lo streaming in uscita.
- **Turno 150**: chunked descritto come "formato proprietario": è standard HTTP/1.1. Inoltre "senza Content-Length resti bloccato per sempre su SSL_read": con `Connection: close` la fine del body è la chiusura della connessione.
- **Turno 152**: la FSM "senza mai tornare indietro" in realtà legge `buffer[i+1]`/`buffer[i+2]` (lookahead oltre `length`). Fuori dai limiti e non funziona se l'header arriva a pezzi.
- **Turno 153**: playlist d'esempio inventata (`playlist-64000_184502.ts`, `TARGETDURATION:6`). Quella reale (turno 154) è diversa.
- **Turno 154**: per contare i nuovi chunk propone di confrontare il nuovo `MEDIA-SEQUENCE` con `last_sequence`. Ma `MEDIA-SEQUENCE` è il numero del **primo** segmento: i nuovi sono quelli con numero > dell'ultimo scaricato, cioè `media_seq + n_segmenti - 1`. Il tuo codice fa correttamente il confronto con l'ultimo (`client_http.c:616-627`). C'è anche un refuso "fino all'ultimo (6406874)".
- **Turno 156**: la libreria si chiama `libavutil`, non "libutil".
- **Turni 159-160**: master playlist con 32k/64k/128k inventata e "hai scoperto che Radio 24 supporta ABR" prima di vederla. Al turno 161 si scopre che c'è una sola variante.
- **Turno 161**: "i 3171 bit in più sono overhead MPEG-TS e HTTP". BANDWIDTH non include HTTP. Il bitrate misurato dei segmenti è circa 72 kbps, più alto anche di 67171.
- **Turno 162**: "99 chunk, corretto al 100%". I segmenti sono **100** (06418776…06418875), verificato anche oggi. Il tuo codice usa giustamente 100 (`client_http.c:615`), mentre Gemini al turno 196 usava `url_count == 99`.
- **Turno 163**: "il secondo buffer PCM è tassativo". Con `SDL_AudioStream` SDL3 fa già da coda (`SDL_PutAudioStreamData` accetta qualsiasi quantità), infatti il tuo codice non ha un secondo ring buffer. "Race condition → crash inevitabile" è esagerato: è comportamento indefinito, non sempre un crash.
- **Turno 165**: il body `{"status":"ok","buffer":100}` è di **28** byte, non 27 (verificato). Inoltre il body POST non è "nascosto in modo sicuro" senza TLS.
- **Turno 166**: "Akamai senza User-Agent risponde 403/400". Falso: verificato 200 OK senza UA (curl e nc, porta 443 e 80).
- **Turno 170**: il codice d'esempio usa `strstr` su `header_accumulator`, che non è terminato da `\0` (comportamento indefinito). Se l'header supera 8 KB scarta i dati in silenzio. Slowloris non è "header infiniti che esauriscono la RAM", ma molte connessioni tenute aperte inviando header lentissimi.
- **Turno 172**: il "bug 2" (riga vuota trattata come chiave) nel tuo codice non c'era, perché `HEADER_CRLF` già intercettava `\r\n\r`. Gli altri 3 bug segnalati erano reali.
- **Turno 174**: "httpbin.org reindirizza a HTTPS". Falso: `http://httpbin.org/stream/3` risponde 200 chunked sulla porta 80 (verificato). Gemini peraltro lo propone subito dopo come "Opzione 1".
- **Turno 175**: definisce "asincrono" un socket che è bloccante.
- **Turno 185**: "il client cifra un segreto con la chiave pubblica del server". È lo scambio RSA di TLS ≤ 1.2, rimosso in TLS 1.3 (Akamai usa TLS 1.3 con (EC)DHE). "Se SSL_connect restituisce 1 il tunnel è sicuro": senza `SSL_VERIFY_PEER` no (vedi sopra).
- **Turno 187**: `SSL_CTX_set_default_verify_paths` "per verificare che Akamai sia chi dice di essere". Da sola non verifica nulla. Mancano anche SNI e controllo dell'hostname. Il tuo codice ha ereditato il problema.
- **Turni 189-190**: il pacchetto `openssl-doc` non esiste, e le man page non sono incluse in `libssl-dev`. Corretto solo al turno 191 (`libssl-doc`).
- **Turno 193**: "funzionante sotto TLS 1.3" detto senza prove (vero per caso, verificato oggi). Sub-playlist d'esempio di nuovo inventata (`media_12345.ts`, 10 s).
- **Turno 196**: non segnala che `Ctx ctx;` contiene un buffer da 5.6 MB sullo stack. Poi lo hai spostato tu nell'heap con `calloc`.
- **Turno 200**: "read_packet restituisce 0 quando il buffer è vuoto". Con le FFmpeg recenti 0 non è un valore valido (si usa `AVERROR_EOF` oppure si blocca). Il tuo codice giustamente si blocca su una condition variable.
- **Turni 201-202**: `tail >= chunk_offsets[...]` ignora il wrap-around del ring buffer. Il salto indietro ignora che la zona prima di `tail` può essere sovrascritta. Gli indici vengono spostati senza mutex. "Torna alla diretta" = `chunk_offsets[98]` non è la diretta, ma l'ultimo segmento scaricato.
- **Turno 203**: "libsdl3-dev c'è da Kubuntu 24.10". SDL 3.2 (la prima stabile) è uscita a gennaio 2025, dopo la 24.10 (ottobre 2024): il pacchetto c'è solo nelle release successive.
- **Turno 155** (dubbio): "Chrome 142+ ha un demuxer HLS nativo su desktop" e "MSE è un ring buffer". La prima è da verificare (vedi domande aperte). MSE è un'API JS, non un ring buffer.

## Domande aperte / cose non chiarite
- Failover a metà download: dove intercettare l'errore in `fetch` (oggi `exit(1)`) e come riprovare lo stesso segmento su un altro IP di `servinfo`?
- Timeout: `SO_RCVTIMEO` sul socket sotto OpenSSL oppure socket non bloccante + `poll` + `SSL_ERROR_WANT_READ`? (Collegato all'actor model in `networking/actor/`, area successiva.)
- Keep-alive: con `Connection: keep-alive` sullo stesso socket TLS si risparmia un handshake per segmento. Cosa cambia nel parser (fine body solo da Content-Length o dal chunk 0)?
- Politica dopo una lunga disconnessione: oggi si saltano i segmenti oltre il quarto. È davvero quello che vuoi (time-shift contro diretta, turni 147-148)?
- Gestire 3xx/`Location` e 404 sui segmenti scaduti invece di terminare il programma.
- Il `BANDWIDTH=67171` dichiarato è minore del bitrate reale dei segmenti (circa 72 kbps): come si calcola esattamente?
- Il manifest dice `mp4a.40.5` (HE-AAC v1), ffprobe vede HE-AACv2: va bene così (compatibilità all'indietro)?
- Chrome riproduce davvero gli `.m3u8` nativamente sul desktop (turno 155)? Da provare in Chrome.

## Argomenti a margine (non legati a progetti: titolo + turni + 1 riga)
- Protocolli testuali vs binari, FSM per MQTT (fixed header, remaining length, payload) — turno 176 — con `recv`/`send` sai già scrivere qualsiasi protocollo.
- Server asincrono single-threaded alla Redis (kqueue/epoll, O_NONBLOCK) e pub/sub — turno 177 — il passo successivo dopo il player.
- IoT e connessione intermittente: code locali, sessioni persistenti nel broker, QoS 0/1/2 — turno 178.
- Radix tree vs trie per i topic MQTT — turno 180 — nodi compressi, meno puntatori; Redis usa `rax.c`.
- B-tree (disco) vs le strutture in RAM di Redis (dict, skiplist, listpack, rax) — turno 181.
- Redis Streams vs Kafka, log append-only e consumer group — turno 182.
- WhatsApp Web e recupero dei messaggi offline (Erlang, log per dispositivo) — turno 183.
- Schema di code offline per un futuro broker MQTT in C — turno 184.
- Libri: Effective C, Expert C Programming, UNP Vol. 1 (Stevens), Attacking Network Protocols, Hacking: The Art of Exploitation — turno 195 — UNP ha dato origine a `daytime_client/`.

## Domande di autoverifica
1. Perché `getaddrinfo` restituisce una lista e non un solo indirizzo? Cosa fanno `AF_UNSPEC` e `SOCK_STREAM` negli hints?
2. Cosa restituisce `recv` quando il server chiude la connessione? E in caso di errore? Perché `recv` va sempre messa in un ciclo?
3. Scrivi a memoria una richiesta GET HTTP/1.1 minima e corretta. Quale header è obbligatorio e perché?
4. Perché il tuo echo client restava appeso con tcpbin.com finché non hai aggiunto `\n`?
5. Nel daytime server ogni byte è una `write` separata, eppure il client fa 1-2 `read`. Cosa dimostra questo su TCP?
6. Decodifica a mano il body chunked `5\r\nWi\rki\r\n5\r\npedia\r\n0\r\n\r\n`. Perché il terminale mostra `ki`?
7. Cosa fa `getNextByte` e perché permette al parser di funzionare con `MAXDATASIZE 1`?
8. Come distingue il tuo parser la fine degli header dall'inizio del body? Quale bug c'era al turno 172 su quel `\n`?
9. Perché assegnare il valore di ritorno di `SSL_read` a un `size_t` è un bug?
10. Qual è il ruolo di `SSL_CTX`, `SSL`, `SSL_set_fd` e `SSL_connect`? Cosa manca nel tuo `initSSL` per rifiutare un certificato scaduto o di un altro host? Cos'è la SNI?
11. Master playlist, media playlist e segmento `.ts`: cosa contiene ciascuno nel caso di Radio 24? Cosa indica `#EXT-X-MEDIA-SEQUENCE`?
12. Un segmento di 6.82667 s a 67171 bit/s: quanti byte ti aspetti? Perché il server ne manda 61288 e perché 61288/188 è un numero intero?
13. In un ring buffer con solo `head` e `tail`, come distingui "vuoto" da "pieno"? Chi muove `tail` nel tuo player?
14. Il tuo `MAXAUDIOBUFFER` è 15 MB, ma quanta parte ne usa davvero la logica `THRESHOLD` + "max 4 chunk"? Cosa cambieresti per avere 11 minuti di time-shift?
15. In metro il segnale sparisce senza FIN né RST: cosa succede oggi al thread di rete del tuo player e come lo renderesti robusto (timeout, failover, retry dello stesso segmento)?
