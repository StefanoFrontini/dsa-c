#!/usr/bin/env python3
"""Divide l'export della conversazione Gemini in un file per turno: study/turns/NNN.md + turns/index.json.

Un turno inizia a ogni riga che comincia con "## User:" (stessa numerazione usata nelle note: turno 1 = prima
domanda). Da rilanciare se l'export cambia:  python3 antirez/study/tools/split_turns.py
"""
import json, re
from pathlib import Path

study = Path(__file__).resolve().parent.parent
src = next(study.parent.glob("Gemini-*.md"))
out = study / "turns"
out.mkdir(exist_ok=True)
for old in out.glob("*.md"):
    old.unlink()

turns, cur = [], None
for line in src.read_text(encoding="utf-8").splitlines(keepends=True):
    if line.startswith("## User:"):
        cur = []
        turns.append(cur)
    if cur is not None:
        cur.append(line)

index = []
for n, lines in enumerate(turns, start=1):
    text = "".join(lines)
    (out / f"{n:03d}.md").write_text(text, encoding="utf-8")
    date = re.search(r"^> (\d+/\d+/\d{4})", text, re.M)
    m, d, y = date.group(1).split("/") if date else ("", "", "")
    body = text.split("## Gemini:")[0]
    body = re.sub(r"^(## User:|> .*)$", "", body, flags=re.M)
    preview = " ".join(body.split())[:160]
    index.append({"n": n, "date": f"{y}-{int(m):02d}-{int(d):02d}" if date else None, "preview": preview})

(out / "index.json").write_text(json.dumps({"source": src.name, "turns": index}, ensure_ascii=False, indent=1), encoding="utf-8")
print(f"{len(turns)} turni scritti in {out}")
