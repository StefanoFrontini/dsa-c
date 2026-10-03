# Math Shunting-yard  — stato: working
Path: antirez/math-shunting-yard · Ultima modifica: 2026-04-03 (commit "sy14"; mtime file 2026-04-22, contenuto uguale a HEAD)

## Cosa fa
Calcolatrice di espressioni intere: parse del testo in una lista di oggetti (numeri, simboli), conversione infix →
postfix con lo Shunting-yard (coda + stack, precedenze, parentesi, meno unario `~`), valutazione della postfix con
una stack machine. Tabelle di precedenza e di funzioni registrate come in toyforth.

## Come si compila / si esegue
```
cc -W -Wall -g math-shunting-yard/math-shunting-yard.c -o $BUILD/msy
$BUILD/msy math-shunting-yard/test.txt          # test.txt = "-(2 + 3)"
leaks --atExit -- $BUILD/msy math-shunting-yard/test.txt
```

## Esito verifica
- Compilazione: 0 warning.
- `-(2 + 3)` → queue `2 3 + ~`, stack `-5`. 0 leaks.
- `3 + 4 * 2` → `3 4 2 * +` = 11; `(3 + 4) * 2` → `3 4 + 2 *` = 14; `1 - 2 - 3` = -4 (associatività sinistra ok);
  `--2` → `2 ~ ~` = 2; `2 * -3` → `2 3 ~ *` = -6; `-2 * 3` → `2 ~ 3 *` = -6; `10 / 3` = 3.
- `((2 + 3)` → 5 + `Warning: Mismatched parenthesis found!`; 0 leaks.
- `2 + 3)` → 5, **nessun warning** (la `)` senza `(` svuota lo stack e viene ignorata).
- `2 +` → stampa `2`, nessun errore segnalato; 0 leaks.
- `1 / 0` → **crash SIGFPE** (exit 136).
- `2 + a` e `7 % 2` → **segfault** (exit 139).
- Input vuoto → nessun risultato, 0 leaks.

## Cosa funziona
- Precedenze e associatività sinistra (`>=`) / destra per `~` (`>`), `math-shunting-yard.c:320-329`.
- Parentesi con `release` delle parentesi scartate (`:289-308`) e pulizia finale (`:565-574`): nessun leak.
- Meno unario con lookbehind sul token precedente (`:246-253`).
- Array dinamico con capacità che raddoppia, usato sia come stack sia come coda (`:138-156`).

## Cosa manca / bug noti
- `:496-497`: divisione per zero non controllata → SIGFPE.
- `:216-219`: `isSymbolChar` accetta lettere e `%`, che non hanno precedenza né funzione → `getPrecedence` stampa
  `Error` e ritorna -1, poi `evalPostfix` dereferenzia `entry == NULL` (`:399-400`) → crash.
- `:403-407`: nel ramo `LIST` l'esito di `evalPostfix` sugli elementi è ignorato → errori di sintassi (`2 +`)
  danno un risultato sbagliato senza messaggio. Nessun controllo che a fine valutazione resti esattamente 1 numero.
- `:294-308`: `)` senza `(` corrispondente non viene segnalata.
- `:212`: `atoi` senza controllo overflow; operazioni su `int` possono andare in overflow.
- `printf("symbol: ...")` / `printf("result: ...")` di debug durante la valutazione (`:398`, `:470`).
- `a.out` nella cartella del 3 aprile: binario vecchio, meglio compilare fuori dal repo.

## Concetti applicati
- [Shunting-yard](../notes/parsing.md#shunting-yard-coda--stack-precedenza-parentesi-associatività)
- [Meno unario](../notes/parsing.md#meno-unario)
- [Pratt parsing e parsing table / dispatch table](../notes/parsing.md#pratt-parsing-concetto-e-parsing-table)
- [Reduce / stack machine](../notes/algebra-fp.md#reduce--fold-la-stack-machine-come-riduzione)

## Prossimo passo consigliato
1. Togliere lettere e `%` da `isSymbolChar` (o aggiungere `%` alle tabelle) e controllare `entry != NULL` in
   `evalPostfix`.
2. Controllare il divisore a zero in `basicMathFunctions` e propagare `SY_ERR` fino al `main`.
3. Dopo `evalPostfix` verificare che lo stack contenga esattamente un NUMBER, altrimenti "espressione malformata".
