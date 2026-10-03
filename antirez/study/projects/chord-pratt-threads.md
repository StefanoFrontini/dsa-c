# Chord Pratt multithread (MapReduce su 2 pthread) — stato: working
Path: `chord-pratt/chord-pratt-threads.c` (+ `run_test.sh`, `benchmarks.csv`, `old_benchmarks.csv`, `stress_test*.txt`) · Ultima modifica: 2026-05-27 (git)

## Cosa fa
È il parser Pratt della chord sheet (`[A]aaa[D]...`, vedi `chord-pratt.md`) con due modalità:
- **single**: lexer → parse → `evalAST` iterativo su tutto il buffer.
- **`--multi`**: il buffer viene tagliato al primo `\n` dopo la metà, due pthread parsano le metà in parallelo, il main valuta i due AST e travasa le LINE della seconda SONG nella prima (reduce), con `retain` per ogni riga.

Entrambe le modalità stampano `Execution Time: X seconds` (wall-clock, `CLOCK_MONOTONIC`). La stampa XML è commentata. `run_test.sh` lancia 50 run per modalità e scrive media e deviazione standard in `benchmarks.csv`.

## Come si compila / si esegue
```sh
B=<scratchpad>/build/concorrenza
cc -W -Wall -g chord-pratt/chord-pratt-threads.c -o $B/cpt      # 1 warning: unused 'result' (riga 903)
cc -W -Wall -O2 chord-pratt/chord-pratt-threads.c -o $B/cpt_O2
$B/cpt chord-pratt/stress_test2.txt            # single
$B/cpt chord-pratt/stress_test2.txt --multi    # 2 thread
cc -g -O1 -fsanitize=thread  chord-pratt/chord-pratt-threads.c -o $B/cpt_tsan
cc -g -O1 -fsanitize=address,undefined chord-pratt/chord-pratt-threads.c -o $B/cpt_asan
leaks --atExit -- $B/cpt chord-pratt/stress_test4.txt --multi
```
Su macOS non serve `-lpthread`. `run_test.sh` usa `./a.out` nella cartella. Per il benchmark ne ho usata una copia in scratchpad, con il path del binario e del CSV passati per variabile e `ITERATIONS=10`.

## Esito verifica
- **Test di base**: test.txt, test2.txt e stress_test (50k), single e multi danno exit 0.
  - stress_test 50k: 0,158 s single, 0,111 s multi (-O0).
- **200k e 600k**: girano in entrambe le modalità (lo stack overflow dei turni 128-135 è risolto). Ho verificato anche il contrario: rimettendo la `freeObject` ricorsiva commentata, 200k single fa exit 139.
- **Correttezza dell'XML**: ho decommentato `printXML` in una copia e confrontato le due modalità con `diff`. Il multi ha **una `<word>` vuota in più** (empty_chord/empty_lyric) alla fine dell'ultima riga della prima metà. Succede sia su test.txt che su test2.txt.
- **Edge case**: file di una riga, file vuoto e file la cui ultima riga è più lunga di metà file vanno in **segfault con `--multi`** (exit 139). lldb mostra `EXC_BAD_ACCESS address=0x0` in `advanceLexer` (`chord-pratt-threads.c:312`) nel thread 2.
- **ThreadSanitizer**: nessuna data race (test2, stress_test `--multi`).
- **ASan/UBSan**: nessun errore. Sotto ASan il multi è molto più lento del single (24 s contro 0,8 s su 100k): il reduce fa una `realloc` +1 per ogni riga, e ASan copia sempre. Con l'allocatore normale il reduce costa circa il 2%.
- **leaks**: 0 leak su test2 (single e multi), 100k multi, 200k single e 600k multi.
- **Memoria**: picco di circa 470 MB per stress_test4 (11 MB). Il tempo totale del processo a 600k è 2,56 s contro 1,09 s misurati: la `release` finale non è nel timer e costa più del parsing.
- **Benchmark** (i7-6700K Intel, 4 core/8 thread, 10 run):

  | build | file | single (s) | multi (s) | speedup |
  |---|---|---|---|---|
  | -O0 | 100k | 0,312 ± 0,010 | 0,221 ± 0,007 | 1,41 |
  | -O0 | 600k | 2,02 ± 0,31 | 1,37 ± 0,05 | 1,47 |
  | -O2 | 100k | 0,282 ± 0,014 | 0,171 ± 0,010 | 1,65 |
  | -O2 | 600k | 1,77 ± 0,18 | 1,07 ± 0,22 | 1,66 |

  Confronto con i CSV dell'M1:

  | file | single (s) | multi (s) | speedup |
  |---|---|---|---|
  | `old_benchmarks.csv` (100k) | 0,132 | 0,086 | 1,55 |
  | `benchmarks.csv` (600k) | 0,844 | 0,565 | 1,49 |

  Valori assoluti circa 2,3× più alti su Intel, speedup nello stesso intervallo.
- **Fasi** (-O0, 100k, multi):

  | fase | tempo (s) |
  |---|---|
  | split | 0,0005 |
  | thread (parse) | 0,256 |
  | `evalAST` seriale | 0,060 |
  | reduce | 0,006 |

  Spostare `evalAST` nei thread fa guadagnare circa il 5%.
- **`clock_getres(CLOCK_MONOTONIC)`** = 1 µs su questo Mac: su test.txt (~10 µs) la misura non ha senso.

## Cosa funziona
- Parallelismo senza lock: contesti, buffer e nodi separati per thread; TSan pulito.
- Merge e ownership corretti: `retain(line)` nel reduce, `release` di entrambi gli AST, 0 leak.
- `evalAST` (stack esplicito pre-allocato) e `freeObject` (loop sulla spina sinistra) iterativi: nessuno stack overflow fino a 600k righe.
- Timer monotono e scelta della modalità a runtime (`--multi`). Script statistico parametrico con funzione bash.

## Cosa manca / bug noti
- `chord-pratt-threads.c:777-794`, `847`: se dopo la metà non c'è nessun `\n`, `bufs.buf2` resta NULL e `initContext(&ctx2, NULL)` porta a un segfault. Serve un fallback: niente split, oppure buf2 = stringa vuota.
- `chord-pratt-threads.c:789` + lexer `:330-333`: il `\0` messo al posto del `\n` fa iniettare al thread 1 la WORD vuota di fine file, quindi l'output multi è diverso da quello single.
- `chord-pratt-threads.c:858-859`: `evalAST` delle due metà gira nel main dopo il join (parte seriale evitabile). Il risultato potrebbe essere calcolato in `parse` e ritornato con `return`/`pthread_join(t, &ret)`.
- `chord-pratt-threads.c:862-866`: il reduce usa `listPushObj` con `realloc` +1 per ogni riga. Meglio un'unica `xrealloc` a `len1+len2`.
- `chord-pratt-threads.c:851-852`, `828`: i valori di ritorno di `pthread_create` e di `fread` non sono controllati. `pthread_exit(NULL)` (`:804`) si può sostituire con `return NULL`.
- `chord-pratt-threads.c:903`: variabile `result` inutilizzata (warning). Il `printXML` ricorsivo, se riattivato, ha ancora il limite di profondità.
- `run_test.sh:26`: usa `./a.out`, ma l'`a.out` presente in `chord-pratt/` è del 7 maggio (non conosce `--multi` né "Execution Time"). Inoltre:
  - non controlla l'exit code del binario, quindi i crash diventano errori di `bc`;
  - non scarta il warm-up;
  - la deviazione standard è di popolazione.
- Split fisso a 2 thread: non scala a N.

## Concetti applicati
- [Thread POSIX: create, join e contesto](../notes/concorrenza.md#thread-posix-create-join-e-passaggio-del-contesto)
- [Split su confine di riga](../notes/concorrenza.md#split-dellinput-su-un-confine-di-riga)
- [MapReduce grazie al monoide](../notes/concorrenza.md#mapreduce-grazie-al-monoide)
- [Ownership dopo il merge](../notes/concorrenza.md#ownership-dopo-il-merge-di-due-ast)
- [Race condition](../notes/concorrenza.md#race-condition-e-memoria-condivisa)
- [Misurare le prestazioni](../notes/concorrenza.md#misurare-le-prestazioni-tempo-monotono-wall-clock-e-cpu-time)
- [Bash + bc](../notes/concorrenza.md#script-bash--bc-media-e-deviazione-standard)
- [Risultati benchmark](../notes/concorrenza.md#risultati-dei-benchmark)
- [Amdahl](../notes/concorrenza.md#legge-di-amdahl-e-algoritmi-parallelizzabili)
- [Bug 200k righe](../notes/concorrenza.md#il-bug-delle-200000-righe-stack-overflow)
- [mmap](../notes/concorrenza.md#mmap-per-file-grandi)

## Prossimo passo consigliato
1. Correggere `split`: se non trova `\n`, niente thread (oppure buf2 = `""`). Poi aggiungere un test che confronti l'XML single e multi con `diff`: deve risultare identico. Questo obbliga a risolvere anche la WORD vuota in più.
2. Spostare `evalAST` dentro `parse` e restituire la SONG con `return ctx->ast_result` / `pthread_join(t, &ret)`. Nel reduce fare un'unica `xrealloc` a `len1+len2`. Rimisurare.
3. Generalizzare a N thread (N confini di riga, array di `CpCtx`, reduce in sequenza) e tracciare lo speedup per N = 1, 2, 4, 8 con lo script, ricompilando prima il binario usato da `run_test.sh`.
