# dsa-c — guida per agenti

Repo di **studio personale** di Stefano (sviluppatore frontend TypeScript/React che impara il C, le strutture dati,
SICP e la logica combinatoria). Non è software di produzione: il codice è scritto per imparare, spesso a tappe.
Scrivi in italiano.

## Mappa

- `antirez/` — **il cuore del repo** (dal 2026): progetti in C nati seguendo antirez e una lunga conversazione con
  Gemini. Parser (`toyforth`, `chord_sheet_parser`, `*-shunting-yard`, `*-pratt`), thread (`chord-pratt/chord-pratt-threads.c`),
  networking e player audio HLS (`networking/client_http.c` → binario `underground_radio`, versione ad attori in
  `networking/actor/`), `snake`, `game_of_life`, esercizi sparsi nella root di `antirez/`.
  - `antirez/sicp/` — SICP in TypeScript: `language-picture/` (picture language + combinatori di Smullyan),
    `streams/` (esercizi a tappe sugli stream, vedi sotto).
  - `antirez/study/` — **mappa della conoscenza e sito di studio** (pubblicato su Netlify, vedi sotto).
- Fuori da `antirez/` (2025, più vecchi e semplici): algoritmi e strutture dati di base (`binary-search`, `*-sort`,
  `dynamic-array`, `queue`, `stack`, `singly-linked-list`, `two-crystal-balls`), esercizi da `beej/` (Beej's Guide),
  `cs50/`, `fcamuso/` (memoria, endianness). `build/` è output di CMake.

## Prima di esplorare: leggi `antirez/study/`

Non serve leggere tutto il codice per orientarti: è già stato fatto e verificato (ottobre 2026).
- `antirez/study/README.md` — aree di studio, **stato di ogni progetto** (broken/working/done) con il prossimo passo.
- `antirez/study/notes/<area>.md` — concetti con riferimenti `file:riga`, bug noti, domande aperte. Le note sono state
  riviste contro il codice: sono la fonte affidabile.
- `antirez/study/projects/<id>.md` — scheda per progetto: come si compila, esito della verifica, bug, prossimo passo.
- `antirez/study/data/<area>.json` — gli stessi contenuti in forma strutturata (li legge il sito).
- `antirez/study/turns/NNN.md` + `index.json` — la conversazione con Gemini divisa per turno (336 turni);
  `turns/corrections.json` — 466 correzioni verificate alle risposte di Gemini.

## Come lavorare con Stefano

- **Modalità tutor.** L'obiettivo è che *lui* impari. Negli esercizi (es. `antirez/sicp/streams/src/stream.ts`) **non
  scrivere le soluzioni**: rivedi il suo codice, esegui i test, spiega cosa non va con domande guida, poi fagli domande
  per verificare che abbia capito il concetto e non solo fatto passare i test.
- **Non fidarti delle risposte di Gemini** citate nel repo: molte contengono errori (vedi `turns/corrections.json` e le
  sezioni "errori di Gemini" delle note). Verifica sul codice o con un test.
- **Commit**: messaggi in italiano, **senza** trailer `Co-Authored-By` o altre attribuzioni a Claude. Push su `master`
  solo se richiesto. Non riscrivere la storia senza chiedere.
- Non modificare i sorgenti dei progetti per "sistemarli" se non è richiesto: i bug noti sono materiale di studio.

## Build ed esecuzione

- macOS, Intel x86_64 (in passato anche un M1 con Homebrew in `/opt/homebrew` e un PC Kubuntu): Homebrew in
  `/usr/local` con ffmpeg, openssl@3, sdl2/sdl3, pkg-config. Usa `pkg-config --cflags --libs ...` invece dei path fissi.
- C: `cc -W -Wall -g file.c -o /tmp/qualcosa` — **compila fuori dal repo** (gli `a.out` nelle cartelle sono vecchi).
  Leak: `leaks --atExit -- ./prog`. Stack: 8 MB nel main, 512 KB nei pthread secondari.
- Player audio: `cd antirez/networking && make` (Makefile con pkg-config); si collega a Radio 24 (HLS), usare un timeout.
- Stream SICP: `cd antirez/sicp/streams && node --test test/01-basi.test.ts` (Node 24 esegue i `.ts`, niente
  `npm install`); tipi: `npm run typecheck`. Percorso a 7 tappe in `antirez/sicp/streams/README.md`.
- Picture language: `tsc -p antirez/sicp/language-picture` → `script.js`, poi un server http statico.

## Sito di studio (`antirez/study/`)

SPA statica vanilla JS, nessuna build: `cd antirez/study && python3 -m http.server 8000`. Netlify pubblica
`antirez/study` a ogni push (`netlify.toml` nella root). Dettagli in `antirez/study/SITE.md`.
Dopo aver aggiunto file al repo: `sh antirez/study/tools/update-repo-files.sh` (serve ai link al codice).
Se cambia l'export Gemini: `python3 antirez/study/tools/split_turns.py`.

## Trappole

- `.gitignore` è una **whitelist**: ignora tutto (`*`) e riammette le cartelle e i file con un punto nel nome
  (`!*.*`, più `Makefile`). Un file senza estensione non viene tracciato; una cartella `node_modules/` invece sì: non crearne.
- `antirez/Gemini-Reference Counting e Ownership in C-*.md` è l'export completo (3,5 MB, quasi tutto codice
  ripetuto): non leggerlo intero, usa `antirez/study/turns/`. `gemini-chat-chord-parser.md` nella root è un export
  più vecchio e parziale della stessa conversazione.
- Nelle note, "turno N" si riferisce alla numerazione di `antirez/study/turns/` (un turno = una domanda + risposta).
