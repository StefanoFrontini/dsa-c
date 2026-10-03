# Sito di studio

Sito statico (HTML + CSS + JS vanilla, ES modules) che legge a runtime le note, le schede e i dati di questa
cartella. Niente build, niente `node_modules`: si pubblica la cartella così com'è.

## Provarlo in locale

```
cd antirez/study
python3 -m http.server 8000
```

Poi apri <http://localhost:8000/>. Da `file://` non funziona: il browser blocca moduli JS e `fetch`
(la pagina lo spiega). Le librerie (`marked`, `highlight.js`, `KaTeX`) e i font arrivano da cdnjs / Google Fonts:
senza rete le note si vedono come testo semplice.

Formule: `$...$` e `$$...$$` fuori dal codice vengono rese con KaTeX (`assets/md.js`, `protectMath`), con le regole
di Pandoc per `$...$` (niente spazio dopo il `$` di apertura né prima di quello di chiusura, che non deve essere
seguito da una cifra), così i `$` della shell e dei prezzi restano testo. Le usano soprattutto le risposte di Gemini.

Pagine (routing hash, nessun redirect necessario):

- `#/` dashboard: percorso delle aree, "Da ripassare oggi", stato dei progetti, export/import dei progressi
- `#/area/<id>` nota dell'area con indice, "ripassato" per concetto e quiz di autoverifica
  (`#/area/<id>/<ancora>` porta a un titolo o a un concetto, es. `#/area/parsing/recursive-descent`)
- `#/project/<id>` scheda del progetto
- `#/errori` (o `#/errori/<area>`) tabella degli errori di Gemini, filtrabile
- `#/turni` indice dei turni della conversazione con Gemini (data, aree, inizio della domanda, stato della
  revisione), filtrabile; `#/turni/correzioni` solo i turni con correzioni; link "Conversazione con Gemini" nel footer
- `#/turno/<n>` testo originale di un turno: avviso "non rivisto" (o l'esito della revisione), riquadro
  "Correzioni a questo turno", "Citato in" (concetti ed errori dei JSON che citano il turno), turno
  precedente/successivo, "Indietro" (torna alla pagina di partenza, alla stessa altezza).
  `#/turno/<n>/corr-<k>` porta alla voce k del riquadro, `#/turno/<n>/corr-punto-<k>` al passaggio evidenziato

## Deploy su Netlify

1. Su Netlify: *Add new site → Import an existing project → GitHub*, scegli `StefanoFrontini/dsa-c`, branch `master`.
2. Le impostazioni sono già in `netlify.toml` nella root del repo: `publish = "antirez/study"`, nessun comando di build.
   Lascia vuoti "Build command" e "Publish directory" nell'interfaccia (vince il file).
3. Ogni push su `master` ripubblica il sito. Note, schede, turni e JSON hanno `Cache-Control: max-age=0, must-revalidate`
   (una correzione si vede al primo ricaricamento); `assets/` ha 5 minuti di cache.

## Dove vivono i dati

| Cosa | File | Note |
|---|---|---|
| Ordine del percorso | `assets/areas.js` | unico elenco hardcoded; anche URL del repo e soglie del ripasso |
| Note delle aree | `notes/<area>.md` | il `### ` dentro `## Concetti` diventa un concetto con checkbox |
| Schede progetto | `projects/<id>.md` | `# Titolo — stato: X` → badge |
| Dati strutturati | `data/<area>.json` | concetti, progetti, errori, `self_check` (se aggiungi `a` la risposta compare a scomparsa) |
| Elenco dei file del repo | `assets/repo-files.json` | rigeneralo con `tools/update-repo-files.sh` dopo aver aggiunto file |
| Turni della conversazione | `turns/NNN.md`, `turns/index.json` | generati dall'export, vedi sotto |
| Correzioni ai turni | `turns/corrections.json` | facoltativo, vedi "Correzioni ai turni" |

I concetti della nota vengono abbinati a quelli del JSON per titolo; se i titoli differiscono, per posizione
(funziona finché il numero di `###` coincide con `concepts`).

Link al codice: ogni `` `percorso:riga` `` (anche `:a-b`, `:a,b`, o senza riga) diventa un link a
`github.com/StefanoFrontini/dsa-c/blob/master/antirez/<percorso>#La-Lb`, nelle note, nelle schede e nei
`code_refs`. Con `assets/repo-files.json` i nomi senza cartella (`tac.c:44`) e i percorsi parziali
(`actor/poll_input.c`) vengono risolti sul file giusto, e i file che non esistono (segmenti `.ts` di esempio,
la cartella di build) restano testo; se un nome è ambiguo (`test.txt`) si sceglie il file della cartella del
progetto, altrimenti resta testo. `:86` senza file resta testo. `notes/x.md` e `projects/y.md` diventano link interni.

## Turni della conversazione (`turns/`)

L'export completo della conversazione con Gemini (`antirez/Gemini-*.md`) è troppo grande perché GitHub lo mostri,
quindi il sito lo serve diviso in turni: `turns/001.md` … `turns/336.md` (un file per turno: `## User:` con la
domanda e `## Gemini:` con la risposta, ognuno con la sua riga `> M/D/YYYY ora`) e `turns/index.json`
(`{source, turns: [{n, date: "AAAA-MM-GG", preview}]}`). Turno 1 = prima domanda, la stessa numerazione delle note.
Se l'export cambia, rigenerali dalla root del repo (cancella e riscrive tutti i `turns/*.md`):

```
python3 antirez/study/tools/split_turns.py
```

- Pagina del turno: la domanda è mostrata come testo semplice (a capo e rientri preservati, perché il codice
  incollato non ha i ```` ``` ````; oltre 40 righe parte compressa), la risposta passa per la stessa pipeline
  delle note (marked + highlight.js, titoli abbassati di un livello). I "Thinking steps" restano in blockquote.
  Nell'export i delimitatori dei blocchi di codice sono scritti `` `\u200B`\u200B` `` (spazi a larghezza zero fra i
  backtick): il sito li riporta a ```` ``` ```` prima del rendering, i file in `turns/` restano come sono.
- Link ai turni (`assets/turns.js`): nelle note, nelle schede, nelle risposte dei quiz e nei testi dei JSON le
  forme "turno 33", "(turni 44-45)", "**Turno 79**:", "Turni: 1-2, 24-26, 31", "turni 128 e 134", "Turni 39 vs 40",
  "Turni: 280 (codice completo), 270" diventano link (in un intervallo, entrambi gli estremi). Un numero senza la
  parola turno/turni resta testo, così come i numeri dentro `code`, blocchi di codice, link già esistenti e
  parentesi ("turno 136 (200k e 600k)"), e i numeri fuori da 1..N. Il tooltip mostra data e inizio della domanda.
- Dai JSON: la riga "Turni" in "Progetti, collegamenti e codice" sotto ogni concetto (`concepts[].turns`) e la
  colonna "Turno" della pagina errori (`gemini_errors[].turn`) sono link.
- Senza `turns/index.json` il sito funziona lo stesso: i link non hanno anteprima e `#/turni` mostra un errore.

## Correzioni ai turni (`turns/corrections.json`)

File facoltativo (codice in `assets/corrections.js`), letto come testo e analizzato con `JSON.parse` come `index.json`:

```
{ "generated": "AAAA-MM-GG",
  "turns": { "<n>": { "verdict": "corretto" | "ok" | "senza-risposta", "date"?: "AAAA-MM-GG",
                      "items"?: [ { "quote", "severity": "errore" | "imprecisione", "claim", "correction",
                                    "evidence"?, "concept"? } ] } } }
```

- Un turno presente nel file è "rivisto" (data: `date` del turno, altrimenti `generated`); senza voci l'avviso dice
  "nessun errore rilevante" (o, con `senza-risposta`, che non c'è una risposta da verificare). Un turno assente, o
  il file mancante, resta "Testo originale di Gemini, non rivisto" come prima; senza file l'indice non mostra
  indicatori né filtro, e la pagina errori non mostra il link ai turni con correzioni.
- Riquadro "Correzioni a questo turno": numero, badge (errore = rosso `--bad`, imprecisione = ambra `--warn`),
  "Gemini dice:" `claim`, "Corretto:" `correction`, "Verifica:" `evidence`; i tre testi sono markdown inline
  (`code`, **grassetto**, *corsivo*, link a codice e "turno N"). `concept` diventa un link se è l'id di un concetto
  dei `data/*.json`, altrimenti non compare.
- `quote` è il testo *renderizzato* della risposta (senza simboli markdown) e viene cercato solo nella risposta di
  Gemini, prima nel blocco più piccolo che lo contiene (`p`, `li`, `td`, `pre`, `blockquote`, titoli) e poi in tutta
  la risposta. Maiuscole, spazi (anche a capo) e spazi a larghezza zero non contano, le virgolette tipografiche
  valgono come quelle dritte; se non lo trova riprova confrontando solo lettere e cifre (punteggiatura diversa,
  `$D_4$` vs `D4`, citazione a cavallo di due blocchi). La citazione può attraversare grassetto, codice e link.
  Se non si trova, la voce resta nel riquadro con "passaggio non localizzato".
- Evidenziazione con la CSS Custom Highlight API (`::highlight(corr-errore)`, `corr-imprecisione`, `corr-active`
  per il passaggio appena raggiunto); senza API il testo viene avvolto in `<mark class="corr-mark">`. Alla fine del
  passaggio un marcatore numerato (fuori da link e codice inline) porta alla voce del riquadro; "vai al punto" fa il
  contrario, apre le eventuali parti compresse che contengono il passaggio e lo mette in risalto. Entrambi
  aggiornano l'indirizzo con `replaceState`, quindi "Indietro" continua a tornare alla pagina di partenza.

## Progressi (localStorage)

Un'unica chiave, `study.v1`:

```
{ v: 1, theme: null | "light" | "dark",
  reviewed: { "<id concetto>": "AAAA-MM-GG" },
  quiz: { "<area>:<indice>": { s: "so" | "incerto" | "non", d: "AAAA-MM-GG", q: "<testo domanda>", n: <volte> } } }
```

- Ogni accesso è in try/catch: senza localStorage (navigazione privata, cookie bloccati) il sito funziona con lo
  stato in memoria e lo segnala.
- I progressi restano nel browser: per cambiare dispositivo usa *Esporta JSON* / *Importa JSON* in fondo alla
  dashboard (l'import sostituisce i progressi attuali; il tema resta quello del dispositivo). *Azzera progressi*
  cancella concetti e quiz.
- Le risposte ai quiz sono legate all'indice della domanda, con il testo salvato accanto: se riordini le domande
  vengono ritrovate per testo; se ritocchi il testo di una domanda vale l'indice.
- "Da ripassare oggi": prima le "non so", poi le incerte (dalla più vecchia), poi le mai viste in ordine di
  percorso, poi le "so" più vecchie di 14 giorni (`REVIEW_AFTER_DAYS` in `assets/areas.js`).
