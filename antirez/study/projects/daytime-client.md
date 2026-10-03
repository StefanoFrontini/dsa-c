# Daytime client/server (UNP cap. 1)  — stato: working
Path: networking/daytime_client/ · Ultima modifica: 2026-07-23 (git; file aggiunti il 2026-07-21)

## Cosa fa
Esercizi del capitolo 1 di *Unix Network Programming* (Stevens), il libro consigliato al turno 195. Nessun turno della conversazione ne parla.
- `daytimetcpsrv.c`: server TCP IPv4 sulla porta **9999**. A ogni connessione invia `ctime()` + `\r\n` e chiude. La scrittura è fatta **un byte per volta** (`Write`, `networking/daytime_client/daytimetcpsrv.c:36-44`): è l'esercizio 1.5 di UNP, per vedere che TCP non conserva i confini delle `write`.
- `daytimetcpcli.c`: client IPv4 verso `argv[1]:9999`. Legge in un ciclo `read` e conta le letture (`Counter is N`, `networking/daytime_client/daytimetcpcli.c:55-62`).
- `daytimetcpcli_IPv6.c`: variante IPv6 verso la porta 13 (il servizio daytime standard).
- `unp.h`: header originale di Stevens. Non viene incluso dai sorgenti, serve solo come riferimento.
- Ci sono wrapper in stile UNP (`Socket`, `Connect`, `Bind`, `Listen`, `Accept`, `Close`) con `perror`.

## Come si compila / si esegue   (comandi verificati da te)
```sh
B=<build dir>
cc -W -Wall -g networking/daytime_client/daytimetcpsrv.c -o $B/daytimetcpsrv
cc -W -Wall -g networking/daytime_client/daytimetcpcli.c -o $B/daytimetcpcli
cc -W -Wall -g networking/daytime_client/daytimetcpcli_IPv6.c -o $B/daytimetcpcli_IPv6
$B/daytimetcpsrv &            # porta 9999
$B/daytimetcpcli 127.0.0.1
```

## Esito verifica
- Build: nessun warning, per tutti e 3 i file.
- Client + server in locale (6 esecuzioni): `Sat Oct  3 12:03:36 2026` / `Counter is 1` (una volta `Counter is 2`). 26 `write` da 1 byte arrivano in 1-2 `read`. `leaks`: 0 leak.
- IPv6: verso NIST (`2610:20:6f15:15::26`, porta 13) il `connect` resta bloccato oltre 10 s (nessuna risposta, nemmeno con `nc -6`). Ricompilato con la porta 9913 contro un server Python su `::1` funziona: stampa la data.
- Senza argomenti: stampa l'uso e poi **segfault** (rc=139).
- Con un IP non valido (`1.2.3`): stampa `inet_pton error`, ma va avanti, si connette a 0.0.0.0 (cioè localhost su macOS) e stampa comunque l'ora.
- Server riavviato subito dopo (porta in TIME_WAIT): `bind error: Address already in use`, ma il server **continua**: `listen` su un socket non legato a nessuna porta lo mette in ascolto su una porta effimera casuale (`lsof`: `TCP *:55625 (LISTEN)`), e i client ricevono `Connection refused`.

## Cosa funziona
- Il ciclo `read` fino a EOF e la dimostrazione del byte stream (contatore).
- Il server iterativo `accept` → `write` → `close`.
- Il client IPv6 con `sockaddr_in6`/`inet_pton(AF_INET6)`.

## Cosa manca / bug noti
- `networking/daytime_client/daytimetcpcli.c:33-35` e `daytimetcpcli_IPv6.c:18-20`: manca `exit` dopo l'uso → segfault su `argv[1]` NULL.
- `daytimetcpcli.c:45-47`: dopo l'errore di `inet_pton` non esce.
- I wrapper fanno solo `perror` e non terminano (`daytimetcpsrv.c:16-34`, `:63-65`; `daytimetcpcli.c:14-24`). In UNP `err_sys` esce. Conseguenza: il bind fallito non viene notato, e se `Connect` fallisce si arriva a `read` su un socket non connesso.
- `daytimetcpsrv.c:73-82`: manca `setsockopt(SO_REUSEADDR)`, quindi non puoi riavviare subito il server.
- `daytimetcpsrv.c:39`: il ritorno di `write` viene ignorato e `bytes_written` conta i tentativi, non i byte scritti. Fa aritmetica su `void *` (`ptr + i`): è un'estensione GNU, con `-Wpointer-arith` dà warning.
- `bzero` è deprecato: meglio `memset`.

## Concetti applicati
- notes/networking.md#socket--connect--send--recv
- notes/networking.md#recv-bloccante-e-lettura-a-pezzi
- notes/networking.md#più-indirizzi-ip-e-failover (connect bloccante senza timeout)

## Prossimo passo consigliato
1. Fai uscire i wrapper con `exit(1)` (come `err_sys`) e aggiungi `exit` dopo l'uso e dopo `inet_pton`.
2. Aggiungi `SO_REUSEADDR` al server e prova a riavviarlo subito.
3. Rendi il client indipendente dalla famiglia con `getaddrinfo` (UNP cap. 11, `tcp_connect`) e unisci le due versioni IPv4/IPv6.
