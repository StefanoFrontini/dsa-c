# Game of Life — stato: done
Path: `game_of_life/game_of_life.c` · Ultima modifica: 2025-07-14 (commit "game of life"). La versione successiva è `life.c` nella root (2026-01-10, vedi scheda esercizi-c-base). Non compare nella conversazione con Gemini.

## Cosa fa
Il Game of Life di Conway nel terminale, su una griglia 20x20 con bordi che si richiudono su se stessi (toroidale). La griglia è un array lineare `char[GRID_CELLS]` sullo stack del main, con indice `r * GRID_COLS + c`. Parte da un glider, lo ridisegna con le sequenze ANSI `\x1b[3J\x1b[H\x1b[2J` ogni 100 ms e alterna due griglie (old → new, new → old).

## Come si compila / si esegue
```sh
cc -W -Wall -g game_of_life/game_of_life.c -o $BUILD/game_of_life
perl -e 'alarm 2; exec @ARGV' -- $BUILD/game_of_life > out.txt   # loop infinito: serve il timeout
```

## Esito verifica
- Compila senza warning.
- In 2 secondi produce 10 frame. Il glider si sposta correttamente in diagonale (frame 0 → 4: una cella a destra e una in basso). Exit 142 = SIGALRM del timeout, come previsto.
- Test dei casi limite di `cell_to_index` (in build/): `(0,-1)` → 19 ok; `(0,-21)` → 19 ok; ma `(0,-20)` → 20 e `(-20,0)` → **400 = fuori dall'array**.

## Cosa funziona
Regole di Conway, wrap dei bordi per i vicini a distanza 1 (gli unici usati), doppio buffer senza `malloc` (tutto sullo stack: 2 x 400 byte).

## Cosa manca / bug noti
- `game_of_life.c:29-36`: con coordinate negative multiple di 20, `GRID_COLS - (-c % GRID_COLS)` dà 20 invece di 0, quindi un indice fuori dai limiti. Il bug è **latente**: i vicini sono sempre a ±1. In `life.c:36-43` è risolto, perché il controllo `>= GRID_COLS` viene dopo.
- `game_of_life.c:25`: `if (r >= GRID_COLS)` dovrebbe essere `GRID_ROWS` (innocuo finché la griglia è quadrata).
- Variabili `char` come contatori di ciclo (`game_of_life.c:51,53,62,...`): vanno bene fino a 127, ma `int` è più chiaro.
- Nessuna uscita pulita dal loop (solo Ctrl-C) e niente input iniziale da file.

## Concetti applicati
- [Regola stack vs heap](../notes/memoria-c.md#regola-di-scelta-stack-vs-heap): dimensione nota e piccola, quindi array sullo stack.
- [Cache locality](../notes/memoria-c.md#cache-locality-e-prefetch): griglia contigua scorsa riga per riga.
- Evoluzione in `life.c`: scambio di puntatori `old/new/tmp` (`life.c:146-156`) invece di due chiamate duplicate.

## Prossimo passo consigliato
1. Correggi `cell_to_index` con il modulo "sempre positivo": `((c % N) + N) % N`. Così elimini tutti i casi speciali.
2. Leggi la configurazione iniziale da un file di testo (`.` e `*`) passato come `argv[1]`.
3. Facoltativo: griglia nell'heap con dimensioni passate da riga di comando (`calloc(rows * cols, 1)`), per ripassare la scelta stack/heap.
