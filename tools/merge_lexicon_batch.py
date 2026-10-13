"""Merge reviewed first-party batches and recompute canonical metadata."""
from __future__ import annotations
import argparse
from collections import Counter
from datetime import date
import hashlib
import json
from pathlib import Path
import re


def rows(path: Path):
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            fields = line.split("\t")
            if len(fields) != 3 or not re.fullmatch("[a-z]+", fields[0]) or not fields[1] or int(fields[2]) < 0:
                raise ValueError(f"Invalid row in {path}: {line!r}")
            yield fields


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--dictionary-dir", type=Path, required=True)
    p.add_argument("--batch", type=Path, required=True)
    p.add_argument("--holdout", type=Path, required=True)
    a = p.parse_args()
    canonical = a.dictionary_dir / "local_core_zh_pinyin.tsv"
    known = {(key, word) for filename in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv")
             for key, word, _ in rows(a.dictionary_dir / filename)}
    excluded = {line.split("\t")[1] for line in a.holdout.read_text(encoding="utf-8").splitlines()
                if line and not line.startswith("#") and len(line.split("\t")) >= 2 and len(line.split("\t")[1]) >= 6}
    additions = []
    for key, word, frequency in rows(a.batch):
        if word in excluded:
            raise ValueError(f"Batch contains a held-out full sentence: {word}")
        if (key, word) in known:
            continue
        known.add((key, word))
        additions.append(f"{key}\t{word}\t{frequency}")
    if additions:
        text = canonical.read_text(encoding="utf-8").rstrip()+"\n\n# category: " + a.batch.stem + "\n"
        canonical.write_text(text+"\n".join(additions)+"\n", encoding="utf-8", newline="\n")
    categories: Counter[str] = Counter()
    lengths: Counter[str] = Counter()
    category = ""
    for line in canonical.read_text(encoding="utf-8").splitlines():
        if line.startswith("# category:"):
            category = line.partition(":")[2].strip()
        elif line and not line.startswith("#"):
            word = line.split("\t")[1]
            categories[category] += 1
            lengths[{1: "single", 2: "two", 3: "three"}.get(len(word), "fourplus")] += 1
    meta_path = canonical.with_suffix(".metadata.json")
    meta = json.loads(meta_path.read_text(encoding="utf-8"))
    meta.update({"format_version": 3, "dictionary_name": "LocalPinyinIME curated common lexicon v3",
                 "entry_count": sum(categories.values()), "sha256": hashlib.sha256(canonical.read_bytes()).hexdigest().upper(),
                 "generated_at": date.today().isoformat(), "category_counts": dict(sorted(categories.items())),
                 "length_distribution": dict(sorted(lengths.items()))})
    source_id = "first_party_" + a.batch.stem
    if source_id not in meta["source_ids"]:
        meta["source_ids"].append(source_id)
    meta["source_policy"] = ("Project-maintained first-party authored lexicon. No third-party bulk lexicon was imported; "
        "no Rime, Sogou, Baidu, Microsoft, Google, or public GitHub candidate dictionary data is used. "
        "Pypinyin is an authoring-only pronunciation checker with manually reviewed compound overrides; "
        "neither Python nor its library is bundled in the input method. Independent prose used for the separate "
        "character context model is attributed in authoring/sources.json.")
    meta_path.write_text(json.dumps(meta, ensure_ascii=False, indent=2)+"\n", encoding="utf-8", newline="\n")
    print(json.dumps({"added": len(additions), "entry_count": meta["entry_count"], "sha256": meta["sha256"]}))


if __name__ == "__main__":
    main()
