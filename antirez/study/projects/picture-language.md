# Picture Language (SICP 2.2.4) in TypeScript + Canvas  — stato: working
Path: `sicp/language-picture/` (index.html, script.ts → script.js, tsconfig.json, style.css, foto.jpeg) · Ultima modifica: 2026-09-24 (commit `pic_lang_14`; `script.ts` ha modifiche non committate)

## Cosa fa
Implementa il linguaggio a immagini di SICP 2.2.4 su un `<canvas>` 600×600: pair/liste alla SICP JS, vettori, segmenti, frame, painter (figura "George" a linee e painter da JPEG), trasformazioni (flip, rotate, shrink, squash), combinatori `beside`/`below` curryficati, `right_split`, `up_split`, `corner_split`, `square_split`, `square_of_four`, `square_limit`. In testa al file c'è la collezione di uccelli di Smullyan tipizzati in TS, più un frattale con ricorsione anonima via Mockingbird.

## Come si compila / si esegue   (comandi verificati da te)
Nel repo non c'è `node_modules` e lo shim `tsc` di mise non ha una versione impostata (`mise ERROR No version is set for shim: tsc`). Verificato con TypeScript 5.9.3 installato nella build dir:
```
BUILD=<scratchpad>/build/sicp
npm install --prefix $BUILD typescript@5
cd sicp/language-picture
$BUILD/node_modules/.bin/tsc -p tsconfig.json --outDir $BUILD/out   # verifica senza toccare script.js
$BUILD/node_modules/.bin/tsc -p tsconfig.json                        # (per te) rigenera script.js accanto a script.ts
```
Esecuzione: per i soli segmenti basta aprire `index.html` (file://). Per il painter da JPEG serve un server http (es. `python3 -m http.server` nella cartella, poi `http://localhost:8000/`) oppure togliere `img.crossOrigin` (vedi bug).

## Esito verifica
- `tsc -p tsconfig.json` (strict, noUnusedLocals, noImplicitReturns): **0 errori**, exit 0.
- `script.js` **non è allineato** con `script.ts`: il JS compilato oggi differisce in 3 punti: (1) contiene ancora il vecchio `C` commentato; (2) ha la vecchia `W` (`(x) => (y) => x(y)(y)`, stessa semantica); (3) **esegue** `WesternNic(beside)(below)(painter)(painter)(frame1)` (in `script.ts:907` ora è commentato). Quindi oggi la pagina mostra il disegno WesternNic, mentre ricompilando lo script attuale la pagina resterebbe **vuota** (tutte le chiamate finali sono commentate, `script.ts:793-961`).
- Headless Chrome 154 (screenshot nella build dir):
  - `script.js` del repo → `beside(below(p)(p))(p)`: due George a sinistra, uno grande a destra. OK.
  - `square_limit(painter, 2)(frame1)` → motivo alla Escher corretto.
  - `M(make_fractal)(painter)(4)(frame1)` → frattale, la ricorsione cresce in alto a destra (come notato al turno 311).
  - `L(flip_vert)(make_fractal)(frame1)` → canvas bianco, **nessun errore in console**, il valore restituito è una funzione (conferma la spiegazione del turno 319).
  - `square_limit(image_to_painter(ctx, img), 2)` con `loadImage("./foto.jpeg")`: da `file://` **fallisce** (`Access to image ... from origin 'null' has been blocked by CORS policy`); senza `crossOrigin` oppure servito via http funziona e l'immagine è dritta (fix dell'asse Y corretto).
- Prove di tipi (tsc, file di test nella build dir con gli esempi commentati riattivati): compilano R, T, F, L, D, B1, E, D2, S, VS, ZD, PSI, Phoenix, Pheasant, EasternNic, WesternNic, `C(beside)`, `B(B(flip_horiz)(flip_vert))(rotate90)`, `M(make_fractal)`, `W(below)`. **Non compila** `C(B)(rotate90)(flip_vert)` (riga commentata `script.ts:912`, errore TS2345: i generici di `B` diventano `unknown`). Compila invece, purtroppo, `L(flip_vert)(make_fractal)` perché `L` ha `y: any`.

### Combinatori: implementati vs usati davvero
| Combinatore | Definizione | Usato con i painter |
|---|---|---|
| B | `script.ts:43` | solo in commenti (`:914-915`) |
| K | `:49` | mai |
| C | `:55` | commenti (`:909-912`, Pheasant `:890`) |
| M (+ SelfApplicable) | `:64-66` | dentro `make_fractal` `:787`; `make_fractal` non viene mai invocato (`:793` commentato) |
| W | `:69` | **unico attivo**: `below_painter = W(below)(painter)` `:768` (calcolato ma mai disegnato) |
| R, T, F, L, D | `:75`, `:82`, `:88`, `:95`, `:101` | commenti `:797`, `:800`, `:802`, `:807`, `:814` |
| B1, E, D2 | `:109`, `:117`, `:126` | commenti `:873`, `:871`, `:869` |
| S, VS, ZD, PSI | `:135`, `:142`, `:149`, `:157` | commenti `:867`, `:865`, `:863`, `:861` |
| Phoenix, Pheasant | `:165`, `:173` | commenti `:881`, `:890` |
| EasternNic, WesternNic | `:183`, `:191` | commenti `:899`, `:907` (WesternNic attivo nel vecchio `script.js`) |
| Becard | solo commento `:197` | — |

## Cosa funziona
- Tutto il nucleo SICP 2.2.4: frame, `frame_coord_map`, `transform_painter`, trasformazioni, `beside`/`below` curryficati, split ricorsivi, `square_of_four`, `square_limit`.
- `image_to_painter` con `ctx.transform` e correzione dell'asse Y (`script.ts:467-496`), se servito via http.
- 21 combinatori di Smullyan (e simili) tipizzati con generici distribuiti per livello; tutti gli esempi con i painter compilano tranne `C(B)`.
- Ricorsione anonima tipizzata con `SelfApplicable<Step>` + `M`.

## Cosa manca / bug noti
- `script.js` non rigenerato dopo le ultime modifiche (vedi sopra): la pagina mostra una versione vecchia.
- Nessuna chiamata attiva in fondo a `script.ts` (`:793-961` tutto commentato): ricompilando, canvas vuoto.
- `loadImage` imposta `img.crossOrigin = "anonymous"` (`script.ts:500`): da `file://` l'immagine non si carica. Per disegnare e basta non serve.
- `L` usa `y: any` (`script.ts:97`): nasconde errori come `L(flip_vert)(make_fractal)` (turni 318-319). Con `y: SelfApplicable<R>` tsc segnala subito l'errore (verificato).
- `C(B)` (`:912`) non compila se scommentato: limite di TS con combinatori generici passati come argomento.
- Tipo `Pair` unico per numeri, vettori, segmenti e frame (`:249-252`): servono cast ovunque e il type checker non distingue un vettore da un frame.
- File "script" globale (nessun `import/export`): tutte le costanti (`B`, `C`, `T`, `S`, `E`…) sono globali; per questo `noUnusedLocals` non segnala i combinatori mai usati. Nel diff non committato il generico `B` dentro `W` (`:70`) oscura il Bluebird nella firma (innocuo, ma confonde).
- `segments_to_painter` ricalcola `frame_coord_map(frame)` due volte per segmento (`:461-462`).
- Non esistono `split` generico (es. 2.45), né painter `over`, né `rooster`/`escher` citati negli esempi di Gemini.

## Concetti applicati
- [Elementi di un linguaggio](../notes/sicp-combinatori.md#elementi-di-un-linguaggio-primitive-combinazione-astrazione)
- [pair/head/tail e liste](../notes/sicp-combinatori.md#pair--head--tail-e-liste-come-in-sicp-js)
- [Frame e frame_coord_map](../notes/sicp-combinatori.md#frame-e-frame_coord_map)
- [Painter come funzione](../notes/sicp-combinatori.md#painter-come-funzione-frame--disegno)
- [transform_painter](../notes/sicp-combinatori.md#transform_painter-e-le-trasformazioni-flip-rotate-shrink-squash)
- [beside/below e chiusura](../notes/sicp-combinatori.md#beside--below-e-la-propriet%C3%A0-di-chiusura)
- [image_to_painter e asse Y](../notes/sicp-combinatori.md#image_to_painter-e-lasse-y-capovolto-turno-282)
- [Currying e generici TS](../notes/sicp-combinatori.md#currying-in-typescript-e-posizione-dei-generici)
- [Combinatori implementati](../notes/sicp-combinatori.md#i-combinatori-di-smullyan-implementati-tipi-generici-ts)
- [Mockingbird e ricorsione anonima](../notes/sicp-combinatori.md#mockingbird-e-ricorsione-anonima-make_fractal-selfapplicable)

## Prossimo passo consigliato
1. Sistemare il flusso di build: riattivare una chiamata finale (es. `s_limit(frame1)`), ricompilare con `tsc -p .` e committare `script.ts` + `script.js` insieme (o aggiungere `tsc --watch`).
2. Togliere `img.crossOrigin` (o servire con `python3 -m http.server`) e riattivare `run()` con `square_limit(imgPainter, 2)`.
3. Tipizzare `L` con `SelfApplicable<R>` invece di `any` e rifare l'esperimento del turno 318: osservare che ora l'errore lo trova tsc.
