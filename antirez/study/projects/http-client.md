# Client HTTP in chiaro (tappe: client_http_2.c → client_http_3.c → versione mock)  — stato: done
Path: networking/client_http_2.c, networking/client_http_3.c, networking/mock_server.py, networking/output.txt · Ultima modifica: client_http_3.c 2026-06-18, client_http_2.c 2026-06-10, mock_server.py 2026-06-23 (git)

> `networking/client_http.c` oggi è il player completo (Makefile → `underground_radio`): l'analisi è in **projects/audio-player.md**. Qui ci sono solo le tappe del client HTTP. La parte di rete di client_http.c (TLS, HLS, ring buffer) è spiegata nei concetti di notes/networking.md.

## Cosa fa
"Progetto 1" della roadmap del turno 164: un client HTTP/1.1 in chiaro su `example.com:80` che separa header e body con una macchina a stati. Le tappe:
1. **Primi commit di client_http.c (06-07/06-10)**: GET con `Host`/`User-Agent: UndergroundRadio/1.0`/`Connection: close`. Prima una sola `recv` (turno 167), poi un ciclo `recv` con `MAXDATASIZE 10` (turno 169).
2. **`client_http_2.c` (06-10)**: primo tentativo di parsing mentre i dati arrivano, con `header_accumulator` + `checkCapacity` (turno 170). **Abbandonato e non compila**.
3. **`client_http_3.c` (commit client_http_4…16, 06-11/06-18)**: FSM dell'header con `MAXDATASIZE 1`. È nata con gli stati `_ACC` duplicati, l'"spaghetti code" del turno 171. È stata poi riscritta con il **byte stream iterator** `getNextByte` (`networking/client_http_3.c:227-238`), come al turno 172. Si ferma a `HEADER_DONE`: non legge il body.
4. **Versione mock (commit `0de4502` "client_http_19", 06-23)**: aggiunge il body chunked (`BODY_CHUNKED_SIZE`/`BODY_CHUNKED_HTML`, turno 173) e punta a `127.0.0.1:8080`. `mock_server.py` (turno 174) invia `Wi\rki` + `pedia` con pause, e `output.txt` contiene l'output atteso (turno 175).
5. Poi (commit `4f24907`, 06-23) arrivano OpenSSL e Akamai, e il file diventa il player → projects/audio-player.md.

## Come si compila / si esegue   (comandi verificati da te)
```sh
cc -W -Wall -g networking/client_http_3.c -o $B/client_http_3 && $B/client_http_3        # example.com:80
cc -W -Wall -g networking/client_http_2.c -o $B/client_http_2                           # fallisce
git show 0de4502:./networking/client_http.c > $B/client_http_0de4502.c
cc -W -Wall -g $B/client_http_0de4502.c -o $B/client_http_0de4502
python3 networking/mock_server.py &        # porta 8080
$B/client_http_0de4502
```

## Esito verifica
- `client_http_3.c`: compila **senza warning**. Contro example.com (IPv6, Cloudflare) stampa `Status Code Rilevato: 200` e i 13 header (`Transfer-Encoding: chunked` → `Encoding Chunked: true`), poi `--- parsing HEADER completato con successo ---`. `leaks`: 0.
- `client_http_2.c`: **non compila**. `error: redefinition of 'start' with a different type: 'int *' vs 'char *'` (riga 213), più 4 warning (enum mescolati alla riga 110, `parseStatusLine` vuota alle righe 235-236, `num_cr` inutilizzato, switch incompleto).
- Versione mock `0de4502` + `mock_server.py`: output **identico a `output.txt`** (`\r` escluso). `cat -v` mostra `Wi^Mki`: il byte `\r` c'è davvero e il terminale mostrava `ki` (turno 175). Stesso output con `MAXDATASIZE` 3, 1000 e 8192. `leaks`: 0.
- Mock aggressivo (`mock2.py` nel build dir, invio byte per byte), sulla versione 0de4502:

  | caso | esito |
  |---|---|
  | `5;foo=bar` + `A` esadecimale + trailer | OK (strtol si ferma al `;`) |
  | `Transfer-Encoding:chunked` senza spazio | OK |
  | Content-Length + chunked insieme | usa chunked (corretto per RFC 9112) |
  | 404 | `HEADER_ERROR`, exit 1 |
  | righe terminate solo da `\n` | rifiutate |
  | server che chiude a metà chunk | `BODY_ERROR` (rilevato) |
  | solo Content-Length | body non letto (stato non ancora presente) |

## Cosa funziona
- Header FSM un byte alla volta, indipendente dalla dimensione di `recv_buf` (anche 1 byte).
- Fine header riconosciuta su riga vuota consumando sia `\r` sia `\n`, così il body inizia esattamente al byte giusto.
- Body chunked con consumo del `\r\n` dopo ogni chunk e controllo di overflow su `size_buffer` (versione 0de4502).
- Testato in locale in modo ripetibile con il mock server.

## Cosa manca / bug noti
- `networking/client_http_3.c:56` + `:170`, `:189`, `:206`: `size_t numbytes` e controllo `== 0`. Il -1 di `recv` diventa un numero enorme: è il bug del turno 172, corretto solo nelle versioni successive con `int` e `<= 0`.
- `client_http_3.c`: restano codice morto della versione `_ACC` (gli stati `*_ACC` dell'enum, `accumulate`, `keepAccumulating`, `copyFromAccToRecvBuf`, righe 160-225) e non c'è parsing del body.
- Tutte le tappe: uno status >= 400 chiude il programma. I 3xx non sono gestiti. Viene saltato un solo spazio dopo `:`. `Transfer-Encoding` è confrontato esattamente con "chunked". I trailer non vengono letti. Le risposte con solo LF vengono rifiutate. Host, porta e path sono `#define` (niente URL da riga di comando).
- `client_http_2.c`: ramo morto, non compila. Si può archiviare o cancellare.

## Concetti applicati
- notes/networking.md#struttura-di-una-richiesta-http11-e-header
- notes/networking.md#recv-bloccante-e-lettura-a-pezzi
- notes/networking.md#parser-dellheader-come-macchina-a-stati-che-consuma-byte-byte-stream-iterator
- notes/networking.md#content-length-vs-transfer-encoding-chunked
- notes/networking.md#test-locale-con-mock-server

## Prossimo passo consigliato
1. Fai di `client_http_3.c` (o della versione 0de4502) un piccolo tool `httpget host port path` con argomenti da riga di comando e `numbytes` di tipo `int`.
2. Estendi `mock_server.py` con i casi di `mock2.py` (estensioni, trailer, troncamento, LF) e trasformali in test automatici che confrontano l'output con file attesi.
3. Aggiungi la gestione dei 3xx (`Location`) e il trim di spazi e tab sui valori degli header.
