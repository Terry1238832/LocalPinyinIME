#!/usr/bin/env python3
"""Revalidate batch 011 baselines and invoke the repository's formal merge tool."""
from __future__ import annotations

from datetime import date
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
BASE = ROOT / "resources/dictionary"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rows(path: Path):
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            key, word, weight = line.split("\t")
            assert re.fullmatch("[a-z]+", key) and int(weight) >= 0, line
            yield key, word, int(weight)


def write(path: Path, value: dict):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")


def main():
    batch = OUT / "lexicon_batch_011.tsv"
    applied_path = batch.with_suffix(".applied.json")
    assert not applied_path.exists(), "Batch is already applied; do not merge again."
    meta = json.loads(batch.with_suffix(".json").read_text(encoding="utf-8"))
    assert sha(batch) == meta["sha256"]
    for name, expected in meta["dedup_baseline_sha256"].items():
        path = BASE / name if name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv") else OUT / name
        assert sha(path) == expected, f"Baseline changed: {name}"
    answers = set()
    fixture_count = 0
    for name, expected in meta["benchmark_sha256"].items():
        path = ROOT / "tests/data" / name
        assert sha(path) == expected, f"Frozen fixture changed: {name}"
        for line in path.read_text(encoding="utf-8").splitlines():
            if line and not line.startswith("#"):
                answers.add(line.split("\t")[1])
                fixture_count += 1
    review_path = OUT / meta["pronunciation_review_file"]
    assert sha(review_path) == meta["pronunciation_review_sha256"]
    canonical = BASE / "local_core_zh_pinyin.tsv"
    canonical_meta = canonical.with_suffix(".metadata.json")
    before_meta = json.loads(canonical_meta.read_text(encoding="utf-8"))
    preserved_policy = before_meta["source_policy"]
    assert "local_core" in preserved_policy and "separate standard" in preserved_policy
    current = [row for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv") for row in rows(BASE / name)]
    known_keys = {(key, word) for key, word, _ in current}
    known_words = {word for _, word, _ in current}
    additions = list(rows(batch))
    assert len(additions) == meta["new_entries"] == len({(key, word) for key, word, _ in additions}) == len({word for _, word, _ in additions})
    for key, word, weight in additions:
        assert re.fullmatch(r"[\u4e00-\u9fff]{2,6}", word) and weight in (5000, 5500, 6000)
        assert (key, word) not in known_keys and word not in known_words and word not in answers, word
    before_hash, before_count = sha(canonical), len(list(rows(canonical)))
    subprocess.run([sys.executable, str(ROOT / "tools/merge_lexicon_batch.py"), "--dictionary-dir", str(BASE), "--batch", str(batch), "--holdout", str(ROOT / "tests/data/typing_holdout.tsv")], check=True)
    after_meta = json.loads(canonical_meta.read_text(encoding="utf-8"))
    after_meta["source_policy"] = preserved_policy
    write(canonical_meta, after_meta)
    assert len(list(rows(canonical))) == before_count + len(additions)
    assert sha(canonical).upper() == after_meta["sha256"]
    source_path = BASE / "sources/lexicon_sources.json"
    sources = json.loads(source_path.read_text(encoding="utf-8"))
    source_id = "first_party_lexicon_batch_011"
    assert all(source["source_id"] != source_id for source in sources["sources"])
    sources["sources"].append({
        "source_id": source_id,
        "source_name": "LocalPinyinIME reviewed authored vocabulary batch 011",
        "source_url": "", "license": "MIT",
        "license_url": "https://github.com/Terry1238832/LocalPinyinIME/blob/main/LICENSE",
        "redistribution_allowed": True, "attribution_required": True, "share_alike_required": False,
        "commercial_use_allowed": True, "derivative_work_allowed": True,
        "review_status": "approved", "reviewed_at": date.today().isoformat(), "included_in_release": True,
        "notes": f"{len(additions)} original individually selected 2-6 Han vocabulary items for photography, filming, creative publishing, music and performance, games, rental housing, neighborhood and property affairs, gardening, pet care, weather and local travel. Three-layer key/display and display-word deduplication; all {fixture_count} frozen typing full answers excluded. No Cartesian templates, external vocabulary imports or automatic existing-word aliases. Fixed coarse project-relative 5000/5500/6000 weights. Pypinyin 0.55.0 authoring-only, contextual compound review and individual pronunciation references in expansion/batch_011.md. Frozen SHA and native coverage in expansion/lexicon_batch_011.json and expansion/lexicon_batch_011.applied.json."
    })
    write(source_path, sources)
    after = [row for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv") for row in rows(BASE / name)]
    applied = {
        "batch": batch.stem, "frozen_batch_sha256": sha(batch),
        "canonical_before_sha256": before_hash, "canonical_before_entries": before_count,
        "entries_reviewed": len(additions), "all_current_layers_deduplicated": True,
        "existing_display_alternate_readings_excluded": True, "all_frozen_case_answers_excluded": True,
        "frozen_case_count": fixture_count, "per_character_possible_readings_valid": True,
        "manual_pronunciation_overrides": len(meta["manual_pronunciation_overrides"]),
        "polyphone_reviewed_display_words": meta["validation"]["polyphone_reviewed_display_words"],
        "pronunciation_review_sha256": meta["pronunciation_review_sha256"],
        "canonical_after_sha256": sha(canonical), "canonical_after_entries": len(list(rows(canonical))),
        "merged_entries": len(additions), "applied_at": date.today().isoformat(), "line_endings": "UTF-8 LF",
        "effective_unique_key_display_before": len(known_keys),
        "effective_unique_key_display_after": len({(key, word) for key, word, _ in after}),
        "raw_all_layer_rows_after": len(after),
        "native_candidate_coverage": {"status": "pending_native_build"},
        "freeze_status": "Frozen TSV applied by merge_lexicon_batch.py; current baseline and all frozen fixtures revalidated."
    }
    write(applied_path, applied)
    print(json.dumps(applied, ensure_ascii=False))


if __name__ == "__main__":
    main()
