# Pratt parsing in C e alberi (AST)
Periodo: 2026-04-25 → 2026-05-27 · Turni: 74-99, 104-105, 110-111, 128-135

## Mappa in breve
- Si parte da un parser di espressioni intere (`math-pratt`): prima solo `+`, poi `- * /`, il meno unario, le parentesi, la valutazione (`evalAST`) e tre modi di stampare l'albero (`tree`, XML, JSON). Turni 74-99.
- Lungo la strada si impara **dove mettere la memoria**: token per valore dentro il lexer, tabella di parsing statica, contesto sullo stack, nodi dell'AST sull'heap (turni 75-83).
- La stessa struttura Pratt viene riusata per il **Chord Parser** (`chord-pratt`): il lexer diventa una macchina a stati (FSM) che inserisce token "fantasma" (`*`, `\n`, accordo/testo vuoti). Così una canzone diventa un'espressione algebrica (turni 104-105, 110-111).
- Con file enormi (200.000 righe) l'albero, sbilanciato tutto a sinistra, manda in **stack overflow** le funzioni ricorsive. Soluzione: una DFS iterativa con stack esplicito sull'heap e una `freeObject` iterativa (turni 128-135).
- Collegamenti: refcount/ownership (area C/memoria), chord parser con recursive descent e shunting-yard (area parsing), multithread/MapReduce e benchmark (altro agente: `chord-pratt-threads.c`, `run_test.sh`).

## Concetti

### Struttura del Pratt parser: prefix (nud) e infix (led)
- Cos'è: ogni token può avere due funzioni. La **prefix** (nud) si usa quando il token apre un'espressione (numero, `-` unario, `(`). La **infix** (led) si usa quando il token arriva dopo un'espressione già letta (`+`, `*`). `parseExpression(prec)` chiama la prefix del token corrente, poi resta in un `while` e chiama le infix finché l'operatore successivo "lega più forte" di `prec`.
- Nel tuo codice: `math-pratt/math-pratt.c:542-565` (`parseExpression`), `:509-532` (`parsePrefix`), `:534-540` (`parseInfix`, che passa la *propria* precedenza alla chiamata ricorsiva: così gli operatori sono associativi a sinistra). Stessa struttura in `chord-pratt/chord-pratt.c:511-571`.
- Decisioni: ogni funzione di parsing consuma da sola il proprio token. Prima `parseExpression` faceva `advanceLexer` dopo la prefix, solo per i numeri: era un "hack" asimmetrico, poi rimosso (turno 84). Nota: quell'asimmetria veniva dal file completo che Gemini ti aveva dato nel turno 81.
- Errori/lezioni: il contesto (unario o binario?) non va dedotto guardando il token precedente. Lo decide *in quale punto* dell'algoritmo ti trovi: inizio espressione → prefix, dentro il `while` → infix (turno 84).
- Turni: 74, 81, 84.

### Binding power / precedenza
- Cos'è: un numero per ogni operatore infix. Il `while` continua finché `prec < precedenza(token corrente)`. I token che non sono operatori (numeri, EOF, `)`, token illegali) hanno precedenza 0, che fa uscire dal ciclo. Il numero in sé non ha bisogno di una precedenza "vera" (turno 74: avevi messo -1 ai NUMBER).
- Nel tuo codice: `math-pratt/math-pratt.c:103-112` (`+ -` = 2, `* /` = 3), `:10` (`PREFIX_PRECEDENCE 6`). In chord-pratt: `\n` = 2, `*` (giustapposizione delle parole) = 3 (`chord-pratt/chord-pratt.c:165-174`).
- Decisioni: i token assenti dalla tabella sono inizializzati a zero, quindi precedenza 0 e funzioni NULL. È comodo, ma vuol dire anche che un token illegale **ferma il parsing in silenzio** (vedi la scheda del progetto).
- Turni: 74, 84, 85.

### Tabella di parsing statica di function pointer
- Cos'è: un array globale `static` indicizzato dall'`enum` del token, scritto con i *designated initializers* (`[TOKEN_PLUS] = {NULL, parseInfix, 2}`). Lookup O(1), niente `malloc`, niente `addRule` e nessun ciclo di ricerca.
- Nel tuo codice: typedef `PrefixFn`/`InfixFn` in `math-pratt/math-pratt.c:88-95`, prototipi in `:97-99`, tabella in `:103-112`.
- Decisioni: sei passato da una tabella dinamica (`pTable` con `xrealloc` + ricerca lineare, turno 74) all'array statico (turno 79). "Statico" non significa "sullo stack di `createContext`": un array locale restituito tramite puntatore diventerebbe un dangling pointer (turno 78).
- Errori/lezioni: spostare la tabella in fondo al file ha causato `use of undeclared identifier 'rulesTable'` (turno 80). L'ordine giusto è: prototipi delle funzioni di parsing, poi tabella, poi funzioni che la usano.
- Turni: 74, 78, 79, 80, 81.

### Token sullo stack, contesto sullo stack, AST sull'heap
- Cos'è: la regola di base è la **durata di vita**. Un token serve per un istante, quindi va tenuto per valore dentro `Lexer.curToken`. I nodi dell'AST devono sopravvivere alla funzione che li crea, quindi vanno sull'heap. Il contesto `pCtx` vive per tutto il `main`, quindi va sullo stack del `main` e si passa `&ctx` ("il chiamante alloca, la funzione inizializza").
- Nel tuo codice: `Token curToken` per valore in `math-pratt/math-pratt.c:55-59`. `initContext` con `memset` a zero in `:569-574`. `pCtx ctx` sullo stack in `:646-647`. `createObject` usa `xmalloc` in `:256-261`.
- Errori/lezioni: **bug della copia per valore** (turno 77). `Token t = ctx->lexer.curToken; t.type = ...` modificava una copia locale, che spariva a fine funzione. Correzione: scrivere `ctx->lexer.curToken.type = ...` oppure usare `Token *t = &ctx->lexer.curToken`. Inoltre `freeObject` deve chiamare `release` sui figli, non `freeObject`, altrimenti salta il refcount (turni 74, 77).
- Turni: 74, 75, 77, 82, 83.

### Arena allocator (solo teoria)
- Cos'è: una sola grande `malloc`, poi le allocazioni successive avanzano un offset. Alla fine basta una sola `free`. È adatto all'AST perché tutti i nodi muoiono insieme. Rimuove la necessità di `retain`/`release`/`freeObject`. Attenzione all'allineamento: la bozza di Gemini lo ignora.
- Nel tuo codice: non implementato; si usa ancora il refcount (`math-pratt/math-pratt.c:228-254`).
- Turni: 76.

### Meno unario: stesso token, due ruoli
- Cos'è: `TOKEN_MINUS` ha sia la prefix (negazione) sia la infix (sottrazione). Nella prefix si chiama `parseExpression(PREFIX_PRECEDENCE)` con un valore più alto di `*` e `/`. Così `-2*3` diventa `(-2)*3` invece di `-(2*3)`.
- Nel tuo codice: `math-pratt/math-pratt.c:106` (riga della tabella), `:514-517` (`createNegationObject`).
- Decisioni: con precedenza 2 il risultato numerico era uguale, ma l'albero era diverso. Con un operatore di potenza conta: per convenzione matematica `-2^2 = -4`, quindi l'unario dovrebbe stare *sotto* `^` e *sopra* `*` (vedi errori di Gemini).
- Verificato: `--1` = 1, `---1` = -1, `1--1` = 2, `1 - -1` = 2, `2*-3` = -6, `-(-(2))` = 2.
- Turni: 84.

### Parentesi: un "muro", non un nodo
- Cos'è: `(` ha solo la prefix. Consuma `(`, chiama `parseExpression(0)` (riparte da precedenza zero), poi **esige** `)` e la consuma. `)` ha precedenza 0, quindi ferma il `while` interno. Nell'AST non esiste un nodo "parentesi": la forma dell'albero basta a codificare il raggruppamento.
- Nel tuo codice: `math-pratt/math-pratt.c:518-527`, `:109-110`.
- Errori/lezioni: nella prima versione nessuno consumava `)`, quindi il parser si fermava su `)` e ignorava `* 3` (turno 85).
- Turni: 85.

### Bug del loop infinito con gli spazi (`(1 + 2) * 3`)
- Cos'è: in `advanceLexer` c'era `char c = p[0]; while (isspace(c)) p++;`. La condizione controlla una copia che non cambia mai, quindi con uno spazio il ciclo è infinito. `(1+2)*3` funzionava perché non entrava mai nel ciclo.
- Nel tuo codice: versione corretta in `math-pratt/math-pratt.c:188-192`: prima salta gli spazi leggendo `ctx->lexer.p[0]`, poi salva `c`.
- Lezione: confrontare un input con e senza spazi ha isolato il bug subito. Gemini, al primo giro, aveva detto che il codice "dovrebbe funzionare" (turno 86).
- Turni: 86, 87.

### evalAST: interprete tree-walk (DFS post-order)
- Cos'è: per calcolare un nodo servono prima i valori dei figli, quindi l'ordine è sinistro → destro → nodo (post-order). Il valore "sale" dalle foglie alla radice.
- Nel tuo codice: `math-pratt/math-pratt.c:582-617`, con il controllo della divisione per zero in `:597-602`. In chord-pratt `evalAST` "valuta" l'albero appiattendolo: `listPushObj(a, b)` + `retain(b)` (`chord-pratt/chord-pratt.c:671-692`).
- Errori/lezioni: per dichiarare variabili subito dopo un `case` servono le `{ }` (`case INFIX: { int a = ...; }`). Prima di C23 una dichiarazione dopo un'etichetta non è ammessa: Apple clang 17 dà il warning "label followed by a declaration is a C23 extension", compilatori più vecchi danno errore. Non è gestito `INT_MIN / -1`, che va in SIGFPE come la divisione per zero (verificato: `-2147483648/-1` → exit 136). L'overflow di `int` è comportamento indefinito.
- Turni: 88.

### Attraversare un albero: DFS (pre/in/post-order) e BFS
- Cos'è: la DFS scende in profondità (ricorsione o stack). **Pre-order** N-L-R serve a copiare o stampare l'albero e dà la notazione prefissa. **In-order** L-N-R ordina un BST e dà la notazione infissa. **Post-order** L-R-N serve a valutare e a liberare memoria e dà la notazione **postfissa/RPN** (`1 2 +`). La **BFS** (per livelli) usa una coda.
- Nel tuo codice: `evalAST` e `freeObject` sono post-order (`math-pratt/math-pratt.c:228-243`, `:582`). `printTree` è pre-order. `printXML` è "ibrido": apre il tag in pre-order, scrive `<op>` in in-order, chiude il tag in post-order (`:377-421`). La stampa RPN e la BFS non sono state scritte.
- Turni: 88, 89, 90, 91.

### Stampa ad albero tipo `tree`: snprintf, buffer sullo stack, backtracking
- Cos'è: un prefisso di stringa (`"│   "` o `"    "`) che cresce scendendo, più il flag `isTail` per scegliere tra `├──` e `└──`.
- Nel tuo codice: `math-pratt/math-pratt.c:284-337` e `:339-368`. Un solo `char buffer[1024]` in `printTree` (`:359`), passato per puntatore. Ogni livello accoda con `memcpy` (`:317-320`) e alla fine ripristina con `sharedBuf[originalLen] = '\0'` (`:336`).
- Decisioni: la prima versione aveva `char newPrefix[1024]` + `snprintf` in ogni frame (1 KB di stack per livello, turno 96). Poi buffer condiviso + backtracking (turno 97), poi `memcpy` al posto di `snprintf` (turno 98). Con `memcpy` devi gestire tu limite e `'\0'` (copi `appendLen + 1`). `"│   "` è lungo 6 byte (UTF-8), non 4: per questo si usa `strlen`.
- Errori/lezioni: il buffer da 1024 byte limita la profondità stampabile. Verificato: `1+1+...+1` con 200 termini esce con "Errore: Buffer dell'albero troppo piccolo!" prima ancora di valutare. Con 150 termini funziona.
- Turni: 90, 96, 97, 98.

### Serializzazione XML e JSON
- Cos'è: trasformare una struttura in memoria (nodi sparsi collegati da puntatori) in un flusso lineare di byte, ricostruibile poi con la deserializzazione. In JSON l'insidia sono le virgole: va messa solo tra `left` e `right`, non dopo l'ultimo campo.
- Nel tuo codice: `math-pratt/math-pratt.c:423-480` (`printJSON`), `:377-421` (`printXML`). In chord-pratt `printXML` gestisce `song/line/word/chord/lyric` (`chord-pratt/chord-pratt.c:582-669`).
- Decisioni: per una presentazione a frontender, XML ricorda il DOM e JSON ricorda ESTree/Babel. Proposta: mostrarli affiancati (turno 93).
- Turni: 91, 92, 93.

### Scrivere su disco un albero serializzato
- Cos'è: due strade. (1) Uno "string builder": buffer + `snprintf` a offset + `realloc` quando è pieno, poi una `fwrite`. `memcpy` da solo non basta perché un `int` va convertito in testo. (2) La strada idiomatica: la funzione riceve un `FILE *out` e usa `fprintf`. Con `stdout` va al terminale, con `fopen("ast.json","w")` su disco. Il buffering lo fa la libreria stdio.
- Nel tuo codice: non implementato. `printJSON` scrive ancora su `stdout` con `printf`.
- Turni: 99.

### Pratt vs recursive descent; statement vs espressioni
- Cos'è: i parser reali sono ibridi. Gli statement (`if`, `while`) usano il recursive descent guidato da parole chiave. Le espressioni usano un algoritmo a precedenze (Pratt, shunting-yard, precedence climbing). Il Pratt è comunque un recursive descent "top-down operator precedence": un solo ciclo guidato dalla tabella invece di una funzione per ogni livello di precedenza.
- Turni: 94, 95.

### Lexer come macchina a stati finiti (FSM)
- Cos'è: stati Q, input Σ, transizioni δ, stato iniziale q0, stati finali F. Nel Chord lexer lo stato ricorda "cosa è appena successo" e permette di **iniettare token fantasma**: `SONG` e `LINE` iniziali, l'operatore `*` tra le parole, accordo/testo vuoti.
- Nel tuo codice: `enum LexerState` in `chord-pratt/chord-pratt.c:95-106`, il grande `switch` in `advanceLexer` a `:290-381`, stato iniziale `STATE_START_SONG` in `:388`. La transizione dipende anche dal tipo del token precedente (`:350`, `:358`): quindi non è una FSM "pura" sui soli caratteri.
- Errori/lezioni: a fine file (`c == 0`, `:366-369`) il lexer emette sempre `*` + accordo vuoto + testo vuoto. Il risultato è che **ogni canzone che non finisce con `\n` ha una parola vuota in fondo**. Un file che finisce con `\n` ha invece una riga in più. Verificato: stress_test con 50.000 righe → 50.001 LINE.
- Turni: 104.

### Il confine tra lexer e parser è una scelta
- Cos'è: "fat lexer" (il lexer produce già `TOKEN_CHORD` e i token fantasma, il parser diventa banale e parallelizzabile) contro "fat parser" (il lexer emette `[`, stringa, `]` e il parser fa `parseChord()`). Esempi correlati: lo scannerless parsing e il "lexer hack" del C per i `typedef`.
- Nel tuo codice: `readChord` in `chord-pratt/chord-pratt.c:201-230` produce un token CHORD intero (fat lexer).
- Turni: 105.

### Stringhe dei token: malloc, stack, zero-copy; ownership di `result`
- Cos'è: (1) un buffer locale in `readChord` restituito per puntatore è un dangling pointer. (2) Un buffer fisso dentro il `Token` impone una lunghezza massima. (3) **Zero-copy / string view**: il file intero resta in memoria, quindi token e nodi possono essere `{ptr, len}` dentro `buf` e si stampano con `%.*s`.
- Nel tuo codice: c'è ancora la **doppia allocazione**. `readLyric`/`readChord` fanno `xmalloc` (`chord-pratt/chord-pratt.c:190`, `:224`), `advanceLexer` libera (`:291-295`) e `createLyricObject`/`createChordObject` ricopiano (`:474`, `:488`).
- Ownership (turno 111): `result = evalAST(ctx.ast)` è solo un puntatore che *osserva* un nodo già posseduto dall'albero. `retain(result)`/`release(result)` servono solo se `result` deve sopravvivere a `release(ctx.ast)`. Nel `main` attuale sono stati tolti (`:740-748`). Il fatto che il puntatore stia sullo stack non c'entra: l'oggetto puntato è sull'heap.
- Turni: 110, 111.

### Stack overflow con alberi profondi
- Cos'è: gli operatori associativi a sinistra producono un albero con una lunga "spina" a sinistra. Il *parsing* resta iterativo (è il `while` del Pratt), ma ogni funzione ricorsiva sull'albero (`evalAST`, `freeObject`/`release`, `printXML`) fa una chiamata per ogni riga. Lo stack del thread principale su macOS è di 8 MB (`ulimit -s` = 8192).
- Nel tuo codice: nella versione single-thread `evalAST` (`chord-pratt/chord-pratt.c:680-686`) e `freeObject` (`:398-401`) sono ancora ricorsive. Verificato: 100.000 righe OK, 200.000 → SIGSEGV dentro `evalAST`. Con `ulimit -s 65520` funzionano anche 200k e 600k. Gli errori `Parse error: bad token/bad expression` venivano da `bc` nello script, che riceveva un tempo vuoto perché il programma era crashato.
- Alternative: aumentare lo stack (`-Wl,-stack_size,0x2000000`) sposta solo il limite. La soluzione vera è eliminare la ricorsione profonda.
- Turni: 128, 134, 135.

### DFS iterativa con stack esplicito
- Cos'è: si sostituisce lo stack delle chiamate con un array sull'heap. Con la spina sinistra: (1) scendi `current = current->infix.left` e fai *push* di ogni `infix.right`; (2) valuti la foglia in fondo (l'accumulatore, cioè la SONG); (3) fai *pop* in ordine LIFO, valuti ogni figlio destro e lo aggiungi all'accumulatore. Poi è venuto il conteggio preliminare per pre-allocare stack e `acc->list.ele` con una sola `xmalloc`.
- Nel tuo codice: solo in `chord-pratt/chord-pratt-threads.c:737-770` (il file single-thread è rimasto ricorsivo). `eval` (`:714-735`) resta ricorsiva sui figli destri, ma la profondità è pari alle parole di una riga, non alle righe.
- Errori/lezioni: (a) prima versione con `createSongObject()` usato come stack e mai rilasciato: leak (turno 130). (b) Primo tentativo di pre-allocazione con `PObj *stack = xmalloc(sizeof(PObj)*i)`, `stack[j] = right[0]` e `acc = xrealloc(acc, i)`: array di strutture invece che di puntatori, copia per valore e realloc dell'oggetto invece della sua lista (turno 132). (c) `if (count=0)` invece di `==`: azzera `count`, quindi `xmalloc(0)` e overflow sull'heap (turno 133; verificato: qui esce con "Unknown AST type"). Lezione: compila con `-Wall`, che segnala l'assegnamento usato come condizione (`-Wparentheses`).
- Turni: 129, 130, 131, 132, 133, 134.

### freeObject iterativa (eliminare la ricorsione in coda a sinistra)
- Cos'è: un ciclo `while (o)`. Per un INFIX: rilascia il figlio destro (corto), fa `free(o)`, decrementa a mano il refcount del sinistro e, se scende a 0, `o = left; continue;`. È una tail-call fatta a mano: lo stesso frame per tutta la spina.
- Nel tuo codice: `chord-pratt/chord-pratt-threads.c:361-...`. Nella versione single-thread `freeObject` è ancora ricorsiva (`chord-pratt/chord-pratt.c:396-422`).
- Verifica: con il codice del turno 135 (evalAST già iterativa) il crash a 200k avviene in `freeObject` (lldb: `EXC_BAD_ACCESS` in `freeObject`). Era quindi la vera causa rimasta.
- Turni: 135.

## Possibili errori o imprecisioni di Gemini
- **Turno 79**: dice che lo "standard architetturale" è mettere `rulesTable` in fondo al file e che i prototipi `struct PObj *parsePrefix(pCtx *ctx)` potrebbero dare warning con `-Wpedantic`. È falso: `pCtx` e `struct pCtx` sono lo stesso tipo e il consiglio ha causato l'errore di compilazione del turno 80, che Gemini poi ammette.
- **Turno 84**: chiama "hack brutto" l'`advanceLexer` condizionale in `parseExpression`. L'asimmetria però nasceva dal file completo che Gemini stesso aveva generato al turno 81 (prefix che non avanza + `advanceLexer` dopo la prefix).
- **Turno 84**: "gli operatori unari hanno precedenza altissima" e l'esempio `-2^3`. Per convenzione `-2^2 = -4` (Python: `-2**2 == -4`), quindi l'unario va sotto la potenza. "Più alta di tutto" non è una regola universale.
- **Turno 86**: analizza il codice con il bug `while (isspace(c))` e conclude che `(1 + 2) * 3` "dovrebbe funzionare perfettamente". Il bug era nel codice incollato; l'ha trovato solo dopo il tuo indizio (turno 87).
- **Turno 88**: spiega le `{ }` dopo `case` con la "visibilità nel case successivo". In C (prima di C23) la ragione è grammaticale: un'etichetta deve essere seguita da uno statement e una dichiarazione non lo è (verificato: Apple clang 17 dà solo il warning `-Wc23-extensions`; compilatori più vecchi danno errore). Inoltre non segnala `INT_MIN / -1`, che va in SIGFPE come la divisione per zero.
- **Turno 95**: **inventa** la storia del tuo Chord Parser (`parseRootNote`, `maj7`, `sus4`...). Il tuo parser tratta righe `[accordo]testo`, non la notazione degli accordi. Lo hai fatto notare tu al turno 102.
- **Turno 97**: "senza allocare un grosso array a ogni passo la funzione vola". Un array sullo stack costa solo lo spostamento dello stack pointer; il costo reale era la copia del prefisso con `snprintf`. Anche la versione condivisa fa `strlen` (O(profondità)) a ogni nodo.
- **Turni 75/82/99**: "malloc chiede memoria al sistema operativo" e "il sistema operativo gestisce i buffer di `FILE*`" sono imprecisioni. `malloc` è un allocatore in user space che solo ogni tanto chiama il kernel; il buffering di `FILE*` lo fa la libreria C (stdio).
- **Turno 128**: dice che `echo "0.13 + " | bc` produce "bad token". Su bc 6.5.0 dà "bad expression". "bad token" nasce all'iterazione dopo, quando `SUM` è vuoto (`" + 0.1"`). La causa di fondo (crash del programma) era giusta, ma non menziona che anche `freeObject` sarebbe andata in overflow.
- **Turno 130**: "i nodi passati a `eval` sono solo foglie, zero ricorsione" e "il test a 200.000 righe non fa più paura". In realtà i figli destri sono catene INFIX di `*` (le parole della riga), quindi `eval` ricorre ancora. Soprattutto, `freeObject` era ancora ricorsiva e a 200k crashava (turni 134-135).
- **Turno 131**: diagnostica il blocco come "heap thrashing" da `realloc` quadratica, fino allo swap su disco. È implausibile: la versione ricorsiva faceva la stessa sequenza di `listPushObj` ed era veloce. Ho ricompilato il codice del turno 130 e su `stress_test.txt` (50k righe) termina in 0,36 s senza blocchi. La causa reale del blocco non è mai stata identificata.
- **Turno 134**: attribuisce il crash a `while (stack->list.len)` / `listPopObj(stack)` su un `PObj **`. Quel codice **non c'era**: la tua versione del turno 133 usava già `j`. La causa vera era `freeObject` ricorsiva (turno 135, confermato con lldb).
- **Turno 133**: diagnosi corretta (`count=0`), ma il sintomo descritto ("`left` punta a se stesso, loop infinito") è una congettura. L'effetto dipende dalla disposizione dell'heap; qui il programma esce con "Unknown AST type".

## Domande aperte / cose non chiarite
- Perché il benchmark del turno 131 si bloccava con 50.000 righe? Il codice del turno 130 qui non si blocca. Forse era un binario `a.out` vecchio o un file diverso: da verificare.
- Come gestire gli errori nel Pratt parser? Oggi `parseExpression` ritorna `NULL`, nessuno lo controlla e `evalAST(NULL)` va in segfault. Nessun controllo che dopo il parsing si sia arrivati a EOF (`1 2` → 1, `1+2)` → 3).
- L'arena allocator e lo zero-copy delle stringhe sono stati discussi ma non provati: quanto cambierebbero i tempi del chord-pratt?
- Deserializzare il JSON prodotto (scrivere il parser inverso) non è stato affrontato.
- Come far stampare in modo iterativo `printXML`/`printTree` per alberi molto profondi? Anche l'indentazione `printIndent` è O(profondità) per riga, quindi l'output è quadratico.

## Argomenti a margine
- Arena allocator, teoria e bozza in C: turno 76. Un'unica malloc, allocazione a offset, una sola free.
- XML o JSON per una presentazione a frontender: turno 93. XML richiama il DOM, JSON richiama ESTree/Babel.
- Lexer hack del C e scannerless parsing: turno 105. Il parser comunica i `typedef` al lexer.
- Messaggi d'errore di `bc` nello script di benchmark: turni 128, 134. "bad token/bad expression" quando l'input è vuoto o comincia con `+`.

## Domande di autoverifica
1. Perché un `TOKEN_NUMBER` non ha bisogno di una precedenza "vera" nella tabella? Cosa succede nel `while` di `parseExpression` quando incontra un token con precedenza 0?
2. Che differenza c'è tra la funzione prefix e la funzione infix di `TOKEN_MINUS`? Come fa il parser a sapere quale chiamare senza guardare il token precedente?
3. Perché `Token t = ctx->lexer.curToken; t.type = TOKEN_PLUS;` non funziona? Scrivi due modi corretti.
4. Perché la tabella `rulesTable` non può essere un array locale di `createContext` restituito tramite puntatore?
5. Nel bug di `(1 + 2) * 3`, perché `(1+2)*3` funzionava?
6. Perché nell'AST non esiste un nodo "parentesi"? Disegna l'albero di `(1+2)*3` e quello di `1+2*3`.
7. Quale attraversamento usa `evalAST`? E `freeObject`? Quale ti serve per ottenere `1 2 3 * +` da `1+2*3`?
8. In `printXML`, quali righe corrispondono a pre-order, in-order e post-order?
9. Nella versione con buffer condiviso di `printAST_recursive`, cosa succede se togli `sharedBuf[originalLen] = '\0'`? Perché `appendLen` vale 6 per `"│   "`?
10. Con `PREFIX_PRECEDENCE 6`, che albero ottieni per `-2*3`? E con precedenza 2? Se aggiungessi `^` (precedenza 4, associativo a destra), dove metteresti l'unario e cosa cambieresti in `parseInfix` per l'associatività a destra?
11. Nel Chord lexer, quali token "fantasma" vengono iniettati e da quali stati? Perché una canzone senza `\n` finale ha una parola vuota in fondo?
12. Perché il parsing di 200.000 righe non va in stack overflow, mentre `evalAST` e `freeObject` ricorsive sì?
13. Descrivi i passi della DFS iterativa di `evalAST` sulla spina sinistra. Perché si usa `PObj **stack` e non `PObj *stack`?
14. Cosa succede, passo per passo, con `if (count=0)` in `evalAST`? Quale flag del compilatore te lo avrebbe segnalato?
15. In `freeObject` iterativa, perché bisogna decrementare a mano il refcount di `left` invece di chiamare `release(left)`?
