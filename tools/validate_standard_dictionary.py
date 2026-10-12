"""Offline integrity check for the pinned, attributed standard data layer."""
import argparse
import hashlib
import json
from pathlib import Path
import re

p = argparse.ArgumentParser()
p.add_argument("--path", type=Path, required=True)
p.add_argument("--metadata", type=Path, required=True)
p.add_argument("--notices", type=Path, required=True)
a = p.parse_args()
meta = json.loads(a.metadata.read_text(encoding="utf-8"))
assert meta["license"] == "Apache-2.0"
assert re.fullmatch("[0-9a-f]{40}", meta["upstream_commit"])
assert meta["heldout_sentences_excluded"] and meta["project_overrides_preserved"]
assert hashlib.sha256(a.path.read_bytes()).hexdigest() == meta["sha256"]
notices = a.notices.read_text(encoding="utf-8")
assert "Apache License" in notices and meta["upstream_commit"] in notices
seen = set()
for n, line in enumerate(a.path.read_text(encoding="utf-8").splitlines(), 1):
    if not line or line.startswith("#"): continue
    fields = line.split("\t")
    assert len(fields) == 3, f"Invalid fields at line {n}"
    key, word, frequency = fields
    assert re.fullmatch("[a-z]+", key) and re.fullmatch(r"[\u3400-\u9fff]{1,8}", word), f"Invalid text at line {n}"
    assert 5000 <= int(frequency) <= 9500, f"Invalid weight at line {n}"
    assert (key, word) not in seen, f"Duplicate at line {n}"
    seen.add((key, word))
assert len(seen) == meta["entry_count"] and 50000 <= len(seen) <= 70000
print(json.dumps({"valid_entries": len(seen), "license": meta["license"], "upstream_commit": meta["upstream_commit"]}))
