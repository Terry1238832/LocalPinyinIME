"""Validate the shipped first-party continuation associations and provenance."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dictionary-dir", type=Path, required=True)
    args = parser.parse_args()
    path = args.dictionary_dir / "continuations_zh.tsv"
    metadata = json.loads(path.with_suffix(".metadata.json").read_text(encoding="utf-8"))
    seen = set()
    prefixes = set()
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        assert len(fields) == 3, f"Invalid columns at line {number}"
        prefix, next_text, weight = fields
        assert re.fullmatch(r"[\u3400-\u9fff]{2,8}", prefix), f"Invalid prefix at {number}"
        assert re.fullmatch(r"[\u3400-\u9fff]{1,16}", next_text), f"Invalid continuation at {number}"
        assert weight.isascii() and weight.isdigit() and 100 <= int(weight) <= 10000, f"Invalid weight at {number}"
        assert (prefix, next_text) not in seen, f"Duplicate association at {number}"
        seen.add((prefix, next_text))
        prefixes.add(prefix)
    assert 200 <= len(seen) <= 5000, "Unexpected model size"
    assert metadata["entry_count"] == len(seen), "Metadata count differs"
    assert metadata["sha256"].lower() == hashlib.sha256(path.read_bytes()).hexdigest(), "Model checksum differs"
    assert metadata["authoring_source"] == "first_party_authored"
    assert metadata["license"] == "MIT"
    assert metadata["review_status"] == "approved" and metadata["included_in_release"] is True
    print(json.dumps({"entries": len(seen), "prefixes": len(prefixes), "sha256": metadata["sha256"]}))

if __name__ == "__main__":
    main()
