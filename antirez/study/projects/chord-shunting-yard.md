# Chord Shunting-yard  — stato: working
Path: antirez/chord-shunting-yard · Ultima modifica: 2026-04-13 (commit "cs7")

## Cosa fa
Chord parser che tratta il testo come un'espressione: `CHORD_TOKEN` operatore prefisso (prec 3), `WORD_MULT`
moltiplicazione implicita iniettata (prec 2), `\n` somma fra righe (prec 1). Il ciclo principale inietta token
neutri (SONG/LINE vuote, chord/lyric vuoti, `\n` finale); lo Shunting-yard produce la postfix e una stack machine
(`evalPostfix` + `applyOperator`) costruisce l'AST SONG → LINE → WORD.

## Come si compila / si esegue
```
cc -W -Wall -g chord-shunting-yard/chord-shunting-yard.c -o $BUILD/chsy
$BUILD/chsy chord-shunting-yard/test.txt          # test.txt = "[C]\naaa"
leaks --atExit -- $BUILD/chsy chord-shunting-yard/test.txt
```

## Esito verifica
- Compilazione: 0 warning.
- `test.txt` (`[C]\naaa`) → `[C]` / `aaa` su due righe; 0 leaks.
- `Now I've [C]heard there was a\nsecret [G]chord` → corretto; 0 leaks.
- `[A]aaa[B]bbb\n[C]ccc[D]ddd` (il caso del turno 44) → corretto; 0 leaks.
- `aaa\n`, file vuoto, `a\n\nb` (riga vuota), `[C][G]x` → corretti; 0 leaks.
- `a\r\nb` (CRLF) → stampa una riga vuota in più (bug).
- `Perché [C]sì` → 4 righe `???5`, testo diventa `Perch` + ` ` + `[C]s`: i caratteri non ASCII vengono persi.
- `[C` → `Unmatched close paren`, `exit(1)`.
- In output compaiono righe di debug `Printing: 2` intercalate al testo.

## Cosa funziona
- Sentinelle e firme fisse degli operatori (`chord-shunting-yard.c:591-635`, `:763-764`, `:821-823`, `:835-836`).
- `CHORD_TOKEN` usato sia come operando sia come operatore (`:700-707`), associatività destra per il prefisso
  (`:547-548`).
- Liberazione completa: `releaseToken` gestisce `LIST_TOKEN` (`:83-88`), stringhe vuote allocate con
  `xmalloc(1)` (`:162`, `:172`, `:350`, `:364`): 0 leaks su tutti i casi provati.

## Cosa manca / bug noti
- `:102`: `isascii` → lettere accentate diventano `ILLEGAL` e vengono scartate (`:827-830`).
- `:252`: `\r` e `\n` sono entrambi ENDOFLINE → con file CRLF ogni a capo diventa due righe.
- `:131-132`, `:142-143`: errori di accordo gestiti con `exit(1)` (fail-fast senza liberare memoria né
  indicare la posizione); `printf("char: %c")` stampa anche il byte 0.
- `:708-711`: `evalPostfix` ignora gli errori dei figli; `applyOperator` in caso di safety check fallito non
  rimette sullo stack gli oggetti estratti (leak solo su flussi malformati). Con testo che finisce con `\n`
  l'ultimo `\n` artificiale fallisce in silenzio (funziona per caso).
- `:445`: `printf("Printing: %d\n")` di debug rimasto in `printCsObj`.
- `:853`: stampa solo `stack[0]`, senza verificare che lo stack contenga esattamente una SONG.
- Duplicazione `listPush`/`listPushObj` (`:378`, `:394`) discussa al turno 48 (macro `LIST_PUSH`), non applicata.

## Concetti applicati
- [Chord parser con Shunting-yard: operatori invisibili e sentinelle](../notes/parsing.md#chord-parser-con-shunting-yard-operatori-invisibili-e-sentinelle)
- [Shunting-yard](../notes/parsing.md#shunting-yard-coda--stack-precedenza-parentesi-associatività)
- [Token neutri / sentinelle](../notes/algebra-fp.md#token-neutri--sentinelle-emptysong-emptyline-vs-valori-di-default-emptychord-emptylyric)
- [Monoide](../notes/algebra-fp.md#monoide-associatività--elemento-neutro)
- [Reduce / fold](../notes/algebra-fp.md#reduce--fold-la-stack-machine-come-riduzione)

## Prossimo passo consigliato
1. Togliere il `printf("Printing: ...")` di debug (`:445`).
2. Trattare `\r\n` come un solo ENDOFLINE nel lexer (`:252`).
3. Sostituire `isascii(c)` con un controllo che accetti i byte ≥ 0x80 (UTF-8) come parte della lirica.
