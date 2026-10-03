# Echo client (+ showip `old.c`, `telnot.c`)  — stato: done
Path: networking/echo_client.c, networking/old.c, networking/telnot.c · Ultima modifica: 2026-06-07 (git)

## Cosa fa
Primi passi con la guida di Beej (turni 142-144, 164):
- `old.c`: è lo "showip" di Beej. `getaddrinfo(host, "443")` stampa tutti gli IPv4/IPv6 di un host e il `ai_socktype` (turni 142-143: i due IP di Akamai).
- `echo_client.c`: risolve `argv[1]:4242`, prova gli indirizzi uno per uno finché `connect` riesce, invia `"Hello world!\n"` e stampa una sola `recv` di risposta (tcpbin.com fa l'eco).
- `telnot.c`: codice di **Beej** (non tuo), un "non-telnet" con `poll()` su stdin e socket. Te lo sei tenuto come riferimento, ed è l'antenato di `actor/poll_input.c`.

## Come si compila / si esegue   (comandi verificati da te)
```sh
cc -W -Wall -g networking/echo_client.c -o $B/echo_client && $B/echo_client tcpbin.com
cc -W -Wall -g networking/old.c -o $B/old && $B/old ilsole24ore-radio.akamaized.net
cc -W -Wall -g networking/telnot.c -o $B/telnot && $B/telnot tcpbin.com 4242
```

## Esito verifica
- Build: nessun warning per tutti e tre.
- `echo_client tcpbin.com` → si connette via IPv6 `2600:3c01::f03c:91ff:feab:f98b`, `Bytes sent: 13`, `Bytes received: 13`, `Hello world!`. `leaks`: 0.
- Test di controllo, una copia **senza `\n`**: resta bloccato su `recv` finché non scatta l'alarm a 10 s. Conferma la spiegazione del turno 164 (tcpbin lavora a righe). Curiosità: con `nc` arriva comunque l'eco, perché `nc` chiude il lato di scrittura e il server riceve EOF.
- `old ilsole24ore-radio.akamaized.net` → 2 IPv6 + 2 IPv4 (oggi `23.61.204.34`, `80.67.66.18`). `leaks`: 0.
- `echo_client localhost` (porta chiusa) → prova `::1` e poi `127.0.0.1` (`Connection refused` per entrambi), `client: failed to connect`, rc=2. Host inesistente → messaggio di `gai_strerror`, rc=1.
- `telnot tcpbin.com 4242` con input `ciao telnot\n` → l'eco funziona. Con stdin `/dev/null` invece va in **loop attivo**: 2.8 s di CPU su 3 s. `read()==0` (EOF) non è gestito e `poll` continua a segnalare POLLIN.

## Cosa funziona
- Ciclo "prova il prossimo indirizzo" con `getaddrinfo` (`networking/echo_client.c:37-68`), `inet_ntop`, `freeaddrinfo`.
- Gestione degli errori di `getaddrinfo`, `socket`, `connect`, `send`, `recv`.

## Cosa manca / bug noti
- `networking/echo_client.c:87`: una sola `recv`. Un eco più lungo o spezzato arriverebbe a metà. Serve un ciclo fino a ricevere `len` byte.
- `echo_client.c:81`: `send` può inviare meno di `len` byte. Non c'è un ciclo per inviare il resto.
- `echo_client.c:70-73`: se nessun indirizzo funziona, `servinfo` non viene liberato (piccolo leak nel percorso d'errore).
- `echo_client.c:24`: il messaggio d'uso dice "server hostname".
- `old.c:38`: stampa `ai_socktype` solo per IPv4.
- `networking/telnot.c:137`: `read` che restituisce 0 (EOF su stdin o server che chiude) non è gestito → loop infinito al 100% di CPU (bug del codice di Beej, da sapere se lo riusi).

## Concetti applicati
- notes/networking.md#getaddrinfo-hints-e-la-lista-addrinfo
- notes/networking.md#più-indirizzi-ip-e-failover
- notes/networking.md#socket--connect--send--recv

## Prossimo passo consigliato
1. Scrivi `sendall()` e `recvn()` (cicli su `send`/`recv`) e usale nell'echo client.
2. In `telnot.c` gestisci `read()==0` (esci o togli il fd da `poll`). È un buon esercizio prima dell'event loop di `actor/`.
3. Aggiungi un timeout (`SO_RCVTIMEO` o `poll` con timeout) per non restare appeso come con tcpbin.
