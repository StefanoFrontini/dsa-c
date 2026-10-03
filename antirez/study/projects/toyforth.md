# Toy Forth (interprete stile antirez) — stato: in-progress
Path: `toyforth/toyforth.c` (+ `toyforth/test.txt`) · Ultima modifica: 2026-03-16 (commit "toyforth v10"; fermo da allora)

## Cosa fa
Interprete di un Forth minimale, scritto seguendo la serie video di antirez. Legge un file, lo trasforma in una lista di oggetti `TfObj` con refcount (interi e simboli) e lo esegue su uno stack (`ctx->stack`). Le funzioni sono in una tabella di function pointer. È il primo progetto della conversazione (turni 1-2: refcount e ownership).

## Come si compila / si esegue
```sh
cc -W -Wall -g toyforth/toyforth.c -o $BUILD/toyforth
$BUILD/toyforth toyforth/test.txt          # test.txt = "10 10 +"
leaks --atExit -- $BUILD/toyforth toyforth/test.txt
```

## Esito verifica
- Compila con 2 warning: `switch` senza i casi INT/BOOL/ALL in `freeObject` (`toyforth.c:114`) e confronto signed/unsigned tra `TfobjType` e `int` (`toyforth.c:225`).
- `test.txt` ("10 10 +") → `Stack content at end: [20]`. Con "5 5 + 3 +" → `[13]`.
- "1 2 - 3" → `Run time error`: è registrato solo `+` (`toyforth.c:381`), anche se lo `switch` gestisce `-` e `*`.
- `"hello"` (stringa) → `Error parsing` + **segfault** (exit 139): `parse` restituisce NULL e il main chiama `printObject(NULL)` (`toyforth.c:468-469`).
- `leaks`: **15 leak, 400 byte**. Il main non libera nulla (`prgtext`, `parsed`, `ctx`). Aggiungendo la pulizia in una copia in build/ restano **2 leak**: il caso `LIST` di `freeObject` non fa `free(o->list.ele)` (`toyforth.c:115-120`).

## Cosa funziona
- Parsing di interi (anche negativi) e simboli; esecuzione di `+` su stack con controllo del tipo (`ctxStackPop(ctx, INT)`) e ripristino di `b` se `a` manca (`toyforth.c:424-444`).
- Ownership corretta in `exec` (retain quando condivide con `prg`, `toyforth.c:413-415`), nelle funzioni matematiche (release dopo pop, push senza retain del nuovo oggetto) e in `registerFunction`/`registerCfunction` (`retain(name)` poi `release(oname)`, `toyforth.c:340-374`).

## Cosa manca / bug noti
- `freeObject` non libera l'array della lista (`toyforth.c:115-120`): leak confermato.
- Nessuna pulizia finale nel main (`toyforth.c:472-479`) e nessun controllo di `parse() == NULL` prima di `printObject` (`toyforth.c:468-469`).
- `perror("Error parsing\n")` stampa "Undefined error: 0": `errno` non c'entra, va usato `fprintf(stderr, ...)` (`toyforth.c:303`).
- `result` non inizializzato se il simbolo non è `+ - *` (`toyforth.c:434-439`). Sono registrati solo `+` (`toyforth.c:381`).
- `listPopType` riceve `ctx` invece della lista, quindi funziona solo sullo stack principale (`toyforth.c:221`).
- Mancano: stringhe `"..."`, liste `[...]`, `dup`, `print`, `if`, funzioni utente (`fe->user_func` è un TODO, `toyforth.c:391-393`). Gli esempi in testa al file (`toyforth.c:1-4`) non sono ancora supportati.
- Il codice non compare nella conversazione oltre ai turni 1-2 (incollato lì quasi identico).

## Concetti applicati
- [Puntatore sullo stack, oggetto nell'heap](../notes/memoria-c.md#puntatore-sullo-stack-oggetto-nellheap)
- [Reference counting](../notes/memoria-c.md#reference-counting-retain--release)
- [Trasferimento vs condivisione](../notes/memoria-c.md#trasferimento-vs-condivisione-dellownership)
- [Convenzione retain fuori dalle utility](../notes/memoria-c.md#convenzione-retain-fuori-dalle-utility) (commento `toyforth.c:213-214`)
- [xmalloc/xrealloc](../notes/memoria-c.md#wrapper-xmalloc--xrealloc)

## Prossimo passo consigliato
1. Aggiungi `free(o->list.ele)` nel caso `LIST` di `freeObject` e una `freeContext()` nel main. Poi riverifica con `leaks --atExit` (obiettivo: 0 leak).
2. Controlla `parsed == NULL` nel main e registra `-` e `*` in `createContext`.
3. Prossima funzione della serie: `dup` (richiede `retain` dell'oggetto duplicato: buon esercizio di ownership).
