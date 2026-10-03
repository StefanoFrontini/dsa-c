# Esercizio poll() (esempio della man page)  — stato: working
Path: networking/actor/poll_input.c (78 righe) · Ultima modifica: 2026-07-15 (git; mtime 16/07)

## Cosa fa
È l'esempio della man page di `poll(2)` ricopiato (turno 264): apre i file passati in argomento, li mette in un array
`struct pollfd` con `events = POLLIN`, chiama `poll(…, -1)` finché almeno un fd è aperto, stampa i flag di `revents`
(`POLLIN`/`POLLHUP`/`POLLERR`, con la bitmask del turno 266), legge fino a 10 byte se c'è `POLLIN`, altrimenti chiude
l'fd. Nota: nonostante il nome, **non** legge la tastiera in modo non canonico: quella parte è stata scritta
direttamente in `client_http_actor.c:1313-1330`.

## Come si compila / si esegue
```
cc -W -Wall -g networking/actor/poll_input.c -o $B/poll_input        # 0 warning
mkfifo f1 f2; $B/poll_input f1 f2      # in un altro terminale: echo aaa > f1 ; echo bbb > f2
```
(Il test l'ho fatto sotto `script -q /dev/null` per avere stdout a righe e timeout con `perl -e 'alarm N'`.)

## Esito verifica
- Senza argomenti: `Usage: ./poll_input file...` → exit 1. OK.
- Due FIFO: `Opened "fifo1" on fd 3`, `Ready: 1`, `fd=3; events: POLLIN`, `read 4 bytes: aaa`; un messaggio di 16
  byte su fifo2 arriva in due letture (10 + 6) perché `buf` è di 10 byte. OK.
- Chiusura dello scrittore di una FIFO su **macOS**: nessun `POLLHUP` osservato, `poll` è rimasto bloccato fino al
  timeout (4 s). Su Linux l'esempio si basa proprio su `POLLHUP`.
- File regolare (`f.txt`): `poll` dice sempre `POLLIN`, a fine file `read` ritorna 0 ma l'fd non viene chiuso →
  ciclo infinito (3,1 milioni di righe in 3 s, ucciso dal timeout).
- `/dev/tty` come argomento: `revents` = 0x20 = `POLLNVAL`, nessun flag stampato, fd chiuso subito. È il limite
  documentato da macOS ("poll() does not support devices", sezione BUGS della man page). Su stdin (pty) invece
  funziona.
- Leaks: non significativo (esce con `exit`; `pfds` non viene liberato prima di uscire).

## Cosa funziona
- Uso corretto di `poll` con timeout -1 e scansione di `revents` (`:45-74`); bitmask a `:57-63`.
- Lettura con `%.*s` per stampare byte non terminati da `\0` (`:66`).

## Cosa manca / bug noti
- `read` che ritorna 0 (EOF) non è gestito (`:64-66`): andrebbe trattato come chiusura (`close` + `num_open_fds--`).
- `char *argv[1]` (`:14`): compila (decade a `char **`) ma è fuorviante; la man page usa `char *argv[]`.
- `pfds` non viene liberato (`:76-77`, irrilevante perché si esce).
- `POLLNVAL` non è stampato tra i flag (`:58-61`): su macOS è il caso dei device.

## Concetti applicati
Sezioni di `notes/actor-model.md`: "poll() e l'event loop", "Bitmask di revents", "poll vs epoll/kqueue/IOCP".

## Prossimo passo consigliato
1. Gestire `s == 0` come EOF e stampare anche `POLLNVAL`.
2. Aggiungere `STDIN_FILENO` all'array e mettere il terminale in modo non canonico: è la palestra minima per il loop
   del player (tasto → evento) senza rete e senza FFmpeg.
3. Aggiungere un timeout dinamico (es. spinner a 100 ms) per vedere dal vivo la differenza tra -1, 0 e N ms.
