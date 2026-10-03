# Tecniche di parsing
Periodo: 2026-03-14 → 2026-05-06 · Turni: 3-23, 27-36, 41-47, 51-73, 94-95, 100-109

## Mappa in breve
Partendo dal Tokenizer del Jack Compiler (Nand2Tetris) e dal parser di toyforth, hai scritto lo **stesso problema
(chord sheet `[C]testo`) con tre tecniche**: Recursive Descent in C (`chord_sheet_parser`), Pratt in TypeScript
(repo `myguitarsongs-repl`, turni 9-11) e Shunting-yard in C (`chord-shunting-yard`), più una calcolatrice
Shunting-yard (`math-shunting-yard`). Da qui sono nate: la mappa "AST finale vs AST + evaluator", il confronto fra
le tecniche (statement vs espressioni), l'error recovery, il confine lexer/parser e l'idea del talk per frontend
developer (regex → parser → React). Collegamenti: la gestione della memoria (refcount, ownership) sta nell'area
memoria; il Pratt in C (`math-pratt`, `chord-pratt`), gli alberi e i thread stanno in altre aree; la lettura
"testo = equazione" (monoidi, reduce, associatività) sta in [algebra-fp](algebra-fp.md).

## Concetti

### Confine lexer / parser
- Cos'è: il lexer trasforma caratteri in token (`advance()` = leggi il prossimo token), il parser trasforma token
  in struttura. Il confine **non è fisso**: puoi avere un lexer "magro" (token `[`, `STR`, `]` e `parseChord()` nel
  parser) o "grasso" (il lexer legge `[C]` e produce già un `CHORD_TOKEN`, o addirittura inietta token invisibili).
- Nel tuo codice: lexer magro in `chord_sheet_parser/chord_sheet_parser.c:460` (`advanceLexer` produce
  `OPENPAREN`/`STR`/`CLOSEPAREN`) + `parseChord` a `:513`; lexer grasso in
  `chord-shunting-yard/chord-shunting-yard.c:128` (`readChord` consuma `[`, testo e `]`). In toyforth
  (`toyforth/toyforth.c:281`) lexer e parser sono fusi perché il linguaggio è fatto di parole separate da spazi.
- Decisioni: lexer separato quando servono posizione riga/colonna per gli errori, commenti, grammatiche ricorsive;
  fusione accettabile per linguaggi minimali (turno 3). Nel chord-pratt C hai spinto la logica in una FSM nel lexer
  per semplificare il parser (turni 102, 105): trade-off "fat lexer vs fat parser".
- Errori/lezione: un lexer grasso rende più difficili messaggi d'errore precisi (in `readChord` l'errore è un
  `exit(1)`, `chord-shunting-yard.c:131,143`).
- Turni: 3, 102, 105

### Recursive Descent (grammatica + una funzione per regola)
- Cos'è: scrivi la grammatica (`song: line+`, `line: word+ ('\n'|'\0')`, `word: chord lyric`,
  `chord: '[' stringConstant ']'`) e per ogni regola una funzione `parseX()` che consuma token con `eatToken`.
  Top-down, il contesto è dato da *dove sei nello stack di chiamate*.
- Nel tuo codice: grammatica nel commento `chord_sheet_parser.c:96-118`; `parseSong :595`, `parseLine :584`,
  `parseWord :554`, `parseChord :513`, `parseLyric :537`; `eatToken :498`. Le ripetizioni (`+`) sono **cicli
  `while`** (`:586`, `:597`), non ricorsione: la profondità massima dello stack è 4-5 chiamate, qualunque sia la
  lunghezza del testo.
- Decisioni: con RD l'AST prodotto è già l'oggetto che il frontend vuole (song → line → word → chord/lyric),
  niente evaluator (turno 12). Gemini al turno 8 conferma RD come scelta giusta per documenti gerarchici (una
  tabella piatta token→funzione non sa se `STR` è accordo o testo; il contesto lo dà la chiamata).
- Errori/lezione: (1) in `advanceLexer` hai scritto `isStringConstant(l->p[0])` invece di `isStringConstant(l)`
  (`:462`, commit "chord v16"): con clang 17 **non compila** (errore `-Wint-conversion`); (2) nel ramo d'errore di
  `parseWord` (`:571-575`) la `WORD` creata a `:555` non viene rilasciata → leak di 32 byte (verificato con
  `leaks` su `test.txt`); (3) i caratteri non ASCII (`isascii`, `:392`) diventano `ILLEGAL`.
- Turni: 4, 5, 8, 19, 66-67, 94-95, 100

### Error recovery vs fail-fast
- Cos'è: *fail-fast* = al primo errore distruggi ciò che hai costruito (con il refcount basta `release` del padre)
  e propaghi un oggetto `ERROR` fino al `main`. *Error recovery* = metti un nodo `ERROR` nell'AST e continui
  (panic mode: scarti token fino a un punto di sincronizzazione, es. `;` in C, `\n` per te).
- Nel tuo codice: `chord_sheet_parser.c` è ibrido: `parseChord` ritorna un `ERROR` (`:514-530`) che `parseWord`
  inserisce nella `WORD` (`:557`) → la canzone viene comunque stampata con l'errore dentro (es. `[C` →
  `Error parsing object: 3...`). `eatToken` avanza anche in caso d'errore (`:501`): consuma il token sbagliato.
  `chord-shunting-yard` invece fa fail-fast brutale con `exit(1)` (`:131-132`, `:142-143`).
- Decisioni: volevi mostrare *tutti* gli errori nel frontend con tooltip (turno 5) → error recovery; Gemini mostra
  anche la versione fail-fast (turno 6). GCC/Clang, TS server e i browser (HTML, CSS) usano recovery; JS e JSON
  sono fail-fast (turni 7, 65, 71-72).
- Errori/lezione: nel fail-fast in C la regola è "se un figlio fallisce, rilascia il padre parziale prima di
  ritornare l'errore"; l'hai mancata proprio nel ramo `:571-575`.
- Turni: 4-7, 57, 65, 71-73

### AST finale vs AST + evaluation (la mappa mentale)
- Cos'è: tre architetture. (1) *Deserializzazione*: testo → parser → AST = dato finale (RD chord parser, JSON).
  (2) *AST + evaluator/executor*: l'AST è una rappresentazione intermedia da "valutare" (toyforth `exec`, Pratt
  chord TS con evaluator che appiattisce l'albero binario in `Line[]/Word[]`). (3) *Compilazione*: le funzioni di
  parsing emettono subito codice per una VM (Jack Compiler → `.vm`, syntax-directed translation, senza AST).
- Nel tuo codice: (1) `chord_sheet_parser.c:595`; (2) `toyforth/toyforth.c:401` (`exec`) e l'evaluator TS del
  turno 10 (`case "CHORD": lastElementLeft.value.concat(firstElementRight.value)`); lo stack machine di
  `chord-shunting-yard.c:683` (`evalPostfix`) costruisce l'AST valutando la postfix.
- Decisioni: React può consumare l'AST in due modi: dopo un evaluator che lo appiattisce (`array.map`) oppure con
  un componente ricorsivo `<AstNode>` (turni 18, 107).
- Turni: 12-14, 18, 107

### Pratt parsing (concetto) e parsing table
- Cos'è: ogni token ha una funzione *prefix/NUD* (se inizia un'espressione), una *infix/LED* (se sta in mezzo) e
  una precedenza (binding power). Il cuore è `parseExpression(prec)`: chiama la prefix, poi `while (prec <
  peekPrecedence) left = infix(left)`. È "Top Down Operator Precedence": una forma di discesa ricorsiva.
- Nel tuo codice: Pratt TS (turno 10): `LOWEST=1, SUM=2, PRODUCT=3`, `CHORD→PRODUCT`, `ENDOFLINE→SUM`;
  `registerPrefix/registerInfix` in mappe token→funzione; la prefix `parseWordExpression` fa lookahead e
  "mangia" accordo + lirica in un colpo (ibrido RD/Pratt). Il `[C] [D]` funziona perché lo spazio è un `LYRIC`
  valido (turno 11).
- Decisioni / precisazione importante: la tabella di toyforth (`toyforth/toyforth.c:43-47`, `callSymbol :388`)
  **non contiene funzioni di parsing**: associa un simbolo a una callback di *esecuzione* (dispatch table
  dell'interprete). La parsing table con function pointer prefix/infix è tipica del Pratt (l'hai fatta nel
  Pratt in C, altra area). La stessa idea di dispatch table l'hai riusata nei tuoi shunting-yard
  (`math-shunting-yard.c:524-528`, `chord-shunting-yard.c:676-678`).
- Errori/lezione: con token che non sono veri operatori la logica prefix/infix si riempie di `if` sul
  `peekToken` (turno 10).
- Turni: 8-12, 49, 94

### Infix → postfix nel Jack Compiler
- Cos'è: `compileExpression` chiama `compileTerm`, poi per ogni operatore compila il termine destro e **solo dopo**
  emette l'operatore (`writeArithmetic`): traduzione guidata dalla sintassi da infix a postfix per una stack VM.
- Nel tuo codice: `compileEngine.ts` del Jack Compiler (TypeScript, incollato al turno 13, non in questo repo).
- Decisioni: Jack **non ha precedenza** (specifica del corso): `2+3*4` vale 20; per avere 14 servono le parentesi.
  Lo Shunting-yard invece gestisce la precedenza. Toyforth è il caso estremo: scrivi già in postfix.
- Turni: 13-16

### Shunting-yard (coda + stack, precedenza, parentesi, associatività)
- Cos'è: algoritmo di Dijkstra che trasforma infix in postfix in O(n) senza ricorsione né AST. Operandi → coda
  di output; operatori → stack; un operatore entrante fa uscire dallo stack quelli con precedenza `>=` (sinistra-
  associativi) o `>` (destra-associativi/unari); `(` fa da muro; `)` svuota fino alla `(` e le parentesi si
  scartano; a fine input si svuota lo stack. Poi una stack machine valuta la postfix.
- Nel tuo codice: `math-shunting-yard/math-shunting-yard.c:287-340` (`ctxStackPush`): `(` push `:289`, `)` svuota
  e fa `release` di entrambe `:294-308`, regola d'associatività `:320-329`; precedenze `:514-520` (`(`=0,
  `+ -`=1, `* /`=2, `~`=3); flush finale con warning sulle parentesi spaiate `:565-574`; valutazione
  `evalPostfix :391` + `basicMathFunctions :462`.
- Decisioni: precedenza 0 a `(` (turno 33, corretta, ma poi la `(` viene comunque gestita come caso a parte);
  coda e stack sono la **stessa struttura**: array dinamico con capacità che raddoppia (`listPush :138-146`),
  la coda non viene mai "consumata" ma iterata (turni 22-23). Ring buffer e linked list scartati.
- Errori/lezione: parentesi scartate senza `release` = leak (turno 33). Bug ancora presenti: `1/0` → SIGFPE
  (`:496-497`); lettere e `%` sono accettate da `isSymbolChar` (`:216-219`) ma non hanno funzione → NULL deref a
  `:399-400` (crash, exit 139); `2 +` e `2+3)` danno un risultato senza segnalare errore (`evalPostfix` ignora gli
  errori nel ramo `LIST`, `:403-407`).
- Turni: 14-16, 20, 22-23, 33, 35

### Meno unario
- Cos'è: `-` è binario solo se preceduto da numero o `)`; altrimenti è negazione. Si sostituisce con un simbolo
  interno (`~`) a precedenza alta, destra-associativo, che nella stack machine fa pop di **un** operando.
- Nel tuo codice: lookbehind in `parse` `math-shunting-yard.c:246-253`; `~` usa `>` invece di `>=` (`:325-326`);
  esecuzione `:463-474`. Test verificati: `-(2 + 3)` → `2 3 + ~` = -5, `--2` → 2, `2 * -3` → -6.
- Errori/lezione: prima versione senza `parser.p++` e senza `else if` → il `~` veniva sovrascritto da un nuovo
  `-` e perso (leak) (turno 35).
- Turni: 34-35

### Chord parser con Shunting-yard: operatori invisibili e sentinelle
- Cos'è: il testo diventa un'espressione: `CHORD_TOKEN` = operatore **prefisso** (prec 3, come il meno unario),
  `WORD_MULT` = moltiplicazione implicita fra parole (prec 2), `\n` = somma fra righe (prec 1). Gli operatori che
  non esistono nel testo vengono **iniettati** dal ciclo principale (emptyChord, emptyLyric, WORD_MULT, sentinelle
  SONG/LINE, `\n` artificiale a fine file).
- Nel tuo codice: `chord-shunting-yard.c:763-764` (sentinelle iniziali), `:769-784` (LYRIC: inietta `*` e chord
  vuoto), `:786-806` (CHORD: `*`, accordo, lirica o lirica vuota), `:808-825` (`\n`: push, flush immediato,
  nuova LINE), `:835-836` (`\n` artificiale), `applyOperator :591-635`. Il `CHORD_TOKEN` è sia operando sia
  operatore: `evalPostfix` lo pusha come oggetto e poi applica l'operatore (`:700-707`), per questo il primo pop
  è l'accordo e il secondo la lirica (`:594-595`).
- Decisioni: `WORD_MULT` iniettato **prima** di ogni parola (prefisso) perché la LINE vuota è l'accumulatore a
  sinistra (turni 44-45). Firme fisse: `(CHORD,LYRIC)→WORD`, `(LINE,WORD)→LINE`, `(SONG,LINE)→SONG`.
- Errori/lezione: `FATAL ERROR: Expected LINE in WORD_MULT, got 2` (turno 45) = numero/posizione degli operatori
  sbagliati; i safety check l'hanno reso visibile. Limiti attuali: non-ASCII perso (`isascii`, `:102`; `Perché`
  → `Perch` + parola vuota), CRLF produce righe vuote extra (`\r` e `\n` sono entrambi ENDOFLINE, `:252`), stampa
  di debug `Printing: %d` dimenticata (`:445`), se il file finisce con `\n` l'ultimo `\n` artificiale fallisce in
  silenzio (lo stack ha solo la SONG; errore ignorato a `:708-711`).
- Turni: 20, 36-37, 41-47

### Confronto fra le tecniche (statement vs espressioni, JSON)
- Cos'è: nei parser reali gli *statement* (`if`, `while`, `function`) si fanno con RD (iniziano con una keyword,
  LL(1)), le *espressioni* con precedence climbing/Pratt; Pratt è RD con un solo ciclo guidato dalle precedenze
  invece di una funzione per livello. JSON: RD puro (nessun operatore, nessuna precedenza; un Pratt senza infix
  degenera in RD).
- Nel tuo codice: il chord RD codifica la precedenza nella **gerarchia delle regole** (song=`\n` più debole, line =
  parole, word = accordo più forte): è la stessa informazione della tabella Pratt (`CHORD=3 > WORD_MULT=2 > \n=1`),
  espressa come livelli di grammatica. Questa è la risposta corretta alla domanda del turno 95.
- Decisioni: la tua intuizione iniziale (turno 66: RD più semplice e adatto ai testi, approccio "equazione" una
  forzatura intellettuale) era sostanzialmente giusta; Gemini l'ha ribaltata ai turni 66-67 con argomenti falsi
  (vedi sezione errori) e poi è tornato alla tua posizione al turno 102.
- Turni: 19-21, 66-67, 94-95, 100

### Finite State Machine
- Cos'è: stati finiti + funzione di transizione (stato, input) → (nuovo stato, azione); quintupla (Q, Σ, δ, q0, F).
- Nel tuo codice: il lexer del chord-pratt C con `STATE_START_SONG`, `STATE_EMPTY_CHORD`, ... (altra area, turni
  102, 104). Anche il lexer del browser per l'HTML è una FSM (turno 65).
- Decisioni: utile quando il significato dell'input dipende dalla "storia recente"; rischio di esplosione degli
  stati. Usi frontend: stati di un form/bottone (XState).
- Turni: 104-105

### Generatori JavaScript come state machine
- Cos'è: una `function*` è compilata come una macchina a stati (lo stato locale vive in un oggetto, `next()`
  riprende dallo `yield`). Pausa gratuita dell'algoritmo.
- Nel tuo codice: non ancora implementato.
- Decisioni: buona idea per un lexer lazy (`yield token`) e soprattutto per un parser "didattico" passo-passo per
  le animazioni del talk; sconsigliato nel parser di produzione (`yield*` a ogni ricorsione, overhead).
- Turni: 109

### Parser dei linguaggi del browser e Markdown
- Cos'è: HTML = tokenizer a stati + "stack of open elements" + error recovery estremo (chiusure implicite,
  adoption agency); il DOM è l'albero risultante. CSS = token + blocchi, recovery "scarta la dichiarazione e vai
  al `;`/`}`"; `calc()` è un mini parser di espressioni. JS = RD per statement + precedence per espressioni, lazy
  pre-parsing in V8, fail-fast (`SyntaxError`). Markdown (CommonMark) = due fasi: blocchi riga per riga, poi
  inline con uno stack di delimitatori per `*`/`_` e `[`.
- Nel tuo codice: analogie con il tuo stack di operatori e le sentinelle; nessun codice dedicato.
- Errori/lezione: diverse affermazioni di Gemini su CSS e Markdown sono esagerate (vedi sotto).
- Turni: 62-65, 71-73

### WebAssembly per il parser
- Cos'è: compili il C in `.wasm`; JS e C condividono la memoria lineare (un `ArrayBuffer`). Le stringhe JS vanno
  codificate (UTF-8) e copiate nella memoria Wasm; il C restituisce un puntatore (un intero). Per leggere l'AST: (a)
  `DataView` con gli offset della struct, (b) funzioni getter esportate (puntatori opachi), (c) serializzazione in
  JSON lato C. La memoria la libera chi l'ha allocata: esporti `release()`, non chiami `free` sulla radice.
- Nel tuo codice: layout `CsObj` in wasm32 = 16 byte (`refcount`+0, `type`+4, `ptr/ele`+8, `len`+12),
  `chord_sheet_parser.c:170-184`; enum `SONG=0..LYRIC=4` (`:168`). Hai scelto la via (a) per imparare
  allineamento e DataView (turno 30).
- Decisioni: componente React che legge direttamente dal buffer Wasm (zero-copy) possibile ma rischioso
  (`memory.grow` stacca il vecchio `ArrayBuffer`, `release` legato a unmount/StrictMode). `JSON.parse` nativo batte
  quasi sempre un parser Wasm se poi i dati devono diventare oggetti JS (turno 101).
- Turni: 27-32, 101

### Web worker e serializzazione
- Cos'è: i worker non condividono oggetti con il main thread: `postMessage` usa lo *structured clone* (copia
  profonda). Alternative: Transferable (`ArrayBuffer` trasferito in O(1)) e `SharedArrayBuffer` (richiede
  COOP/COEP e `Atomics`).
- Nel tuo codice: non ancora implementato (era nella roadmap del turno 108).
- Decisioni: il worker serve a non bloccare la UI, non a rendere il parsing più veloce; per testi piccoli il
  costo di clonazione supera il parsing. Demo proposta: canzone da 100k righe con spinner CSS. 2 worker su 50k
  righe: parsing più veloce ma la deserializzazione sul main thread resta sequenziale + merge finale (turno 61).
- Turni: 58-61, 108

### Idea del talk per frontend developer
- Cos'è: arco narrativo: la regex "facile" (`/\[([^\]]+)\]([^\[]*)/g` perde il testo prima del primo accordo) e il
  React "spaghetti" (split + `index % 2`), poi lexer/parser e AST, RD (grammatica → funzioni), "testo come
  equazione" (5*2+3 → 13 come testo → AST), Shunting-yard visivo, `reduce` in TS, componente React ricorsivo,
  error recovery con nodi errore, analogie HTML/CSS/JS/Markdown, demo web worker.
- Nel tuo codice: le librerie TS da preparare (RD, Pratt, Shunting-yard, calcolatrice, renderer React) non sono
  ancora nel repo.
- Decisioni: poco codice C sulle slide, due versioni TS (con e senza `reduce`), animazioni con Keynote Magic
  Move/PowerPoint Morph o una web app React con bottone "Step" (generatori). Priorità dichiarata (turno 108):
  diventare un programmatore migliore, il talk viene dopo.
- Turni: 17-21, 51-57, 62-67, 106-108

## Possibili errori o imprecisioni di Gemini
- **Turni 4-5**: aveva suggerito `if(isStringConstant(l))`; tu hai scritto `isStringConstant(l->p[0])` e nella
  review del turno 5 Gemini non se n'è accorto. Risultato: `chord_sheet_parser.c:462` oggi non compila con clang 17.
- **Turno 8**: presenta la tabella di toyforth come "table-driven parsing" ("quando risolve le precedenze si chiama
  Pratt"). In toyforth la tabella mappa simboli → callback di **esecuzione** (`toyforth.c:43-47, 388-398`), non
  funzioni di parsing.
- **Turno 28**: in "Abra cadabra, WebAssembly is Awesome!" le `a/A` sono 7, non 6.
- **Turno 31**: `wasmInstance.exports._free` è impreciso: con Emscripten il nome con underscore è quello
  dell'oggetto `Module` generato dal JS di supporto; gli export reali del modulo dipendono da toolchain/flag
  (`EXPORTED_FUNCTIONS`) e con `clang --target=wasm32` senza libc `free` non esiste proprio.
- **Turno 52**: lo sketch TS con `reduce` parte da `initialStack = [emptySong, emptyLine]` ma `ENDOFLINE` non
  rimette una LINE vuota: dalla seconda riga `WORD_MULT` farebbe pop della SONG. Nel tuo C le LINE sono token in
  coda (`chord-shunting-yard.c:764, 822`). Inoltre tratta `CHORD` come operando, mentre nel tuo design è un
  operatore prefisso.
- **Turno 58**: "creare un thread costa millisecondi": la creazione di un pthread costa tipicamente decine di
  microsecondi (resta vero che per una canzone non conviene).
- **Turni 62-63, 66**: Markdown inline "con regole di precedenza simili a Pratt/binding power": CommonMark usa uno
  stack di delimitatori con regole left/right-flanking, non precedenze; i blocchi non si chiudono con un
  "operatore `\n\n`" ma confrontando ogni riga con i blocchi aperti.
- **Turni 66-67 (importante)**: "RD fa esplodere il call stack", "left recursion", "Pratt è bottom-up", "l'approccio
  algebrico è l'unica soluzione scalabile". Falso: la profondità del RD dipende dalla profondità della grammatica,
  non dalla lunghezza del testo (i `+` sono cicli, `chord_sheet_parser.c:586, 597`); lo snippet mostrato
  (`parseLine` che richiama `parseLine` dopo la parola) è ricorsione **destra**, non sinistra; Pratt è "Top Down
  Operator Precedence" (lo dice Gemini stesso al turno 94). Contraddice i turni 8 e 102.
- **Turno 71**: "`calc()` non si può fare con RD per la left recursion": la grammatica CSS è
  `<calc-sum> = <calc-product> [ ['+'|'-'] <calc-product> ]*`, cioè EBNF con ripetizione, parsata con RD.
- **Turno 94**: RD per le espressioni presentato come "incubo, lentissimo, troppa memoria": è la tecnica standard
  (una funzione per livello di precedenza); Pratt/precedence climbing riduce le chiamate, ma non è un salto di
  complessità.
- **Turno 95**: risposta inventata (`parseRootNote`, `parseQuality`, ... "il vostro momento Eureka"): non hai mai
  scritto un parser della struttura interna degli accordi. L'hai corretto tu al turno 102.
- **Turno 102**: chiama "Lexer Hack" l'iniezione di token; il vero "lexer hack" del C (turno 105, corretto) è il
  feedback typedef dal parser al lexer.
- **Turni 103 e 127 (importante)**: "il RD è intrinsecamente sequenziale / P-completo, solo la versione Pratt è
  parallelizzabile". Falso: la parallelizzazione viene dal tagliare su `\n` (punto in cui lo stato del parser è
  noto: inizio riga) e dall'unire con un'operazione associativa (concatenazione delle SONG, turno 118). Il tuo RD
  può parsare ogni metà in una SONG e poi concatenare. Il parsing di grammatiche context-free non è P-completo in
  generale; il Game of Life è parallelo per cella in ogni generazione. Al turno 127 dice anche "spezzare alla
  cieca", in contraddizione con il turno 58 (bisogna tagliare su `\n`).
- **Turno 103**: "hai dimezzato i tempi": le tue misure (turno 126) danno 0.134 s → 0.083 s, speedup ≈ 1.6x.

## Domande aperte / cose non chiarite
- Il `chord_sheet_parser` deve fare error recovery o fail-fast? Il codice è a metà (errori dentro le WORD, ma
  `eatToken` consuma il token sbagliato e non c'è un punto di sincronizzazione esplicito su `\n`).
- Come rappresentare la posizione (riga/colonna) dell'errore nei token per il tooltip del frontend (mai
  implementato).
- Gestione UTF-8 nei lexer (`isascii` scarta lettere accentate: problema reale per testi italiani).
- Il Pratt TS (repo `myguitarsongs-repl`) non è in questo repo: non verificato qui.
- Le librerie TypeScript per il talk e la web app (textarea + rendering accordi) non sono ancora iniziate.

## Argomenti a margine
- Export della conversazione in .txt/.pdf/Docs — turni 68-70 — Gemini ha simulato file con codice Python e poi
  ha prodotto un riassunto spacciato per "trascrizione integrale".
- Software per animare le slide (Manim, Keynote Magic Move, Remotion, Figma Smart Animate) — turno 106.
- Tag HTML `<ruby>` per mettere l'accordo sopra la sillaba — turno 107.
- Legge di Amdahl e costo di coordinamento — turno 61.
- Lazy parsing di V8 — turno 72.

## Domande di autoverifica
1. Qual è la differenza fra lexer e parser? Perché toyforth può fonderli?
2. Scrivi la grammatica del chord sheet e indica quale funzione C corrisponde a ogni regola.
3. Perché `line: word+` si implementa con un `while` e non con una chiamata ricorsiva? Quanto cresce lo stack con
   una canzone di 10.000 righe?
4. Cosa fa `eatToken` quando il token non è quello atteso e che conseguenza ha sull'error recovery?
5. Fail-fast in C con refcount: cosa devi fare con l'oggetto padre prima di ritornare l'errore? Dove manca nel tuo
   `parseWord`?
6. Quando l'AST è già l'output finale e quando serve un evaluator? Dai un esempio per ciascun caso dai tuoi progetti.
7. Esegui a mano lo Shunting-yard su `(3 + 4) * 2 - 1` mostrando coda e stack a ogni passo.
8. Perché `(` può avere precedenza 0 e perché le parentesi non finiscono mai nella postfix?
9. Come distingui `-` binario da `-` unario? Perché il `~` usa `>` e non `>=`?
10. Che cosa contiene la tabella di toyforth e che cosa contiene una parsing table Pratt?
11. Nel chord shunting-yard, perché `WORD_MULT` va iniettato prima della parola e non dopo?
12. Perché nel tuo `evalPostfix` il primo pop dell'operatore CHORD restituisce l'accordo e non la lirica?
13. Statement vs espressioni: quale tecnica per ciascuno nei parser reali e perché? Come codifica la precedenza un
    parser RD?
14. Perché un parser Wasm difficilmente batte `JSON.parse` quando il risultato deve tornare a JavaScript?
15. Un web worker rende il parsing più veloce? Cosa misureresti in una demo per dimostrarne il vero vantaggio?
