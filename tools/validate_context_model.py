"""Verify the packaged model, source hashes and excluded evaluation fixtures offline."""
import argparse
import hashlib
import json
from pathlib import Path
import re

p = argparse.ArgumentParser()
p.add_argument("--dictionary-dir", type=Path, required=True)
p.add_argument("--cases-dir", type=Path, required=True)
a = p.parse_args()
root = a.dictionary_dir
model = root / "context_zh.tsv"
meta = json.loads(model.with_suffix(".metadata.json").read_text(encoding="utf-8"))
assert 0 < model.stat().st_size <= 4 * 1024 * 1024
assert hashlib.sha256(model.read_bytes()).hexdigest() == meta["sha256"]
assert meta["maximum_order"] == 4 and meta["runtime_dependencies"] == "none"
assert meta["training_excludes_holdout"]
seen = set()
for n, line in enumerate(model.read_text(encoding="utf-8").splitlines(), 1):
    if not line or line.startswith("#"):
        continue
    gram, count = line.split("\t")
    assert re.fullmatch(r"\^?[\u3400-\u9fff]+\$?", gram), f"Invalid gram at {n}"
    assert 1 <= len(gram) <= 4 and 0 < int(count) <= 100000
    assert gram not in seen, f"Duplicate at {n}"
    seen.add(gram)
assert len(seen) == meta["entry_count"] and len(seen) <= 200000
sources = json.loads((root / "authoring/sources.json").read_text(encoding="utf-8"))
attributed = {s["file"] for s in sources["sources"] if s.get("rights") and s.get("author")}
for item in meta["inputs"]:
    assert Path(item["file"]).name == item["file"] and item["file"] in attributed
    path = root / "authoring" / item["file"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"]
excluded = {item["file"] for item in meta["excluded_case_files"]}
assert {"typing_holdout.tsv", "typing_fresh.tsv"} <= excluded
for item in meta["excluded_case_files"]:
    assert Path(item["file"]).name == item["file"]
    path = a.cases_dir / item["file"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"]
print(json.dumps({"valid_ngrams": len(seen), "attributed_inputs": len(meta["inputs"]),
                  "excluded_fixtures": len(excluded)}))
