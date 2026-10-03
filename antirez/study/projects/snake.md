# Snake (umano + IA BFS / A*) — stato: working
Path: `snake/` (`snake.c`, `platform.h`, `platform_mac.c`, `minHeap.c`, `CMakeLists.txt`, `snake-gemini-chat.md`) · Ultima modifica: `snake.c` 2026-02-23 (commit "refactor2"); cartella 2026-05-27 (aggiunta della chat). **Non è nella conversazione principale**: lo stato si ricava da `snake/snake-gemini-chat.md`, una chat Gemini separata esportata il 26/05/2026, senza numeri di turno.

## Cosa fa
Snake nel terminale, su una griglia 30x15 con game loop a 60 FPS e serpente che si muove ogni 100 ms. Tre modalità:
- `./snake`: umano, frecce + ESC.
- `./snake bfs [perf]`: IA che cerca il percorso con BFS, opzionalmente con misura delle prestazioni.
- `./snake astar`: IA con A*, distanza di Manhattan e min-heap.

Se l'IA non trova un percorso entra in "SurvivalMode" (prima mossa sicura). L'I/O di piattaforma è isolato in `platform.h`/`platform_mac.c` (termios raw, `gettimeofday`, `usleep`) in vista di un porting sul Raspberry Pi Pico.

Percorso del progetto secondo la chat: coda BFS su array invece che su linked list → backtracking con `parent[]` → modalità umano/IA → bug di game over → pattern *strategy* (`controller` come function pointer) → misure con `sample`/`time`/`leaks` → serpente da linked list a **ring buffer** → frutto incorporato nel contesto (zero malloc) → `run_id` al posto di `memset` → `platform.h` + CMake → min-heap → A*. L'ultimo bug della chat (A* fermo: controller non assegnato nel main) è **già corretto** in `snake.c:710-714`.

## Come si compila / si esegue
```sh
cmake -S snake -B $BUILD/snake -DCMAKE_BUILD_TYPE=Debug && make -C $BUILD/snake
perl -e 'alarm 12; exec @ARGV' -- $BUILD/snake/snake bfs   < /dev/null > run.out
perl -e 'alarm 12; exec @ARGV' -- $BUILD/snake/snake astar < /dev/null > run.out
cc -W -Wall -g snake/minHeap.c -o $BUILD/snake/minHeap && $BUILD/snake/minHeap   # test isolato del min-heap
```
(Il programma è interattivo: con stdin da `/dev/null` `read` restituisce 0, quindi nessun tasto. Il timeout lo ferma con exit 142.)

## Esito verifica
- CMake e make: build senza warning (`-W -Wall` dal CMakeLists). `minHeap.c` stampa `2` e `5` (estrae correttamente i minimi).
- `bfs`: in circa 10 s il punteggio arriva a 5, nessun GAME OVER, nessun SurvivalMode.
- `astar`: in circa 10 s il punteggio arriva a 6, nessun GAME OVER.
- Umano: parte, resta in IDLE senza input (corretto).
- Memoria: nessuna `malloc` in `snake.c` (zero allocazioni). `sizeof(GameContext)` = 5920 B sullo stack del main (`snake.c:693`). `a_star` mette sullo stack `minHeap` (7204 B) + `g_score` (1800 B) (`snake.c:443-446`), `bfs_path` mette `BFSQueue` (908 B).

## Cosa funziona
Game loop con delta time e frame capping, buffer di input circolare (3 tasti), BFS e A* con backtracking, ring buffer per il corpo, strategy pattern per i controller, layer di piattaforma separato, metriche FPS.

## Cosa manca / bug noti
- `snake.c:524` `runSurvivalMode`: `ctx->snake.body[ctx->snake.head - 1]`. Quando il ring buffer si richiude (`head == 0`) legge `body[-1]`, fuori dall'array. Altrove si usa giustamente `(head - 1 + GRID_CELLS) % GRID_CELLS` (`snake.c:83`, `:270`, `:448`). Bug latente: serve che il SurvivalMode scatti proprio dopo il giro.
- `snake.c:367-370` `insertHeap` non controlla `count < GRID_CELLS`. In A* lo stesso nodo può essere reinserito più volte, se trova un g migliore: oggi non succede, ma non è garantito.
- `snake.c:268` e `:440`: due `static int run_id` separati condividono `ctx->visited`. Se BFS e A* venissero usati nella stessa partita, si avrebbero falsi "visitato".
- `Node n = {..., .h = heuristic(n.p, b)}` (`snake.c:494`) legge `n.p` dentro il proprio inizializzatore. L'ordine di valutazione degli inizializzatori non è garantito dallo standard: meglio calcolare `Point np` prima.
- `printGrid(char *grid, char score)` (`snake.c:647`): un punteggio > 127 va in overflow, meglio `int`.
- `platform_mac.c`: `read` che restituisce -1 non è gestito (`c` resta non inizializzato). `updateFruit` va in loop infinito se la griglia è piena.
- Porting sul Pico non iniziato. Serve `static GameContext` (stack del Pico da 2 KB, chat snake) e spostare `minHeap` fuori dallo stack di `a_star`.
- Hamiltonian (citato nella chat) non implementato.
- Note sulla chat Gemini: "malloc chiede memoria al sistema operativo" e "452 nodi allocati da Terminal/I/O" sono imprecisi. Sono allocazioni della libc/dyld *dentro* il processo, e `malloc` è codice user space.

## Concetti applicati
- [Cache locality](../notes/memoria-c.md#cache-locality-e-prefetch): ring buffer e coda su array invece della linked list.
- [run_id](../notes/memoria-c.md#run_id-azzerare-senza-memset)
- [Regola stack vs heap](../notes/memoria-c.md#regola-di-scelta-stack-vs-heap) e [buffer grandi sullo stack](../notes/memoria-c.md#buffer-grandi-sullo-stack--stack-overflow) (Pico)
- [Contesto sullo stack](../notes/memoria-c.md#contesto-sullo-stack--initcontext-caller-allocates-callee-initializes): `GameContext game_ctx` + `initGame(&game_ctx, ...)`.

## Prossimo passo consigliato
1. Correggi `snake.c:524` usando lo stesso calcolo modulare di `getNextHeadPosition`. Prova a forzarlo con una `GRID` piccola, per esempio 5x5.
2. Aggiungi un controllo in `insertHeap` (`assert(h->count < GRID_CELLS)`) e sposta `run_id` in `GameContext`, unico per BFS e A*.
3. Prepara il porting sul Pico: `static GameContext game_ctx;` e `minHeap` dentro il contesto. Misura con `sizeof` quanta RAM statica serve.
