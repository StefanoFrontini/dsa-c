#!/bin/sh
# Rigenera assets/repo-files.json: l'elenco dei file di antirez/ tracciati da git.
# Il sito lo usa per risolvere i riferimenti al codice senza cartella (es. `tac.c:44`)
# e per non creare link a file che non esistono. Da lanciare dopo aver aggiunto file.
set -eu
cd "$(dirname "$0")/.."
git ls-files -- .. ':!:../study' | sed 's#^\.\./##' | python3 -c '
import json, sys, datetime
files = sorted(l.strip() for l in sys.stdin if l.strip())
print(json.dumps({"generated": datetime.date.today().isoformat(), "files": files}, indent=0, ensure_ascii=False))
' > assets/repo-files.json
echo "assets/repo-files.json: $(grep -c '"' assets/repo-files.json) righe"
