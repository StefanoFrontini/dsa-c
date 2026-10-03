# SICP (picture language) e logica combinatoria
Periodo: 2026-07-25 → 2026-10-01 · Turni: 270, 278-336 (271-277 sono a margine: web radio)

## Mappa in breve
- Parti da SICP 2.2.4 (il "picture language" di Henderson): un **painter** è una *funzione* `frame → disegno`; tutto il resto (flip, rotate, beside, below, square_limit) sono funzioni che prendono painter e restituiscono painter. È l'esempio da manuale di **proprietà di chiusura** e di linguaggio "embedded" (turni 278-288).
- Lo implementi in TypeScript + Canvas (`sicp/language-picture/script.ts`), con `pair/head/tail` e liste alla SICP JS, poi aggiungi un painter che disegna un JPEG (turni 280-282).
- Da lì passi a *To Mock a Mockingbird* (Smullyan): derivazioni algebriche con B (turni 290-296) e poi **ogni uccello diventa una funzione TS curryficata e tipizzata**, usata per comporre i painter (turni 297-331).
- Chiusura teorica: M e ricorsione anonima, Y/uccelli saggi, base S-K, Turing-completezza, uccello ideale ↔ halting/Gödel, eBPF (turni 307-310, 332-336).
- Collegamenti: la parte "algebra/chiusura/gruppi" (283-288) è trattata in dettaglio in `algebra-fp/...`; l'idea "C non ha closure → struct + puntatore a funzione + `void *` contesto" (278, 294) si ricollega a tutta l'area C (callback, event loop).
- Progetti: `picture-language` (working, molto ricco) e `sicp-streams` (stub, solo codice commentato).
- Capitoli toccati. **SICP (JS edition)**: 1.1 elementi della programmazione (288); 1.3 funzioni di ordine superiore; 2.1.3 cosa sono i dati / es. 2.4 (`pair.ts`); 2.2 chiusura, 2.2.1 liste; **2.2.4 picture language** (frame, painter, transform_painter, livelli di linguaggio; es. 2.44-2.51) + slide `sicp-lecture-8.pdf` (278); 3.5 stream (solo `streams.ts`, citato al 270); cap. 4 astrazione metalinguistica e `parse` di SICP JS (288-289); cap. 3-5 citati al 270. **To Mock a Mockingbird**: cap. "To Mock a Mockingbird" (M, composizione, K, L), "Birds Galore" (B, D, B1, E, B2, D1, D2, C, T, R, F, …), "Mockingbirds, Warblers and Starlings" (W, S), "A Gallery of Sage Birds" (uccelli saggi, 335), "The Master Forest" (base S-K, 336), "Is There an Ideal Bird?" + epilogo (332-333). Numerazione dei capitoli da verificare sul libro.

## Concetti

### Elementi di un linguaggio: primitive, combinazione, astrazione
- In breve: primitive = painter (`segments_to_painter` `sicp/language-picture/script.ts:452`, `image_to_painter` `:467`), combinazione = `beside`/`below` (`:642`, `:687`), astrazione = funzioni con nome (`right_split` `:709`, `square_limit` `:754`). L'"evaluator" è la chiamata stessa del painter col frame (shallow embedding); in C sarebbe naturale un deep embedding (AST + interprete, turno 278).
- Approfondimento (shallow vs deep embedding, confronto con i tuoi parser): vedi [algebra-fp → Elementi di un linguaggio ed evaluator](algebra-fp.md#elementi-di-un-linguaggio-ed-evaluator-shallow-vs-deep-embedding).
- Turni: 278, 286, 288, 289.

### pair / head / tail e liste come in SICP JS
- Cos'è: la coppia è il mattone di tutti i dati; liste = catene di pair terminate da `null`; vettori, segmenti e frame sono pair/liste.
- Nel tuo codice: tipo `Pair` `script.ts:249-252`, `pair` `:256`, `head`/`tail` `:264`/`:269`, `list` ricorsiva `:282-294`, `list_ref` `:321`, `len` `:332`, `append` `:336`, `map` `:345`, `for_each` `:354`; vettori `make_vector`…`scale_vect` `:371-399`; segmenti `:401-411`; `make_frame` come lista di 3 vettori `:413-427`. In `sicp/pair.ts:1-59` (tutto commentato) ci sono le versioni "procedurali" della coppia (SICP 2.1.3 / es. 2.4: `dispatch(m)` e coppia come funzione che riceve un selettore).
- Decisioni/limiti: un unico tipo `Pair` con `head: number | Pair | null` per tutto ⇒ servono cast (`as number`, `as Pair`) ovunque e il type checker non distingue vettore/segmento/frame. Alternativa più "TS": tipi distinti (`type Vec = {x:number;y:number}`, `type Frame = {origin; edge1; edge2}`) mantenendo i selettori come barriera di astrazione.
- Errori: in `pair.ts:53` (commentato) `head2` restituisce `p` invece di `a` → non compilerebbe.
- Turni: 280 (codice completo), 270.

### Frame e frame_coord_map
- Cos'è: un frame è (origine, edge1, edge2). `frame_coord_map(frame)` restituisce la funzione che porta un punto del quadrato unitario `(x,y)` in `origin + x·edge1 + y·edge2` (mappa affine).
- Nel tuo codice: `script.ts:441-450`. `frame1` `script.ts:553-557` usa origine `(0,600)`, edge2 `(0,-600)`: l'asse Y del canvas cresce verso il basso, così lo "giri" e ottieni il sistema cartesiano di SICP (y verso l'alto).
- Nota: `segments_to_painter` calcola `frame_coord_map(frame)` due volte per segmento (`:461-462`): si potrebbe calcolare una volta sola fuori dal `for_each`.
- Turni: 280, 282.

### Painter come funzione frame → disegno
- Cos'è: `type Painter = (frame: Pair) => void` (`script.ts:253`). Il painter non sa *dove* disegnerà: lo decide il frame che riceve. Per questo i combinatori funzionano identici con linee o con un JPEG.
- Nel tuo codice: `segments_to_painter` `:452-466` (George, punti `:508-549`), `image_to_painter` `:467-496`.
- Lezione: costruire un painter non disegna nulla; si disegna solo quando lo chiami con un frame (`s_limit(frame1)`). Nel file attuale tutte le chiamate finali sono commentate (`:793-961`), quindi la pagina è vuota (vedi scheda progetto).
- Turni: 280, 288, 297.

### transform_painter e le trasformazioni (flip, rotate, shrink, squash)
- Cos'è: `transform_painter(p, origin, corner1, corner2)` crea un nuovo painter che, dato un frame, calcola un sotto-frame (mappando i tre punti) e ci fa disegnare `p`. Tutte le trasformazioni sono solo scelte diverse dei 3 punti.
- Nel tuo codice: `script.ts:559-576`; `flip_vert` `:580`, `flip_horiz` `:589`, `shrink_to_upper_right` `:597`, `rotate90/180/270` `:605/:615/:625`, `squash_inwards` `:634`, `identity` `:577`.
- Mappe sul quadrato unitario (utile per verificare a mano): rotate90 `(x,y)→(1-y,x)`, flip_vert `(x,y)→(x,1-y)`, flip_horiz `(x,y)→(1-x,y)`, rotate180 `(x,y)→(1-x,1-y)`.
- Turni: 280, 285, 297.

### beside / below e la proprietà di chiusura
- In breve: `beside(p1)(p2)` e `below(p1)(p2)` restituiscono un `Painter` ⇒ si annidano all'infinito (SICP 2.2). Versione curryficata `script.ts:642-664` e `:687-707` (la vecchia a due argomenti è commentata `:665-686`); in `beside` hai anticipato `paint_left` al primo livello (turno 305: guadagno trascurabile, il punto è concettuale).
- La parte algebrica (magma vs monoide, `over`, gruppo diedrale delle trasformazioni) è in [algebra-fp → Proprietà di chiusura](algebra-fp.md#proprietà-di-chiusura-sicp-picture-language) e [Magma, monoide, gruppo](algebra-fp.md#magma-monoide-gruppo-nel-picture-language).
- Turni: 283-286, 302-305.

### right_split, up_split, corner_split, square_limit
- Cos'è: ricorsione su painter: `right_split(p,n) = beside(p)(below(smaller)(smaller))`; `corner_split` combina up e right; `square_limit` mette 4 copie di `corner_split` con `square_of_four`.
- Nel tuo codice: `script.ts:709-757`. Verificato in headless Chrome: `square_limit(painter,2)` disegna il classico motivo "alla Escher".
- Turni: 280, 284.

### square_of_four e funzioni di ordine superiore
- In breve: `square_of_four(tl,tr,bl,br)` (`script.ts:742-753`) prende 4 trasformazioni `Painter→Painter` e ne restituisce una; `square_limit` `:755` la usa con `(flip_horiz, identity, rotate180, flip_vert)`. È il ponte verso i combinatori: si combinano operazioni, non immagini.
- Vedi [algebra-fp → Algebra di ordine superiore e combinatori](algebra-fp.md#algebra-di-ordine-superiore-e-combinatori).
- Turni: 284.

### image_to_painter e l'asse Y capovolto (turno 282)
- Cos'è: per un JPEG non mappi punti uno a uno: imposti la matrice affine del canvas con `ctx.transform(e1x, e1y, e2x, e2y, ox, oy)` e disegni l'immagine nel quadrato `0,0,1,1`. Ma nelle immagini raster `y=0` è la **cima**, mentre nel frame SICP `y=0` è il **basso** ⇒ l'immagine usciva capovolta e tu compensavi con `flip_vert`.
- Nel tuo codice: `script.ts:480-487` (transform), correzione `:489-491` (`translate(0,1)` + `scale(1,-1)`), caricamento asincrono `loadImage` `:497-505`, uso in `run()` commentato `:951-961`.
- Errori incontrati: (1) immagine capovolta (282, risolto); (2) percorso assoluto `/Users/...` (281): il problema reale con `file://` è `img.crossOrigin = "anonymous"` (`:500`), vedi scheda progetto ed errori di Gemini.
- Turni: 280, 281, 282.

### Currying in TypeScript e posizione dei generici
- Cos'è: per usare i combinatori ogni funzione deve prendere un argomento alla volta. In TS conta *dove* dichiari i generici: un generico deve stare sul livello in cui compare per la prima volta un argomento da cui inferirlo, altrimenti alla prima applicazione parziale viene fissato a `unknown`.
- Nel tuo codice: `B` `script.ts:43-47` (`<U,V>` poi `<T>`), `R` `:75-79` (`<U>` poi `<T,V>`), `F` `:88-92` (un generico per livello).
- Verificato con tsc 5.9 (build dir): `B` con `<A,B,C>` tutti fuori ⇒ `Bb(rotate90)(flip_vert)` errore TS2345 (`unknown` non assegnabile a `Painter`); il Robin con tutti i generici fuori (la "Soluzione A consigliata" del turno 313) ⇒ errore. La tua forma è quella giusta.
- Limite emerso: `C(B)` (riga commentata `:912`) **non compila**: passando un combinatore generico a un altro, TS istanzia i generici interni a `unknown` (errore TS2345). Funziona se scrivi a mano la versione non generica o con annotazioni esplicite.
- Turni: 303, 304, 306, 313, 315, 316.

### I combinatori di Smullyan implementati (tipi generici TS)
- Cos'è: ogni uccello è una funzione curryficata pura definita da un'equazione (es. `Bxyz = x(yz)`).
- Nel tuo codice (`script.ts`): B `:43`, K `:49`, C `:55`, M `:66` (+ `SelfApplicable` `:64`), W `:69`, R `:75`, T `:82`, F `:88`, L `:95`, D `:101`, B1 `:109`, E `:117`, D2 `:126`, S `:135`, VS (Violet Starling) `:142`, ZD (Zebra Dove) `:149`, PSI `:157`, Phoenix `:165`, Pheasant `:173`, EasternNic `:183`, WesternNic `:191`. Becard solo come commento `:197`.
- Nota: Violet Starling, Zebra Dove, Psi, Phoenix, Pheasant e i Nicator **non sono nel libro di Smullyan** (per quanto mi risulta vengono dai cataloghi moderni di "combinator birds", es. quelli usati nella comunità APL/BQN; Phoenix = S' di Turner, Psi = `on` di Haskell). Gemini li chiama "fauna di Smullyan" (326) senza dirlo.
- Decisioni: in Phoenix hai generalizzato i tipi (`x: (R) => (U) => V`, `y` restituisce `R`) seguendo il suggerimento del turno 328. Nel diff non committato hai rinominato i generici di W in `<A, B>`: il generico `B` "oscura" la costante `B` (il Bluebird) solo dentro la firma, innocuo ma fonte di confusione.
- Turni: 304, 306, 307, 312-331.

### Derivazioni da B: B1, D, E, B2 (turni 290-296)
- Cos'è: esercizi di Smullyan (cap. "Birds Galore"): esprimere uccelli solo con B. Regola: l'applicazione associa a sinistra (`BBB` = `(BB)B`).
- Risultati verificati: `D = BB`; `B1 = BBB`; `E = BB1 = B(BBB)`; anche `E = B1BD = BBBB(BB)` (la tua seconda dimostrazione, corretta, 290-292); `B2 = EB = B(BBB)B` e `B2 = BB1B` (296). L'errore del turno 295: `B1Bxyzwv = B(xyz)wv = xyz(wv)`, non `Bx(yzw)v` ⇒ hai corretto passando per `B1(Bx)`.
- Nota: `B(BB)` = `BD` = D1 (Dickcissel, `D1xyzwv = xyz(wv)`), non D come dice Gemini (291).
- Turni: 290, 291, 292, 295, 296.

### Comporre i painter con B, C, T, W, M (turni 297-319)
- B = composizione: `B(flip_horiz)(flip_vert)` = rotate180 (verificato con le mappe: `(1-x,1-y)`); `B(B(flip_horiz)(flip_vert))(rotate90)` = rotate270 (`script.ts:914` commentato). Annidare a sinistra o a destra dà la stessa pipeline (associatività della composizione, 299-300).
- C scambia argomenti: `C(B)XY = BYX` (301: il Thrush non va bene); `C(beside)` mette il primo painter a destra (`:909-911`).
- Trasformare solo il 2° argomento di `beside`: è esattamente il **Dove** `D(beside)(p1)(flip_vert)(p2)` (verificato con tsc); la formula di Gemini del 302 è sbagliata.
- W duplica: `W(below)(painter)` = `below(p)(p)` — è l'unico combinatore usato davvero nel codice attivo (`script.ts:768`).
- Esempi (tutti commentati, tutti compilano con tsc tranne `C(B)`): R `:797`, T `:800`, F `:802`, L `:807`, D `:814`, PSI `:861`, ZD `:863`, VS `:865`, S `:867`, D2 `:869`, E `:871`, B1 `:873`, Phoenix `:881`, Pheasant `:890`, EasternNic `:899`, WesternNic `:907` (quest'ultimo è ancora attivo nel `script.js` vecchio).
- Turni: 297-302, 312, 314-316, 320-331.

### Mockingbird e ricorsione anonima (make_fractal, SelfApplicable)
- Cos'è: `Mx = xx`. Se una funzione riceve *se stessa* come parametro `self`, può richiamarsi con `self(self)` = `M(self)` senza avere un nome ⇒ ricorsione anonima.
- Nel tuo codice: `type SelfApplicable<R> = (x: SelfApplicable<R>) => R` `script.ts:64`, `M` `:66`, `type Step` `:778`, `make_fractal` `:779-791`; uso `M(make_fractal)(painter)(4)(frame1)` (commentato `:793-794`, verificato in headless Chrome: la parte ricorsiva cresce in alto a destra, come avevi notato al 311).
- Perché non va in loop in JS (eager): `M(make_fractal)` restituisce subito una closure; `M(self)` viene valutato solo dentro il corpo, quando `depth > 0`. Il caso base ferma la ricorsione.
- Errori: TS2571 "Object is of type 'unknown'" con `self` non tipizzato (310) ⇒ risolto tipizzando `self: SelfApplicable<Step>`. Bug del Lark (318-319): `L(flip_vert)(make_fractal)` passa uno `Step` a `flip_vert` che vuole un `Painter`; risultato: nessun errore, nessun disegno (verificato: la chiamata con `frame1` restituisce una funzione). Non è stato segnalato dal compilatore perché `L` ha `y: any` (`script.ts:97`). Con `y: SelfApplicable<R>` tsc dà errore subito (verificato).
- Turni: 307-311, 317-319.

### Combinatore Y vs M, uccelli saggi
- Cos'è: un uccello saggio θ soddisfa `θx = x(θx)` (punto fisso). Si costruisce da M: `BMLx = M(Lx) = Lx(Lx) = x(Lx(Lx))` ⇒ `BML` è saggio; anche `BM(CBM)` e `SLL`. Y di Curry è un esempio. In JS eager Y va in loop: serve Z (`x => f(v => x(x)(v))`), turno 308.
- Nel tuo codice: niente Y; `make_fractal` usa la tecnica "self-application" che è l'idea alla base di Y.
- Turni: 308, 335.

### Base S/K e Turing-completezza
- Cos'è: S e K bastano a generare tutti i combinatori (`I = SKK`, `M = SII`), quindi M non serve come primitiva. Iota (`ιx = xSK`) da solo basta (verificato: ιι = I, ι(ι(ιι)) = K, ι(ι(ι(ιι))) = S).
- Turni: 287, 324, 336.

### Uccello ideale, halting problem, Gödel; eBPF e cicli
- Cos'è: nel finale del libro non può esistere un uccello che "decida" una proprietà non banale di tutti gli altri: è l'analogo per i combinatori dell'indecidibilità di Church/Turing ed è costruito con la stessa diagonalizzazione del teorema di Gödel (stessa famiglia, enunciati diversi: Gödel parla di dimostrabilità, Turing/Church di decidibilità).
- Conseguenze pratiche (332): analizzatori statici e compilatori devono essere approssimati/conservativi; per avere garanzie forti si restringe il linguaggio. eBPF (334): niente ricorsione (call graph aciclico), cicli solo se il verifier ne dimostra il limite (kernel ≥ 5.3), stack 512 byte, tail call con limite ~33.
- Turni: 332, 333, 334.

## Possibili errori o imprecisioni di Gemini
- **285, 287** (rotate90+flip_vert ≠ flip_horiz, D4 ha 8 elementi; Y "di Turing", "S, K, M") sono già analizzati in [algebra-fp → errori di Gemini](algebra-fp.md#possibili-errori-o-imprecisioni-di-gemini).
- **291**: riduce `B(BB)xyzwv` a `((xy)(zw))v` e lo chiama Dove. Corretto: `B(BB) = BD`, `BDxyzwv = xyz(wv)` = Dickcissel D1 (la conclusione "≠ E" resta giusta).
- **302**: `B(B beside)(flip_vert) p1 p2 = beside(p1)(flip_vert(p2))` è sbagliato: si riduce a `beside((flip_vert p1)(p2))` (applica un painter a un painter; tsc dà errore). Quello giusto è il Dove: `D(beside)(p1)(flip_vert)(p2)`.
- **313**: consiglia "Soluzione A: tutti i generici sul primo livello" per il Robin. È il contrario (e contraddice il turno 304): con `R(painter)` TS fissa T e V a `unknown` e `R(painter)(beside)` non compila (verificato). La tua versione (`<U>` poi `<T,V>`) è corretta.
- **317**: propone come esempio proprio `L(flip_vert)(makeFractal)`, che non funziona (Step ≠ Painter) e suggerisce `y: any`, che nasconde l'errore.
- **318**: attribuisce il canvas bianco a TypeError o stack overflow. In realtà nessun errore: il risultato è una funzione mai chiamata (verificato in Chrome). La spiegazione giusta arriva solo al 319.
- **308**: l'esempio `doubleFlip = M((f) => (p) => f(f(p)))` non ha senso (f è la funzione stessa, non una trasformazione); `cornerSplit` costruito con Z è in realtà `right_split`; il commento "a destra mettiamo p sopra il livello ricorsivo" è invertito (lo hai corretto tu al 311).
- **281**: "il percorso assoluto viene bloccato dal browser per sicurezza": impreciso. Con la pagina aperta da `file://`, `/Users/...` diventa `file:///Users/...`; il vero ostacolo è `img.crossOrigin = "anonymous"` su `file://` (CORS). Con un dev server `http://` invece il percorso assoluto del disco semplicemente non esiste sul server (404). Inoltre il path era quello del vecchio Mac (`/Users/stefanofrontini/...`).
- **280**: `crossOrigin = "anonymous"` "per evitare problemi CORS": serve solo per immagini remote che vuoi rileggere (getImageData); per disegnare e basta non serve, e su `file://` fa fallire il caricamento.
- **335**: `Y = B(SM)(SM)` è falso (`B(SM)(SM)x = SM(SMx)`, un'applicazione parziale, non `x(Yx)`); uccelli saggi corretti: `BML`, `BM(CBM)`, `SLL`. Anche il dire che Smullyan "evita deliberatamente" il nome Y è un'ipotesi non verificata.
- **333**: "un sistema con S e K (o M e L) è Turing-completo": {S, K} sì, ma {M, L} non è una base completa: da M e L non si ottiene K (nessuno dei due scarta argomenti), quindi non genera tutti i combinatori. Attenzione però: "non genera K" non implica da solo "non è Turing-completo" (vedi turno 336 sul λI-calcolo); la conversazione non lo dimostra né in un senso né nell'altro. Inoltre "Turing ideò le macchine per tradurre Gödel" è una semplificazione (rispondeva all'Entscheidungsproblem di Hilbert).
- **336**: "senza cancellazione (solo B, C, W, I) il sistema non è Turing-completo": discutibile/sbagliato: il λI-calcolo di Church (che corrisponde ai combinatori B, C, W, I) definisce tutte le funzioni ricorsive; ciò che manca è la possibilità di scartare argomenti, non la potenza di calcolo.
- **332**: "Datalog elimina la ricorsione da Prolog": Datalog ha la ricorsione; elimina i simboli di funzione (termini composti), ed è per questo che termina. Gli smart contract (EVM) sono Turing-completi con limite di gas, non un esempio di linguaggio ristretto come eBPF.
- **326** e seguenti: presenta Violet Starling, Zebra Dove, Psi, Phoenix, Pheasant e Nicator come uccelli di Smullyan; nel libro non ci sono.

## Domande aperte / cose non chiarite
- Come scrivere in TS combinatori che accettano *altri combinatori generici* (`C(B)`) senza perdere i tipi? (limite dell'inferenza di TS sui generici di rango superiore: serve annotare a mano.)
- Il picture language in C (turno 278): AST + interprete o struct `{fn, void *ctx}`? Mai implementato: buon esercizio ponte tra le due aree.
- `script.js` va ricompilato ogni volta: manca uno script (`tsc -p . --watch`) e `index.html` non dà feedback se l'immagine non si carica.
- `rotate90` ripetuto 4 volte dà lo stesso painter? Matematicamente sì, ma in floating point i frame non sono identici al bit: rilevante se un giorno confronti painter.
- SICP 3.5 (stream) non è stato discusso nella conversazione: `streams.ts` è solo una bozza (vedi scheda `sicp-streams`).

## Argomenti a margine (non legati a progetti)
- Perché leggere SICP (astrazione metalinguistica, stato e tempo, interpreti) — turno 270.
- Radio Garden e directory di web radio (radio-browser.info, crowdsourcing, health-check) — turno 271.
- Legalità di un aggregatore di web radio (deep linking, niente ad-injection/registrazione/re-stream) — turni 272-274.
- Shoutcast/ICY e `icy-metaint` (metadati in-band) vs HLS — turni 275-276.
- Riprendere l'ascolto dopo una disconnessione: HLS con finestra DVR vs Shoutcast "live puro"; ring buffer — turno 277.
- Lisp/Racket (`#lang sicp`, `sicp-pict`) vs TS+Canvas per studiare SICP — turno 279.
- `parse` in SICP JS capitolo 4: fornita dall'ambiente (Source Academy), non è un esercizio; manca l'omoiconicità — turno 289.
- Combinatori in C: funzione + `void *` contesto, array di puntatori a funzione, macro, AST — turno 294.
- A cosa servono i combinatori nell'architettura (DSL, parser combinator, middleware, ragionamento equazionale) — turno 293.

## Domande di autoverifica
1. Che tipo ha un painter nel tuo codice e perché costruire `square_limit(painter, 2)` non disegna ancora nulla?
2. Cosa restituisce `frame_coord_map(frame1)` applicato a `(0.5, 0.5)`? E a `(0, 1)`? Perché `frame1` ha edge2 = `(0,-600)`?
3. Scrivi i tre punti da passare a `transform_painter` per ottenere `shrink_to_upper_right` e spiega perché bastano tre punti.
4. Perché l'immagine JPEG usciva capovolta mentre George no? Cosa fanno esattamente `translate(0,1)` e `scale(1,-1)`?
5. Perché `beside` non è associativo? Dai un esempio con le larghezze.
6. Calcola `B(flip_horiz)(flip_vert)` sulle coordinate `(x,y)`: quale trasformazione ottieni?
7. Calcola `flip_vert(rotate90(p))` sulle coordinate `(x,y)`: è una trasformazione che hai già nel codice?
8. Riduci `BBBB(BB)xyzwv` passo per passo. Quale uccello è?
9. Perché `B1Bxyzwv ≠ Bx(yzw)v`? Riduci entrambe le espressioni.
10. Come trasformi solo il *secondo* painter di `beside` usando un uccello? E solo il primo?
11. Perché `const B = <A,B,C>(f) => (g) => (x) => …` dà `unknown` e la tua versione no? Applica lo stesso ragionamento al Robin.
12. Perché `M(make_fractal)` non va in ricorsione infinita in JavaScript, mentre il combinatore Y scritto alla lettera sì?
13. Spiega perché `L(flip_vert)(make_fractal)(frame1)` non disegna nulla e non dà errori. Come si cambia il tipo di `L` per far trovare l'errore a TypeScript?
14. Dimostra che `BML` è un uccello saggio. Perché per avere la Turing-completezza non serve M come primitiva?
15. In che senso l'inesistenza dell'"uccello ideale" è parente del teorema di Gödel, e perché eBPF vieta la ricorsione?
