# math-pratt — stato: working
Path: `math-pratt/` (`math-pratt.c`, `test.txt`) · Ultima modifica: 2026-05-02 (git log; sorgente `math-pratt.c` mtime 2026-04-29)

## Cosa fa
Legge un'espressione aritmetica intera da file. La analizza con un Pratt parser: `+ - * /`, meno unario, parentesi. Costruisce un AST di `PObj` con refcount e lo stampa in tre formati: albero tipo `tree`, XML e JSON. Infine lo valuta con `evalAST`.

## Come si compila / si esegue
```sh
BUILD=<cartella build>
cc -W -Wall -g math-pratt/math-pratt.c -o $BUILD/math-pratt     # 0 warning
$BUILD/math-pratt math-pratt/test.txt                          # test.txt = "2*3*4"
leaks --atExit -- $BUILD/math-pratt <file>
```
L'espressione si legge **da file** (non da argv). Un file vuoto (0 byte) è rifiutato con "File is empty".

## Esito verifica
- `test.txt` (`2*3*4`): albero `*(*(2,3),4)`, XML e JSON corretti, `Result is: 24`, exit 0.
- `leaks`: 0 leak su un'espressione valida (`( ( 1 + 2 ) * ( 3 - 4 ) )`) e anche sul percorso d'errore `(1+2`.
- Input al limite (risultati reali):

| Input | Esito |
|---|---|
| `1+2*3` | 7 |
| `(1 + 2) * 3` / `(1+2)*3` | 9 / 9 (bug degli spazi risolto) |
| ` ( ( 1 + 2 ) * ( 3 - 4 ) ) ` | -3 |
| `((((7))))` | 7 |
| `--1`, `---1`, `-(-(2))` | 1, -1, 2 |
| `1--1`, `1 - -1`, `2*-3`, `-2*3` | 2, 2, -6, -6 |
| `10-3-2`, `100/7/2` | 5, 7 (associatività a sinistra ok) |
| `1/0` | "Runtime error: Division by zero!", exit 1 |
| `(1+2` | "Syntax error: close paren expected ')'", exit 1 |
| `1+2)` | 3: la `)` in più è ignorata in silenzio |
| `1 2`, `1 $ 2` | 1: il resto è ignorato in silenzio |
| `-`, `1+`, `()`, `1 + )`, `a+1` | **segfault (exit 139)** in `evalAST(NULL)` |
| solo spazi/`\n` | "Token 9 cannot be used in prefix notation" → **segfault** |
| `1+` + numero di 130 cifre | "Number literal too long" → albero sbagliato → **segfault** |
| `2147483647+1` | -2147483648 (overflow `int`, comportamento indefinito) |
| `99999999999` | 1215752191 (`atoi` senza controllo) |
| `-2147483648/-1` | **SIGFPE (exit 136)** |
| `1+1+…+1` con 150 termini / con 200 termini | 150 / "Errore: Buffer dell'albero troppo piccolo!", exit 1 |
| 200.000 `-` di fila, 100.000 parentesi annidate | **segfault**: stack overflow nella ricorsione del parser |

## Cosa funziona
- Pratt parser completo per `+ - * /` con precedenze 2/3, associatività a sinistra, meno unario (`PREFIX_PRECEDENCE 6`) e parentesi annidate a piacere.
- Tabella statica di function pointer (`math-pratt/math-pratt.c:103-112`), token per valore nel lexer, contesto sullo stack con `initContext` + `memset` (`:569-574`).
- `evalAST` post-order con controllo della divisione per zero (`:582-617`).
- Stampa `tree` con buffer condiviso + backtracking + `memcpy` (`:284-368`), `printXML` (`:377-421`), `printJSON` (`:423-480`).
- Nessun leak sugli input validi.

## Cosa manca / bug noti
- **`NULL` non gestito**: `parseExpression` ritorna `NULL` in caso di errore (`:546-549`, `:529-531`). `main` chiama `evalAST(ctx.ast)` senza controllo (`:661`) e anche gli INFIX con figlio `NULL` arrivano a `evalAST` → segfault. I messaggi d'errore vanno su `stdout` e si perdono se lo stdout è una pipe e il processo crasha.
- **Nessun controllo di fine input**: dopo `parseExpression(&ctx, 0)` (`:650`) non si verifica `curToken.type == TOKEN_ENDOFFILE`. `1 2`, `1+2)` e `1 $ 2` danno un risultato senza errore.
- `readNumber` (`:127-130`): con un numero troppo lungo fa `return` **senza impostare il token**, che resta quello precedente. `atoi` non segnala l'overflow (meglio `strtol` + `errno`).
- `isSymbolChar` (`:141-144`) accetta le lettere (`isalpha`), che poi diventano `TOKEN_ILLEGAL` con `printf("??? - %d")`. `TOKEN_SYMBOL` non è usato.
- `evalAST`: overflow di `int` e `INT_MIN / -1` non gestiti.
- `printTree`: buffer da 1024 byte, quindi non stampa alberi più profondi di circa 170 livelli ed esce con `exit(1)` prima di valutare (`:317-325`).
- Le funzioni ricorsive (`parseExpression` per gli unari/parentesi annidati, `evalAST`, `freeObject`, `printXML`, `printJSON`) vanno in stack overflow con input patologici.
- Non implementati (discussi): stampa RPN/postfissa (turno 90), BFS (turno 89), serializzazione su disco con `FILE *` (turno 99), arena allocator (turno 76).

## Concetti applicati
- [Struttura del Pratt parser](../notes/pratt-alberi.md#struttura-del-pratt-parser-prefix-nud-e-infix-led)
- [Binding power](../notes/pratt-alberi.md#binding-power--precedenza), [Tabella statica](../notes/pratt-alberi.md#tabella-di-parsing-statica-di-function-pointer)
- [Token/contesto sullo stack](../notes/pratt-alberi.md#token-sullo-stack-contesto-sullo-stack-ast-sullheap)
- [Meno unario](../notes/pratt-alberi.md#meno-unario-stesso-token-due-ruoli), [Parentesi](../notes/pratt-alberi.md#parentesi-un-muro-non-un-nodo), [Bug degli spazi](../notes/pratt-alberi.md#bug-del-loop-infinito-con-gli-spazi-1--2--3)
- [evalAST](../notes/pratt-alberi.md#evalast-interprete-tree-walk-dfs-post-order), [Attraversamenti](../notes/pratt-alberi.md#attraversare-un-albero-dfs-preinpost-order-e-bfs)
- [Stampa tipo tree](../notes/pratt-alberi.md#stampa-ad-albero-tipo-tree-snprintf-buffer-sullo-stack-backtracking), [XML/JSON](../notes/pratt-alberi.md#serializzazione-xml-e-json)

## Prossimo passo consigliato
1. In `main`, dopo il parsing: se `ctx.ast == NULL` oppure `ctx.lexer.curToken.type != TOKEN_ENDOFFILE`, stampa "Syntax error" su `stderr` ed esci con 1. Fai controllare anche a `parseInfix` che `right` non sia `NULL`. Ritesta `-`, `1+`, `()`, `1 2`.
2. Aggiungi `printRPN(PObj*)` (post-order, turno 90) e `serializeJSON(PObj*, int, FILE*)` che scrive `ast.json` con `fprintf` (turno 99).
3. Sostituisci `atoi` con `strtol` + controllo di `errno`/range e imposta comunque `TOKEN_ILLEGAL` quando il numero è troppo lungo.
