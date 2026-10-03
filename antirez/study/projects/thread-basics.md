# Thread basics: mutex e condition variable (esempi LLNL) — stato: done
Path: `networking/mutex_example.c`, `networking/condition_variables.c` · Ultima modifica: 2026-07-05 (git)

## Cosa fa
Sono i due esempi classici del tutorial "LLNL POSIX Threads Programming", consigliato al turno 113 e ricopiati per studio prima di rendere multithread il player (turni 227-229).
- `mutex_example.c`: prodotto scalare di due vettori (4 thread × 100 elementi). Ogni thread calcola una somma parziale locale e la aggiunge a `dotstr.sum` sotto mutex (`:67-69`). Il thread riceve il proprio offset castato in `void *` (`:116`, `:45`). Il main fa join e stampa `Sum = 400`.
- `condition_variables.c`: due thread `inc_count` incrementano `count` 10 volte ciascuno (con `sleep(1)`), sotto mutex. Quando `count == 12` fanno `pthread_cond_signal` (`:33-36`). Il thread `watch_count` aspetta con `while (count < COUNT_LIMIT) pthread_cond_wait(...)` (`:60-67`), poi aggiunge 125.

È lo stesso schema che hai usato nel player: `audio_buffer_threshold_cv` imita `count_threshold_cv` (`networking/client_http.c:127-130`).

## Come si compila / si esegue
```sh
B=<scratchpad>/build/concorrenza
cc -W -Wall -g networking/mutex_example.c -o $B/mutex_example          # 0 warning
cc -W -Wall -g networking/condition_variables.c -o $B/condvar           # 0 warning
$B/mutex_example
$B/condvar                                                               # ~10 s (sleep)
cc -g -fsanitize=thread networking/mutex_example.c -o $B/mutex_tsan
```

## Esito verifica
- `mutex_example`: stampa `a: 1.000000` e `Sum = 400.000000`, exit 0. `leaks --atExit`: 0 leak.
- `condvar`: watch_count va in wait con count=0. Al conteggio 12 compare "threshold reached. Just sent signal.", il watcher si sveglia e porta count a 137, poi gli inc arrivano a 145. Stampa `Final value of count = 145. Done.` in 10,2 s. 0 leak.
- **TSan**:
  - I binari originali **si bloccano** sotto TSan su macOS (processo in sleep, nemmeno `alarm` lo uccide) perché il `main` termina con `pthread_exit(NULL)` (`mutex_example.c:131`, `condition_variables.c:105`).
  - Con `return 0` (copie in scratchpad) entrambi finiscono senza nessun warning.
  - Controprova: togliendo lock e unlock da `dotprod`, TSan riporta `WARNING: ThreadSanitizer: data race ... Write of size 8 ... dotprod mutex_race.c:68`, anche se la somma stampata resta 400.

## Cosa funziona
- Entrambi corretti e senza race.
- `watch_count` usa il `while` attorno a `pthread_cond_wait`, così copre sia i risvegli spuri sia il caso "soglia già raggiunta prima di aspettare".
- Il pattern "lavoro locale + sezione critica minima" in `dotprod` è quello giusto.

## Cosa manca / bug noti
- Non sono bug, ma da sapere:
  - `pthread_exit(NULL)` nel main (`mutex_example.c:131`, `condition_variables.c:105`) è legale ma rompe TSan su macOS. Usare `return 0`.
  - Il valore di ritorno di `pthread_create`/`pthread_mutex_init` non è controllato.
  - `inc_count` segnala solo quando `count == COUNT_LIMIT` esatto. Se il watcher partisse dopo, lo salverebbe solo il `while`.
- Mancano esercizi tuoi derivati, ad esempio:
  - versione con `pthread_cond_broadcast` e più watcher;
  - produttore/consumatore su un ring buffer piccolo, con due condition variable (non vuoto / non pieno).

## Concetti applicati
- [Mutex](../notes/concorrenza.md#mutex)
- [Condition variable](../notes/concorrenza.md#condition-variable)
- [Race condition](../notes/concorrenza.md#race-condition-e-memoria-condivisa)
- [Thread POSIX: create, join e contesto](../notes/concorrenza.md#thread-posix-create-join-e-passaggio-del-contesto)

## Prossimo passo consigliato
1. Scrivere un mini produttore/consumatore (ring buffer da 8 int, `not_empty`/`not_full`), senza rete e senza SDL, e verificarlo con TSan (`return 0` nel main).
2. Portare in `client_http.c` quello che impari:
   - `while` invece di `if` in `decode()` (`:815`);
   - lock una volta per blocco invece che per byte (`:579-581`);
   - signal dopo ogni blocco scritto e non solo a fine `fetch` (`:799-801`).
