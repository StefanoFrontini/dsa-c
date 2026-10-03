# Study — mappa della conoscenza

Sistematizzazione della conversazione con Gemini (`../Gemini-Reference Counting e Ownership in C-20261003-1108.md`,
336 turni, 14/03/2026 → 01/10/2026) e audit dei progetti in `antirez/`. Generato il 03/10/2026.

- `notes/<area>.md` — concetti con riferimenti al codice, errori di Gemini verificati, domande aperte, autoverifica
- `projects/<progetto>.md` — scheda di stato: comandi verificati, esito, cosa manca, prossimo passo
- `data/<area>.json` — gli stessi contenuti in forma strutturata (fonte dati del sito, Fase 2)

I riferimenti "turno N" puntano ai turni della conversazione Gemini.

Revisione del 03/10/2026: ogni nota è stata riletta e le affermazioni principali confrontate con il codice;
le poche imprecisioni trovate (spiegazioni semplificate in modo scorretto) sono state corrette direttamente nel testo.

## Aree

| Area | Periodo | Turni | Note | Concetti |
|---|---|---|---|---|
| Memoria in C: ownership e reference counting | mar → giu | 1-2, 24-26, 37, 75-83, 96-99, 110-111, 209-212 | [memoria-c](notes/memoria-c.md) | 23 |
| Tecniche di parsing | mar → mag | 3-73, 94-95, 100-109 | [parsing](notes/parsing.md) | 16 |
| Algebra e programmazione funzionale | apr → ago | 38-53, 103, 127, 283-288 | [algebra-fp](notes/algebra-fp.md) | 10 |
| Pratt parsing in C e alberi (AST) | apr → mag | 74-99, 104-105, 128-135 | [pratt-alberi](notes/pratt-alberi.md) | 20 |
| Thread, parallelismo e benchmark | mag → lug | 112-137, 179, 214-217, 227-229 | [concorrenza](notes/concorrenza.md) | 15 |
| Networking: socket, HTTP, TLS, HLS | mag → giu | 137-203 | [networking](notes/networking.md) | 18 |
| Audio player: decodifica e riproduzione | giu → lug | 194-242 | [audio](notes/audio.md) | 18 |
| Actor model ed event loop | giu → lug | 220-221, 243-269 | [actor-model](notes/actor-model.md) | 20 |
| SICP (picture language) e logica combinatoria | lug → ott | 270-336 | [sicp-combinatori](notes/sicp-combinatori.md) | 17 |

Percorso: memoria/refcount (toyforth) → parsing (chord parser in 3 tecniche) → monoidi → Pratt e alberi →
MapReduce con i thread → networking e HLS → audio player multithread → actor model → SICP e combinatori → **stream (SICP 3.5)**.

## Stato dei progetti

| Progetto | Stato | Ultima modifica | Area | Prossimo passo |
|---|---|---|---|---|
| [audio-player-actor](projects/audio-player-actor.md) | 🔴 broken | 2026-07-17 | actor-model | Farlo compilare (9 errori), poi correggere il loop di `poll` (timeout in ms, fd duplicato, `read` da stdin, `ACTOR_NUM`) |
| [chord-sheet-parser](projects/chord-sheet-parser.md) | 🔴 broken | 2026-03-24 | parsing | `:462` → `isStringConstant(l)`; rilasciare la WORD nel ramo d'errore di `parseWord` |
| [sicp-streams](projects/sicp-streams.md) | ⚪ stub | 2026-09-21 | sicp-combinatori | `cons_stream` con coda ritardata e memoizzata, poi `stream_map`/`filter`, `integers`, crivello |
| [toyforth](projects/toyforth.md) | 🟡 in-progress | 2026-03-16 | memoria-c | `free(o->list.ele)` in `freeObject`, poi 0 leak con `leaks --atExit` |
| [audio-player](projects/audio-player.md) | 🟢 working | 2026-07-07 | audio | Parsing del numero di sequenza (rompe a ~10.000.000, circa marzo 2027); verifica del certificato TLS |
| [chord-pratt](projects/chord-pratt.md) | 🟢 working | 2026-05-27 | pratt-alberi | Portare `evalAST`/`freeObject` iterative (crash ≥200k righe); accettare UTF-8 (`città` → `citt`) |
| [chord-pratt-threads](projects/chord-pratt-threads.md) | 🟢 working | 2026-05-27 | concorrenza | `split` senza `\n` dopo la metà → segfault; test `diff` single vs multi |
| [math-pratt](projects/math-pratt.md) | 🟢 working | 2026-05-02 | pratt-alberi | Errore di sintassi se l'AST è NULL o restano token (`1+`, `()` → segfault) |
| [math-shunting-yard](projects/math-shunting-yard.md) | 🟢 working | 2026-04-03 | parsing | Divisione per zero, simboli sconosciuti → NULL deref |
| [chord-shunting-yard](projects/chord-shunting-yard.md) | 🟢 working | 2026-04-13 | parsing | `printf` di debug a `:445`, `\r\n` |
| [picture-language](projects/picture-language.md) | 🟢 working | 2026-09-24 | sicp-combinatori | `script.js` non allineato a `script.ts`; togliere `crossOrigin` per `file://` |
| [poll-input](projects/poll-input.md) | 🟢 working | 2026-07-15 | actor-model | Gestire EOF (`read` = 0), poi stdin non canonico |
| [daytime-client](projects/daytime-client.md) | 🟢 working | 2026-07-23 | networking | I wrapper devono uscire su errore; `SO_REUSEADDR` |
| [snake](projects/snake.md) | 🟢 working | 2026-02-23 | memoria-c | Indice `body[head-1]` con `head == 0` (`snake.c:524`) |
| [esercizi-c-base](projects/esercizi-c-base.md) | 🟢 working | 2026-02-13 | memoria-c | `hexdump.c` non compila; leak in `tac.c` |
| [echo-client](projects/echo-client.md) | ✅ done | 2026-06-07 | networking | `sendall`/`recvn` |
| [http-client](projects/http-client.md) | ✅ done | 2026-06-18 | networking | Tool `httpget` con test sul mock server |
| [thread-basics](projects/thread-basics.md) | ✅ done | 2026-07-05 | concorrenza | Produttore/consumatore su ring buffer + TSan |
| [game-of-life](projects/game-of-life.md) | ✅ done | 2025-07-14 | memoria-c | `cell_to_index` con modulo positivo |

Nota: il sorgente dell'audio player è `networking/client_http.c` (il Makefile produce `underground_radio`);
`client_http_2.c` e `client_http_3.c` sono tappe del client HTTP in chiaro.

## Temi trasversali emersi dall'audit

- **Ricorsione e stack**: lo stack del main è 8 MB, quello dei pthread secondari 512 KB (macOS). Le funzioni ricorsive
  su AST grandi (chord-pratt) e i buffer grandi sullo stack (player, turno 208) sono la stessa lezione.
- **UTF-8**: `isascii` nei lexer di chord-pratt e chord-shunting-yard tronca il testo alla prima lettera accentata.
- **Input malformati**: i parser di calcolatrice (math-pratt, math-shunting-yard) vanno in segfault invece di
  segnalare l'errore. Buon terreno per gli esercizi di error handling.
- **Gemini**: ogni nota ha una sezione "Possibili errori o imprecisioni di Gemini" verificata con codice o server
  reale (in totale ~130 voci). Le più importanti per il ripasso: "il recursive descent non è parallelizzabile" (falso),
  `emptyChord`/`emptyLyric` come elementi neutri (falso: lo sono LINE e SONG vuote), API SDL3 inesistenti,
  `Y = B(SM)(SM)` (falso), master playlist di Radio 24 con 3 varianti (inventata).
