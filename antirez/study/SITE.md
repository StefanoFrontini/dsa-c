# Sito di studio

Sito statico (HTML + CSS + JS vanilla, ES modules) che legge a runtime le note, le schede e i dati di questa
cartella. Niente build, niente `node_modules`: si pubblica la cartella così com'è.

## Provarlo in locale

```
cd antirez/study
python3 -m http.server 8000
```

Poi apri <http://localhost:8000/>. Da `file://` non funziona: il browser blocca moduli JS e `fetch`
(la pagina lo spiega). Le librerie (`marked`, `highlight.js`) e i font arrivano da cdnjs / Google Fonts:
senza rete le note si vedono come testo semplice.

Pagine (routing hash, nessun redirect necessario):

- `#/` dashboard: percorso delle aree, "Da ripassare oggi", stato dei progetti, export/import dei progressi
- `#/area/<id>` nota dell'area con indice, "ripassato" per concetto e quiz di autoverifica
  (`#/area/<id>/<ancora>` porta a un titolo o a un concetto, es. `#/area/parsing/recursive-descent`)
- `#/project/<id>` scheda del progetto
- `#/errori` (o `#/errori/<area>`) tabella degli errori di Gemini, filtrabile

## Deploy su Netlify

1. Su Netlify: *Add new site → Import an existing project → GitHub*, scegli `StefanoFrontini/dsa-c`, branch `master`.
2. Le impostazioni sono già in `netlify.toml` nella root del repo: `publish = "antirez/study"`, nessun comando di build.
   Lascia vuoti "Build command" e "Publish directory" nell'interfaccia (vince il file).
3. Ogni push su `master` ripubblica il sito. Note, schede e JSON hanno `Cache-Control: max-age=0, must-revalidate`
   (una correzione si vede al primo ricaricamento); `assets/` ha 5 minuti di cache.

## Dove vivono i dati

| Cosa | File | Note |
|---|---|---|
| Ordine del percorso | `assets/areas.js` | unico elenco hardcoded; anche URL del repo e soglie del ripasso |
| Note delle aree | `notes/<area>.md` | il `### ` dentro `## Concetti` diventa un concetto con checkbox |
| Schede progetto | `projects/<id>.md` | `# Titolo — stato: X` → badge |
| Dati strutturati | `data/<area>.json` | concetti, progetti, errori, `self_check` (se aggiungi `a` la risposta compare a scomparsa) |
| Elenco dei file del repo | `assets/repo-files.json` | rigeneralo con `tools/update-repo-files.sh` dopo aver aggiunto file |

I concetti della nota vengono abbinati a quelli del JSON per titolo; se i titoli differiscono, per posizione
(funziona finché il numero di `###` coincide con `concepts`).

Link al codice: ogni `` `percorso:riga` `` (anche `:a-b`, `:a,b`, o senza riga) diventa un link a
`github.com/StefanoFrontini/dsa-c/blob/master/antirez/<percorso>#La-Lb`, nelle note, nelle schede e nei
`code_refs`. Con `assets/repo-files.json` i nomi senza cartella (`tac.c:44`) e i percorsi parziali
(`actor/poll_input.c`) vengono risolti sul file giusto, e i file che non esistono (segmenti `.ts` di esempio,
la cartella di build) restano testo; se un nome è ambiguo (`test.txt`) si sceglie il file della cartella del
progetto, altrimenti resta testo. `:86` senza file resta testo. `notes/x.md` e `projects/y.md` diventano link interni.

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
