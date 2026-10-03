# Algebra e programmazione funzionale
Periodo: 2026-04-13 → 2026-08-28 · Turni: 38-50, 53, 103, 127, 283-288

## Mappa in breve
Implementando il chord parser con lo Shunting-yard ti sei accorto che la stack machine è un `Array.reduce` e che
le composizioni "accordo + lirica", "parola dopo parola", "riga dopo riga" si comportano come operazioni
algebriche. Da qui: monoidi (associatività + elemento neutro), sentinelle iniettate come elementi neutri, monoide
libero (liste e concatenazione), Algebraic Data Types (sum/product) e il motivo per cui l'associatività permette
di dividere il lavoro fra thread. Mesi dopo, con il picture language di SICP, sei tornato sugli stessi concetti
dal lato opposto: proprietà di chiusura, magma vs monoide vs gruppo, combinatori di ordine superiore e gli
elementi di un linguaggio. Collegamenti: [parsing](parsing.md) (dove questi concetti sono applicati), area thread
(MapReduce in C), area logica combinatoria (To Mock a Mockingbird).

## Concetti

### Reduce / fold: la stack machine come riduzione
- Cos'è: `reduce(f, init)` scorre una lista e accumula; l'accumulatore è lo stato. Valutare una postfix con uno
  stack è un `reduce` in cui l'accumulatore è lo stack. Come `5*2+3` si riduce a `13`, il testo con accordi si
  "riduce" all'AST (turno 53).
- Nel tuo codice: `chord-shunting-yard/chord-shunting-yard.c:683-730` (`evalPostfix`) scorre la coda e trasforma
  lo stack; `applyOperator :591-635` è la funzione reducer. Le sentinelle `SONG_TOKEN`/`LINE_TOKEN` (`:763-764`)
  sono l'equivalente dell'`initialValue`.
- Decisioni: per il talk due versioni TS (ciclo `for` con push/pop e `reduce` immutabile) (turni 51-52); stesso
  principio di Redux/`useReducer`.
- Errori/lezione: senza valore iniziale il reducer deve indovinare il tipo dell'accumulatore (WORD o LINE?) → tanti
  `if`; con il valore iniziale la firma diventa fissa.
- Turni: 38, 41, 51-53

### Monoide (associatività + elemento neutro)
- Cos'è: un insieme M con un'operazione binaria `M × M → M` (chiusura), associativa (`(a·b)·c = a·(b·c)`) e con un
  elemento neutro `e` (`a·e = e·a = a`). Esempi: (interi, +, 0), (stringhe, concat, ""), (array, concat, []).
- Nel tuo codice: i monoidi "veri" sono (LINE, concatenazione di parole, LINE vuota) e (SONG, concatenazione di
  righe, SONG vuota). Nel Pratt C l'hai sfruttato al massimo: un solo caso `INFIX` in `evalAST` senza `switch`
  sull'operatore perché `WORD_MULT` e `\n` fanno la stessa cosa (turno 103).
- Decisioni: hai riletto la tua tabella di precedenze come "equazione": `CHORD` prefisso (come `-1`), `WORD_MULT`
  prodotto, `\n` somma (turno 40).
- Errori/lezione: l'operatore che usi davvero, `listPushObj(line, word)` (`chord-shunting-yard.c:614`), **non è**
  l'operazione del monoide: è un *append* `LINE × WORD → LINE` (tipi diversi). L'operazione del monoide è
  `LINE ++ LINE`. La differenza esplode quando devi unire due SONG prodotte da due thread: fare push di una SONG
  dentro un'altra crea una matrioska; serve la concatenazione (turno 118).
- Turni: 39-40, 103, 118

### Monoide libero, chiusura e "evoluzione dei tipi"
- Cos'è: il monoide libero su un insieme A è l'insieme delle liste di A con la concatenazione (neutro `[]`).
  Un'operazione `WORD × WORD → LINE` viola la chiusura; la recuperi vedendo ogni WORD come lista di un solo
  elemento: `[w1] ++ [w2] = [w1, w2]`.
- Nel tuo codice: la tua formulazione del turno 103: `WORD_MULT: f([WORD],[WORD]) → [WORD]`,
  `ENDOFLINE: f([LINE],[LINE]) → [LINE]`. Il prefisso `CHORD(LYRIC) = WORD` non è un'operazione di monoide ma un
  costruttore (applicazione di funzione).
- Decisioni: il tuo parser è "una cascata di due monoidi liberi + un costruttore" (turno 40).
- Turni: 40, 103

### Token neutri / sentinelle (emptySong, emptyLine) vs valori di default (emptyChord, emptyLyric)
- Cos'è: iniettare in testa alla coda l'elemento neutro come accumulatore iniziale, così ogni operatore ha una firma
  fissa: `(CHORD, LYRIC) → WORD`, `(LINE, WORD) → LINE`, `(SONG, LINE) → SONG`, e alla fine sullo stack resta
  sempre una sola SONG.
- Nel tuo codice: `createEmptySongToken :222`, `createEmptyLineToken :229`, iniezione `:763-764` e dopo ogni `\n`
  `:821-823`; `\n` artificiale a fine file `:835-836`. `createEmptyChordToken :158` e `createEmptyLyricToken :168`
  invece **non sono elementi neutri** di un'operazione: sono il valore di default del campo mancante nel prodotto
  WORD (`aaa` diventa `WORD(chord "", lyric "aaa")`).
- Decisioni: iniezione nel ciclo `while` del main, non nel lexer (turno 42). `WORD_MULT` prima della parola
  (prefisso) perché l'accumulatore LINE è a sinistra (turno 45).
- Errori/lezione: con la LINE vuota in testa, `WORD_MULT` dopo la parola dava `LINE W1 W2 * *` e il primo `*`
  provava a unire due WORD → `FATAL ERROR: Expected LINE in WORD_MULT, got 2` (turni 44-45). La stringa
  letterale `"emptyChord"` passata a `free` (turni 36-37) → ora `xmalloc(1)` (`:162`).
- Turni: 41-47

### Algebraic Data Types (sum e product)
- Cos'è: *product type* = A **e** B (struct/tupla; numero di valori = |A|·|B|); *sum type* = A **oppure** B (union
  TS `A | B`, enum di Rust; |A|+|B|). In C il sum type si fa con la *tagged union*: un campo `type` + una `union`.
- Nel tuo codice: `CsObj` in `chord_sheet_parser/chord_sheet_parser.c:168-184` e
  `chord-shunting-yard.c:296-311` è un sum type (tag `type` + union `list`/`str`); WORD (accordo **e** lirica) è
  concettualmente un product type, anche se la implementi come lista di 2 elementi; il `Token` è un altro sum type
  (`chord-shunting-yard.c:46-60`). In TS l'equivalente è la union discriminata `{tag: "word", ...} | ...`.
- Decisioni: la "moltiplicazione" CHORD·LYRIC = WORD corrisponde all'istanziare un product type.
- Errori/lezione: una lista (LINE, SONG) non è un product type a N campi ma un tipo **ricorsivo**:
  `List<A> = Nil | Cons(A, List<A>)` (sum di product).
- Turni: 50

### Perché l'associatività permette il parallelismo
- Cos'è: se `·` è associativa puoi raggruppare come vuoi: `(A·B)·(C·D)` = `((A·B)·C)·D`. Quindi dividi l'input in
  pezzi, ogni thread riduce il suo pezzo, alla fine combini i risultati parziali (MapReduce).
- Nel tuo codice: Pratt C multithread (altra area, turni 112-126): due metà tagliate su `\n`, due AST, merge con
  concatenazione delle SONG. Misura: 0.134 s → 0.083 s su 100k righe (turno 126).
- Decisioni: il taglio va fatto su un `\n` vicino a metà (lo stato del parser lì è noto: inizio riga). Il costo di
  coordinamento (split, merge, creazione thread, in JS la serializzazione) limita lo speedup.
- Errori/lezione: non è la tecnica di parsing (Pratt vs RD) a rendere parallelizzabile il problema, ma la
  presenza di punti di taglio + un'operazione di merge associativa. Nota bonus: il merge dell'evaluator Pratt TS
  (`CHORD` → incolla l'ultima riga di sinistra con la prima di destra, turno 10) è anch'esso associativo e
  permetterebbe di tagliare anche a metà riga.
- Turni: 58, 103, 127

### Proprietà di chiusura (SICP, picture language)
- Cos'è: un'operazione di combinazione è chiusa se il risultato è dello stesso tipo degli argomenti, quindi può
  essere ricombinato senza limiti (`beside(p1, p2)` è ancora un Painter).
- Nel tuo codice: `sicp/language-picture/script.ts:642-663` (`beside`, curried: `beside(p1)(p2)`), `:687-707`
  (`below`), `right_split :709`, `corner_split :725` costruiti solo con combinazioni chiuse.
- Decisioni: lezione di SICP: primitive + combinatori chiusi + astrazione = gestione della complessità; vale anche
  per middleware/pipeline (turno 286).
- Turni: 283, 286

### Magma, monoide, gruppo nel picture language
- Cos'è: `(Painter, beside)` è solo un **magma** (operazione chiusa, nient'altro): non associativa
  (`beside(a, beside(b,c))` dà 50/25/25, l'altra 25/25/50) e senza neutro (un painter vuoto dimezza comunque lo
  spazio). `(Painter, over, painter vuoto)` sarebbe un monoide. Le simmetrie del quadrato con la composizione
  formano un **gruppo** (diedrale D4: 4 rotazioni + 4 riflessioni, ognuna con inversa).
- Nel tuo codice: `identity :577`, `flip_vert :580`, `flip_horiz :589`, `rotate90/180/270 :605-633`, tutte via
  `transform_painter :559`. Attenzione: le 6 trasformazioni che hai non formano un gruppo chiuso: mancano le due
  riflessioni diagonali (es. `flip_vert(rotate90(p))` = riflessione sull'antidiagonale).
- Turni: 283, 285

### Algebra di ordine superiore e combinatori
- Cos'è: salendo di livello gli elementi sono le **trasformazioni** `Painter → Painter` e gli operatori sono
  funzioni che prendono trasformazioni e ne restituiscono una (algebra di funzioni alla Backus).
- Nel tuo codice: `square_of_four :742-753` prende 4 trasformazioni e ritorna una trasformazione;
  `square_limit :754` lo usa con `flip_horiz, identity, rotate180, flip_vert`. Combinatori di Smullyan in TS:
  Mockingbird `M`, Warbler `W` (`script.ts:60-71`) usato in `W(below)(painter)` = `below(painter)(painter)`
  (`:768`).
- Turni: 284, 287

### Elementi di un linguaggio ed evaluator (shallow vs deep embedding)
- Cos'è: un linguaggio = elementi primitivi + mezzi di combinazione + mezzi di astrazione (SICP 1.1). Nel picture
  language l'evaluator non è una funzione separata: un Painter **è** una funzione `frame → disegno` (*shallow
  embedding*), valutare = chiamarla con un frame. Con un *deep embedding* il programma è un dato (AST) e serve un
  `eval` esplicito (SICP cap. 4).
- Nel tuo codice: picture language = shallow (`frame_coord_map :441`, `segments_to_painter :452`,
  `image_to_painter :467`). I tuoi parser producono invece un deep embedding: AST + evaluator (`evalPostfix`
  `chord-shunting-yard.c:683`, `exec` in `toyforth/toyforth.c:401`).
- Decisioni: nel tuo "linguaggio degli accordi" hai primitive (CHORD, LYRIC) e combinazioni (prefisso, `*`, `\n`)
  ma nessun mezzo di astrazione (es. dare un nome a un ritornello e riusarlo).
- Turni: 288

## Possibili errori o imprecisioni di Gemini
- **Turno 39**: presenta `emptyLyric`/`emptyChord` come elemento neutro del monoide e dice che con un monoide "il
  reduce non crasherà mai". Sono valori di default di un campo del product WORD, non neutri di un'operazione; gli
  elementi neutri sono LINE vuota e SONG vuota. Il "non crasherà mai" è un'esagerazione.
- **Turni 39 vs 40**: al 39 afferma che la chiusura vale ("sono tutti nodi dell'AST"), al 40 dice (correttamente)
  che `WORD × WORD → LINE` la viola.
- **Turno 50**: "WORD_MULT forma tipi prodotto più complessi (array di N elementi)": una lista è un tipo ricorsivo
  (sum di product), non un product. "CHORD e LYRIC sono due sum types isolati": confuso, sono varianti dello stesso
  sum type.
- **Turno 103**: indica gli stati `STATE_EMPTY_CHORD` del lexer come l'elemento neutro `e` con `x∘e = x` della
  concatenazione: no, il neutro della concatenazione è la lista vuota (LINE/SONG vuota).
- **Turno 103**: "nel RD il thread della seconda metà non sa in che stato è" e **turno 127** "RD non parallelizzabile,
  P-completo": falso per la tua grammatica (vedi [parsing](parsing.md)); conta il taglio su `\n` + merge
  associativo. Inoltre il Game of Life è parallelizzabile per cella; al 127 suggerisce di "spezzare alla cieca".
- **Turno 285**: "applicare `rotate90` e poi `flip_vert` equivale a `flip_horiz`": falso. Con le tue definizioni
  `flip_vert(rotate90(p))` mappa (x,y) → (1−y, 1−x), riflessione sull'antidiagonale. L'insieme {identity, rotate90,
  rotate180, rotate270, flip_vert, flip_horiz} non è chiuso: D4 ha 8 elementi (servono le 2 riflessioni diagonali).
- **Turno 287**: chiama Y "punto fisso di Turing": Y è il combinatore di punto fisso di Curry, quello di Turing è Θ.
  "Da S, K, M costruisci tutto": bastano S e K (M = SII). "Senza notazione algebrica": il libro usa proprio notazioni
  come `Bxyz = x(yz)` (le usi tu al turno 290). Il titolo italiano citato non è verificato.
- Corretto e utile: magma per `beside` e monoide per `over` (turno 283), shallow vs deep embedding (turno 288).

## Domande aperte / cose non chiarite
- Implementare davvero la concatenazione `SONG ++ SONG` come funzione (non come ciclo nel main) e verificarne
  l'associatività con un test.
- Il merge "incolla ultima riga + prima riga" dell'evaluator Pratt TS permette tagli a metà riga: formalizzarlo
  come monoide (qual è il neutro?).
- Aggiungere al picture language le due riflessioni diagonali e verificare la tabella di composizione di D4.
- Un "mezzo di astrazione" per il linguaggio degli accordi (nominare sezioni, ripetizioni) come cambierebbe la
  grammatica?

## Argomenti a margine
- Teoria delle categorie citata come "monoidi = teoria delle categorie" — turno 103 — nome grosso per un concetto di
  algebra astratta.
- Generics in C: macro `LIST_PUSH` con `sizeof(*(l)->list.ele)` vs `void *` — turno 48 — per unificare
  `listPush`/`listPushObj` (`chord-shunting-yard.c:378, 394`).
- Pratt in C, architettura senza codice (NUD/LED, tabella di regole) — turno 49 — prosegue nell'area Pratt/alberi.
- To Mock a Mockingbird come introduzione alla logica combinatoria — turno 287.

## Domande di autoverifica
1. Dai la definizione di monoide e tre esempi presi da JavaScript.
2. Perché la valutazione di una postfix con uno stack è un `reduce`? Chi è l'accumulatore, chi è l'initialValue?
3. Nel chord parser, quali sono i due monoidi e i loro elementi neutri?
4. `emptyChord` è un elemento neutro? Di quale operazione? Motiva.
5. Che differenza c'è fra `push(LINE, WORD)` e `LINE ++ LINE`? Quale delle due serve per unire due SONG di due thread?
6. Cosa significa "chiusura" e perché `WORD × WORD → LINE` la viola? Come la recupera il monoide libero?
7. Spiega la struttura di `CsObj` in termini di sum e product type. Che tipo è una lista?
8. Perché senza le sentinelle `applyOperator` aveva bisogno di tanti `if` sui tipi?
9. Perché iniettare `WORD_MULT` dopo la parola produceva `Expected LINE in WORD_MULT, got 2`?
10. Perché l'associatività permette di dividere il parsing fra thread? Dove devi tagliare il testo e perché?
11. `(Painter, beside)` è un monoide? Mostra con le percentuali di larghezza perché non è associativo.
12. Che struttura formano rotazioni e riflessioni con la composizione? Quali elementi mancano nel tuo `script.ts`?
13. Qual è il tipo di `square_of_four`? Perché è un'operazione "di ordine superiore"?
14. Shallow vs deep embedding: dove sta l'evaluator nel picture language e dove nel tuo chord parser?
15. Quali sono gli elementi di un linguaggio secondo SICP e cosa manca al tuo linguaggio degli accordi?
