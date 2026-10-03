# Esercizi C di base (file sparsi nella root) — stato: working
Path: `*.c` + `hello_world.s` nella root di `antirez/` · Ultima modifica: 2026-01-10 → 2026-02-13 (bst*.c). Non compaiono nella conversazione con Gemini: sono esercizi di gennaio-febbraio 2026 (video di antirez e prove sulla memoria). Nel repo ci sono anche `a.out`, `memoria.bin` e `database.txt`, generati da `matrix_dump.c` e `mmap_test.c`.

## Cosa fa
Raccolta di piccoli programmi didattici. Una riga per file:

| File | Cosa insegna | Esito |
|---|---|---|
| `hello_world.c` | ciclo fatto con `goto` (etichetta `again:`) | ok, stampa 0..9 |
| `hello_world.s` | assembly **arm64** (M1) di un *altro* hello world (`printf("Hello World \n")`), non corrisponde all'attuale `hello_world.c` | si assembla solo con `cc -arch arm64 -c`; su Intel `cc hello_world.s` fallisce |
| `struct.c` | struct nell'heap con `malloc(sizeof(*f))`, `sizeof(struct fract)` = 8, semplificazione di una frazione | ok, ma **2 leak** (f1, f2 mai liberate, `struct.c:31-36`) |
| `typedef.c` | `typedef` di struct e di puntatore (`fractptr`), indirizzo di una variabile sullo stack | ok |
| `tac.c` | lista concatenata costruita in testa = stampa delle righe al contrario; `fgets` in un buffer sullo stack da 1024 | ok, ma **leak di ogni riga**: libera il nodo ma non `l->s` (`tac.c:44`); `leaks` conferma 16 leak su 16 righe |
| `pls.c` | stringa con prefisso di lunghezza + refcount + `magic` **nascosti prima del puntatore** (trucco stile sds); flexible array member `char str[]` | ok, 0 leak; il commento a `pls.c:12` ("L è un byte") è vecchio: `len` è `uint32_t` |
| `issue1.c` | layout di una struct (`long` + `char[24]` = 32 byte) e byte **non inizializzati** sullo stack visti con hexdump | ok; mostra spazzatura dopo il `\0` |
| `hexdump.c` | funzione hexdump (base di stdio1/2, issue1) | **non compila**: manca `#include <stdio.h>`, quindi "call to undeclared library function 'printf'" (errore in C99+ con clang 17) |
| `stdio1.c` | `fopen` + `fread` a blocchi di 32 byte | ok (legge `stdio1.c` dalla cwd); se `fopen` fallisce continua comunque e va in crash (`stdio1.c:32-38`) |
| `stdio2.c` | stessa cosa con le syscall `open`/`read` (file descriptor invece di `FILE*`) | ok (legge `stdio3.c`) |
| `stdio3.c` | `mmap` di un file in sola lettura | ok; nessun controllo di errore, `munmap` e `close` mancanti |
| `mmap_test.c` | `ftruncate` + `mmap(MAP_SHARED, PROT_WRITE)` + `memcpy` + `msync`: scrivere su un file come se fosse RAM | ok; **scrive `database.txt` nella cwd** |
| `matrix.c` | buffer overflow didattico: `scanf("%s")` in `char buffer[10]` per sovrascrivere `is_admin` | su Intel `is_admin` sta **sotto** `buffer`, quindi non viene toccato; con input lungo: abort (exit 134, stack protector) o segfault con `-fno-stack-protector` |
| `matrix_dump.c` | dump binario dello stack su file con `fwrite` (`memoria.bin`) | ok; scrive `memoria.bin` nella cwd; usa aritmetica su `void *` (estensione GNU, `matrix_dump.c:25`) |
| `life.c` | Game of Life v2 (gen 2026): griglia `char[400]` sullo stack, scambio di puntatori `old/new` invece di copiare | ok (loop infinito, vedi scheda game-of-life) |
| `bst.c` | albero binario di ricerca con inserimento tramite **doppio puntatore** `struct Node **root` | ok, 0 leak |
| `bst2.c` | stesso BST con inserimento che **ritorna la nuova radice** (`root->left = add_node(...)`); ignora i duplicati | ok, 0 leak |
| `funcptr.c` | puntatori a funzione come parametro (`call_n_times`) | ok |
| `funcptr2.c` | `qsort` con comparatore `const void *` | ok; `rand()` senza `srand`, quindi stessa sequenza a ogni avvio |

## Come si compila / si esegue
```sh
B=$SCRATCH/build/memoria/base
for f in bst bst2 funcptr funcptr2 hello_world hexdump issue1 life matrix matrix_dump mmap_test pls stdio1 stdio2 stdio3 struct tac typedef; do
  cc -W -Wall -g $f.c -o $B/$f
done
# eseguire in $B (mmap_test e matrix_dump scrivono file nella cwd; stdio1/3 leggono il proprio sorgente)
cd $B && cp ~/dev/dsa-c/antirez/{stdio1.c,stdio3.c,typedef.c} . && ./tac typedef.c
echo abc | ./matrix
leaks --atExit -- ./tac typedef.c
```

## Esito verifica
- 18/19 compilano senza warning con `-W -Wall`. `hexdump.c` dà errore di compilazione.
- `leaks`: `struct` 2 leak (32 B); `tac` 16 leak (512 B, le stringhe); `pls`, `bst`, `bst2`, `stdio3` 0 leak.
- `funcptr`: 10x "Hello!" + 10x "Bau Bau!". `funcptr2`: `1 2 3 7 8 8 9 10 13 14`. `bst`/`bst2`: `5 10 12`.
- `matrix_dump` (Intel): `buffer` a `...70e`, `is_admin` a `...704`, quindi l'int sta prima del buffer. Il `memoria.bin` del repo (M1) aveva un layout diverso.

## Cosa funziona
Tutti gli esercizi fanno quello che dichiarano, tranne `hexdump.c` (non compila) e `hello_world.s` (assembly arm64, non corrisponde al `.c`).

## Cosa manca / bug noti
- `hexdump.c:1`: aggiungere `#include <stdio.h>`.
- `tac.c:44`: `free(head->s)` prima di `free(head)`. Inoltre `free_mem` (`tac.c:9-14`) è inutilizzata e anche lei non libera `s`.
- `struct.c:36`: mancano `free(f1); free(f2);`.
- `stdio1.c:32-34`: dopo "Unable to open the file" manca `return 1`.
- `stdio3.c`: controllare `fd == -1` e `mem == MAP_FAILED`, poi `munmap`/`close`.

## Concetti applicati
- [Reference counting](../notes/memoria-c.md#reference-counting-retain--release) (`pls.c`)
- [Regola stack vs heap](../notes/memoria-c.md#regola-di-scelta-stack-vs-heap) (`issue1.c`, `matrix*.c`, `struct.c`)
- [mmap](../notes/memoria-c.md#mmap-un-file-trattato-come-memoria) (`mmap_test.c`, `stdio3.c`)
- [malloc non azzera](../notes/memoria-c.md#malloc-non-azzera-la-memoria-e-il-terminatore-0) (`issue1.c`, byte casuali)

## Prossimo passo consigliato
1. Correggi `hexdump.c` (include) e il leak di `tac.c`, poi rilancia `leaks --atExit -- ./tac file`.
2. Rigenera `hello_world.s` dall'attuale `hello_world.c` sull'Intel (`cc -S hello_world.c`) e confronta come il `goto` diventa `cmp`/`jmp`.
3. Su `pls.c`: aggiungi una `ps_concat` che crea una nuova pls. Ripassi `malloc(sizeof(struct pls) + len + 1)` e l'ownership del risultato.
