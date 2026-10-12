"""Build compact character counts from attributed, independent prose.

The shipped model has only 1-4 character statistics, not memorized sentences.
No external Python dependency or network access is needed for regeneration.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

HAN_RUN = re.compile(r"[\u3400-\u9fff]+")


def build(inputs: list[Path], excluded_cases: list[Path]) -> tuple[Counter[str], dict]:
    excluded = {line.split("\t")[1] for path in excluded_cases
                for line in path.read_text(encoding="utf-8").splitlines()
                if line and not line.startswith("#") and len(line.split("\t")) >= 2
                and len(line.split("\t")[1]) >= 6}
    counts: Counter[str] = Counter()
    manifest = []
    skipped = 0
    characters = 0
    for path in sorted(inputs):
        text = path.read_text(encoding="utf-8")
        weight = 4 if path.name.startswith("daily_") else 1
        for line in text.splitlines():
            if line.startswith("#"):
                continue
            for match in HAN_RUN.finditer(line):
                clause = match.group()
                if any(expected in clause for expected in excluded):
                    skipped += 1
                    continue
                if len(clause) < 2:
                    continue
                characters += len(clause)
                sequence = "^" + clause + "$"
                for n in range(1, 5):
                    for i in range(len(sequence)-n+1):
                        gram = sequence[i:i+n]
                        if gram not in {"^", "$"}: counts[gram] += weight
        manifest.append({"file": path.name, "weight": weight, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    return counts, {"inputs": manifest, "han_characters": characters,
                    "excluded_case_files": [{"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                                            for path in excluded_cases],
                    "excluded_holdout_clauses": skipped, "training_excludes_holdout": True}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--corpus-dir", type=Path, required=True)
    parser.add_argument("--exclude-cases", type=Path, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    counts, metadata = build(list(args.corpus_dir.glob("*.txt")), args.exclude_cases)
    lines = ["# LocalPinyinIME character context model v2; independent prose; 1-4 character counts"]
    lines.extend(f"{text}\t{min(count, 100000)}" for text, count in sorted(counts.items()))
    args.output.write_text("\n".join(lines)+"\n", encoding="utf-8")
    metadata.update({"format_version": 2, "maximum_order": 4, "entry_count": len(counts),
                     "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
                     "runtime_dependencies": "none", "source_manifest": "authoring/sources.json"})
    args.output.with_suffix(".metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in metadata.items() if k != "inputs"}, ensure_ascii=True))


if __name__ == "__main__":
    main()
