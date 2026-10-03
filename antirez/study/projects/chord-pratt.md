# chord-pratt (single thread) — stato: working
Path: `chord-pratt/chord-pratt.c` (+ `test.txt`, `test2.txt`, `stress_test*.txt`) · Ultima modifica: 2026-05-27 (git log)

Nella stessa cartella c'è anche `chord-pratt-threads.c` (versione MapReduce con 2 pthread, `evalAST` e `freeObject` iterative), con `run_test.sh` e `benchmarks.csv`. **Lo verifica un altro agente**; qui è solo citato. `a.out` nella cartella è un binario vecchio (7 maggio).

## Cosa fa
Analizza una canzone in formato `[Accordo]testo` riga per riga. Il lexer è una **macchina a stati** che inietta token fantasma:
- `SONG` e `\n` + `LINE` all'inizio;
- `*` tra due parole;
- accordo/testo vuoti quando mancano.

Così la canzone diventa un'espressione algebrica: `SONG \n LINE * WORD * WORD \n LINE * …`. Il Pratt parser costruisce l'AST; `\n` ha precedenza 2, `*` precedenza 3, e un accordo è un prefix che "assorbe" il testo che lo segue. `evalAST` appiattisce l'albero in `SONG → LINE → WORD(chord, lyric)`. L'AST e il risultato vengono stampati in XML.

## Come si compila / si esegue
```sh
cc -W -Wall -g chord-pratt/chord-pratt.c -o $BUILD/chord-pratt   # 0 warning
cd chord-pratt && $BUILD/chord-pratt test.txt
leaks --atExit -- $BUILD/chord-pratt test2.txt
```
Attenzione: con gli `stress_test*.txt` **non** lanciarlo con l'output su file o terminale. `printXML` indenta di 2 spazi per livello e l'albero è profondo quanto il numero di righe, quindi l'output è quadratico: con 50.000 righe ha prodotto più di 1 GB in 60 s ed è stato interrotto. Per misurare parsing + eval ho usato una copia in build con le due `printXML` disattivate (il sorgente originale non è stato toccato).

## Esito verifica
- `test.txt` (`[A]aaa[D][C]\n[B]bbb`): risultato `song → line(A/aaa, D/–, C/–) → line(B/bbb, –/–)`, exit 0, **0 leak**. L'ultima `WORD` vuota è un artefatto del lexer (vedi bug).
- `test2.txt` (110 righe): exit 0 in 0,12 s, **0 leak**, 111 LINE (la riga in più viene dal `\n` finale).
- Senza stampa XML (copia in build):

| File | Righe | Esito |
|---|---|---|
| stress_test.txt | 50.000 | ok, 0,47 s, 50.001 LINE |
| stress_test2.txt | 100.000 | ok, 0,87 s |
| stress_test3.txt | 200.000 | **SIGSEGV** (stack overflow in `evalAST`, lldb: `chord-pratt.c:672`) |
| stress_test4.txt | 600.000 | **SIGSEGV** |

  Con `ulimit -s 65520` anche 200k e 600k terminano correttamente. Il limite è quindi solo la ricorsione.
- Input al limite:

| Input | Esito |
|---|---|
| file vuoto | exit 0, canzone con una riga e una parola vuota (nessun controllo "file vuoto") |
| `\n` | una riga `empty_line` + una riga con una parola vuota |
| `hello` | parola senza accordo + parola vuota finale |
| `[A]`, `[A][B]` | accordi con `empty_lyric` + parola vuota finale |
| `[A]a b c` | gli spazi fanno parte del testo: `lyric = "a b c"` |
| `[A]ciao\r\n[B]x` | `\r` e `\n` producono **due** fine-riga: compare una `empty_line` in più |
| `[A]a\n\n[B]b` | riga vuota → `empty_line` (corretto) |
| `[A` | "Unmatched close paren", exit 1 |
| `[]x` | "No string after open paren", exit 1 |
| `[A]città` | **testo troncato a `citt`** e resto del file ignorato, exit 0 |
| `x]y` | `]` è un token illegale: il parsing si ferma, `y` viene perso, exit 0 |
| `[A]città\n[B]resto` | anche la riga 2 viene persa; 0 leak |

## Cosa funziona
- Lexer FSM (`chord-pratt/chord-pratt.c:290-381`) + Pratt parser (`:511-571`) + tabella statica (`:165-174`): la canzone diventa un albero con la precedenza corretta (le righe raggruppano le parole).
- `evalAST` (`:671-692`) appiattisce l'albero in SONG/LINE/WORD mutando i nodi foglia. Il refcount (`retain(b)`) fa sì che `release(ctx.ast)` liberi tutto: 0 leak.
- Gestione degli errori sintattici sulle parentesi quadre (`:203-218`).
- Ownership di `result` chiarita: niente `retain`/`release` superflui nel `main` (`:740-748`, turno 111).

## Cosa manca / bug noti
- **Stack overflow a ≥200.000 righe**: `evalAST` (`:680-686`) e `freeObject` (`:398-401`) sono ricorsive sulla spina sinistra. Le versioni iterative esistono solo in `chord-pratt-threads.c` (`:361`, `:737-770`) e non sono state riportate qui. Anche `printXML` (`:582-669`) è ricorsiva e la sua indentazione è O(profondità).
- **Caratteri non ASCII**: `isStringConstant` usa `isascii` (`:176-180`), quindi qualsiasi lettera accentata UTF-8 diventa `TOKEN_ILLEGAL`. Il parsing si ferma **in silenzio** (exit 0) e tronca la canzone. Grave per testi in italiano.
- **Token illegali e input residuo ignorati**: nessun messaggio se il parser si ferma prima di EOF (vedi `x]y`). `main` non controlla `ctx.lexer.curToken.type` dopo il parsing (`:734`).
- **Parola vuota finale**: a EOF (`:366-369`) il lexer emette sempre `*` + accordo vuoto + testo vuoto, quindi ogni canzone che non termina con `\n` ha una `WORD` vuota in coda. Un `\n` finale crea invece una LINE in più.
- **CRLF**: `\r` e `\n` sono due fine-riga separati (`:364-365`), quindi i file Windows hanno righe vuote extra.
- **Doppia allocazione delle stringhe**: `readLyric`/`readChord` fanno `xmalloc` (`:190`, `:224`), `advanceLexer` libera (`:291-295`) e `createLyricObject`/`createChordObject` ricopiano (`:474`, `:488`). Lo zero-copy (turno 110) non è stato fatto.
- `printXML`: il testo `empty_line`/`empty_song` usa `indent` invece di `indent+1` (`:638`, `:653`), un difetto solo estetico. Testi e accordi non vengono escapati (`<`, `&`).
- `PREFIX_PRECEDENCE` e il blocco commentato su `timespec` (`:10-48`) sono residui di altri progetti.

## Concetti applicati
- [Lexer come FSM](../notes/pratt-alberi.md#lexer-come-macchina-a-stati-finiti-fsm), [Confine lexer/parser](../notes/pratt-alberi.md#il-confine-tra-lexer-e-parser-è-una-scelta)
- [Struttura del Pratt parser](../notes/pratt-alberi.md#struttura-del-pratt-parser-prefix-nud-e-infix-led), [Binding power](../notes/pratt-alberi.md#binding-power--precedenza)
- [Stringhe dei token e ownership](../notes/pratt-alberi.md#stringhe-dei-token-malloc-stack-zero-copy-ownership-di-result)
- [Stack overflow con alberi profondi](../notes/pratt-alberi.md#stack-overflow-con-alberi-profondi), [DFS iterativa](../notes/pratt-alberi.md#dfs-iterativa-con-stack-esplicito), [freeObject iterativa](../notes/pratt-alberi.md#freeobject-iterativa-eliminare-la-ricorsione-in-coda-a-sinistra)
- [Serializzazione XML](../notes/pratt-alberi.md#serializzazione-xml-e-json)

## Prossimo passo consigliato
1. Riporta in `chord-pratt.c` la `evalAST` iterativa e la `freeObject` iterativa di `chord-pratt-threads.c`. Ritesta `stress_test3.txt` con lo stack di default (8 MB) e la stampa XML disattivata.
2. In `isStringConstant` accetta i byte ≥ 0x80, cioè togli `isascii` e confronta come `unsigned char`. Ritesta `[A]città`.
3. Dopo il parsing, se il token corrente non è `TOKEN_ENDOFFILE`, stampa "unexpected token" con la posizione (`ctx.lexer.p - ctx.lexer.prg`) ed esci con 1.
