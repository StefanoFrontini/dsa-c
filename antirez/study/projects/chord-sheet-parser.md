# Chord Sheet Parser (Recursive Descent)  — stato: broken
Path: antirez/chord_sheet_parser · Ultima modifica: 2026-03-24 (commit "chord v16")

## Cosa fa
Legge un file di testo con accordi inline (`Now I've [C]heard there was a`) e costruisce l'AST
SONG → LINE → WORD → (CHORD, LYRIC) con un parser a discesa ricorsiva (una funzione per regola della grammatica,
commento a `chord_sheet_parser.c:96-118`), oggetti con reference counting e nodi `ERROR` dentro l'albero.

## Come si compila / si esegue
```
cc -W -Wall -g chord_sheet_parser/chord_sheet_parser.c -o $BUILD/csp      # FALLISCE (vedi sotto)
# copia corretta solo nella build dir per poter testare il resto:
sed 's/isStringConstant(l->p\[0\])/isStringConstant(l)/' chord_sheet_parser/chord_sheet_parser.c > $BUILD/csp_patched.c
cc -W -Wall -g $BUILD/csp_patched.c -o $BUILD/csp_patched
$BUILD/csp_patched chord_sheet_parser/test.txt
leaks --atExit -- $BUILD/csp_patched chord_sheet_parser/test.txt
```
L'`a.out` nella cartella (20 marzo) è più vecchio del sorgente: non rappresenta il codice attuale.

## Esito verifica
- Compilazione (Apple clang 17): `chord_sheet_parser.c:462:27: error: incompatible integer to pointer conversion
  passing 'char' to parameter of type 'Lexer *' [-Wint-conversion]` → **non compila**.
- Con la correzione di 1 riga (solo nella build dir), 0 warning:
  - `test.txt` (`Now I've been]C`): stampa `Now I've been(type: 4)Error parsing object: 2. Expected token: 4 Got: 2`
    e poi `C` → errore riportato nell'albero; **leaks: 1 leak da 32 byte** (la WORD di `parseWord`).
  - `Now I've [C]heard there was a\nsecret [G]chord` → AST corretto, 0 leaks.
  - `[A]aaa[B]bbb\n[C]ccc[D]ddd` → corretto.
  - `[C` → nodo ERROR (`Expected token: 2 Got: 4`), 0 leaks. `[]x` → errore + `x`, 0 leaks. File vuoto → nessun
    output, 0 leaks.

## Cosa funziona
- Grammatica e funzioni `parseSong/parseLine/parseWord/parseChord/parseLyric` (`:513-602`), ripetizioni con `while`.
- Lexer separato con `advanceLexer`/`eatToken` (`:460-507`), token con refcount.
- Parole senza accordo (chord vuoto) e accordi senza testo (lirica vuota) (`:559-569`).
- Error recovery parziale: gli errori di `parseChord` finiscono dentro la WORD e il parsing continua.
- Liberazione a cascata dell'albero (`releaseCsObj :281-304`), incluso l'array `list.ele`.

## Cosa manca / bug noti
- `chord_sheet_parser.c:462`: `isStringConstant(l->p[0])` deve essere `isStringConstant(l)` (non compila).
- `chord_sheet_parser.c:571-575`: nel ramo d'errore la `WORD` creata a `:555` non viene rilasciata → leak 32 byte.
- `:501`: `eatToken` avanza anche quando il token è sbagliato (lo consuma); nessuna sincronizzazione su `\n`.
- `:392`: `isascii` → lettere accentate/UTF-8 diventano `ILLEGAL` (testi italiani rotti).
- `:519-520` / `:541-542`: legge `t->str.ptr` dalla union prima di sapere se il token è davvero `STR`
  (innocuo perché in caso d'errore non lo usa, ma è lettura di un campo non valido).
- `:573`: il messaggio d'errore di `parseWord` dice "expected ENDOFFILE", poco utile; i codici numerici
  (`otype + 48`) non sono leggibili per un utente.
- `createChordObject/createLyricObject` con `len == 0` salvano il puntatore passato (`:226-228`): funziona solo
  perché si passa `NULL`.
- Stampa di debug `(type: N)` al posto dell'output JSON/TS richiesto dal commento iniziale.

## Concetti applicati
- [Recursive Descent](../notes/parsing.md#recursive-descent-grammatica--una-funzione-per-regola)
- [Error recovery vs fail-fast](../notes/parsing.md#error-recovery-vs-fail-fast)
- [Confine lexer/parser](../notes/parsing.md#confine-lexer--parser)
- [AST finale vs evaluation](../notes/parsing.md#ast-finale-vs-ast--evaluation-la-mappa-mentale)
- [ADT sum/product](../notes/algebra-fp.md#algebraic-data-types-sum-e-product)

## Prossimo passo consigliato
1. Correggere `:462` in `isStringConstant(l)` e ricompilare con `-W -Wall`.
2. In `parseWord` (`:571-575`) chiamare `releaseCsObj(o)` prima di `return error`, poi `leaks` su `test.txt`.
3. Decidere la strategia d'errore: o fail-fast coerente, o recovery con sincronizzazione al prossimo `\n`.
