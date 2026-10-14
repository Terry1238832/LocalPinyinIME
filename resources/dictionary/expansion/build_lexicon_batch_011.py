#!/usr/bin/env python3
"""Freeze individually authored vocabulary after compound pronunciation review.

The authoring-only dependency is pypinyin 0.55.0. Prepare mode produces review
worklists, never a release batch; an applied batch cannot be regenerated.
"""
from __future__ import annotations

import argparse
from collections import Counter
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import re

import pypinyin
from pypinyin import Style, lazy_pinyin, pinyin

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
FILES = [OUT / f"batch011_{name}_draft.txt" for name in ("creation_media", "community_living", "outdoor_transit")]
WEIGHTS = {
    "photography_and_filming": 5000,
    "video_editing_and_postproduction": 5000,
    "graphic_creation_and_publishing": 5000,
    "writing_and_submissions": 5000,
    "music_rehearsal_and_performance": 5000,
    "game_creation_and_communities": 5000,
    "weather_and_outdoor_plans": 6000,
    "local_streets_and_community": 5500,
    "driving_and_parking": 5500,
    "cycling_and_roadside": 5500,
}
QUALITY_EXCLUSIONS: dict[str, str] = {}
PRONUNCIATION_REFERENCE_LINKS: dict[str, str] = {
    "钥匙": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=41195&la=0&powerMode=0",
    "落下（遗落）": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=59225&la=0&powerMode=0",
    "系（绑扣）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=21712&la=0&powerMode=0",
    "藤蔓（上海植物园官方单词标音）": "https://m.thepaper.cn/newsDetail_forward_29410860",
    "拎（上海政府单字标音）": "https://service.shanghai.gov.cn/sheninfo/specialdetail.aspx?Id=de864051-48ef-4297-b72c-d2c4eecaac66",
}

PRONUNCIATION_REFERENCE_LINKS.update({'空调': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=18915&la=0&powerMode=0', '扎（捆束）': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=7634', '黏': 'https://stroke-order.learningweb.moe.edu.tw/dictMean.jsp?ID=40655', '调弦': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=52655&q=1&word=%E8%AA%BF', '移调': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=150456&la=0&powerMode=0', '校对': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=22516&la=0&powerMode=0'})


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def default_key(word: str) -> str:
    return "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))


@lru_cache(maxsize=None)
def multiple_readings(char: str) -> bool:
    return len(pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0]) > 1


def needs_review(word: str) -> bool:
    return any(multiple_readings(c) for c in word)


def candidates():
    for path in FILES:
        category = ""
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.startswith("# category:"):
                category = line.partition(":")[2].strip()
            elif line and not line.startswith("#"):
                assert category, f"Missing category in {path.name}"
                for word in line.split():
                    assert re.fullmatch(r"[\u4e00-\u9fff]{2,6}", word), word
                    yield category, word, WEIGHTS.get(category, 5500)


def select():
    base = ROOT / "resources/dictionary"
    baselines = [base / name for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
    baselines += [OUT / f"lexicon_batch_{n:03d}.tsv" for n in range(1, 11)]
    known_keys, known_words = set(), set()
    for path in baselines:
        for line in path.read_text(encoding="utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                key, word, _ = line.split("\t")
                known_keys.add((key, word))
                known_words.add(word)
    benchmarks = sorted((ROOT / "tests/data").glob("typing_*.tsv"))
    answers = {line.split("\t")[1] for path in benchmarks for line in path.read_text(encoding="utf-8-sig").splitlines()
               if line and not line.startswith("#")}
    selected, seen, alternate, excluded_benchmark, duplicates, existing, quality = [], set(), [], [], [], [], []
    authored = 0
    for category, word, weight in candidates():
        authored += 1
        if word in seen:
            duplicates.append(word)
            continue
        seen.add(word)
        if word in answers:
            excluded_benchmark.append(word)
            continue
        if word in QUALITY_EXCLUSIONS:
            quality.append({"word": word, "reason": QUALITY_EXCLUSIONS[word]})
            continue
        key = default_key(word)
        assert re.fullmatch("[a-z]+", key), (key, word)
        if (key, word) in known_keys:
            existing.append(word)
            continue
        if word in known_words:
            alternate.append({"word": word, "proposed_pinyin": key})
            continue
        selected.append((category, key, word, weight))
    selected.sort(key=lambda row: (row[0], row[1], row[2]))
    return selected, baselines, benchmarks, answers, authored, alternate, excluded_benchmark, duplicates, existing, quality


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--prepare", action="store_true")
    args = parser.parse_args()
    assert pypinyin.__version__ == "0.55.0"
    if (OUT / "lexicon_batch_011.applied.json").exists():
        raise SystemExit("Batch 011 is applied; retain its frozen authoring baselines.")
    selected, baselines, benchmarks, answers, authored, alternate, excluded_benchmark, duplicates, existing, quality = select()
    if args.prepare:
        pending = [f"{key}\t{word}\t{category}" for category, key, word, _ in selected if needs_review(word)]
        (OUT / "batch011_pronunciation_pending.tsv").write_text("# default pinyin\tword\tcategory; authoring review worklist only\n" + "\n".join(pending) + "\n", encoding="utf-8", newline="\n")
        (OUT / "batch011_selected_preview.tsv").write_text("# authoring preview only; not release data\n" + "\n".join(f"{key}\t{word}\t{category}" for category, key, word, _ in selected) + "\n", encoding="utf-8", newline="\n")
        print(json.dumps({"authored": authored, "selected": len(selected), "polyphone_review_required": len(pending), "duplicate_candidates": len(duplicates), "existing": len(existing), "alternate": len(alternate), "quality_rejected": len(quality), "categories": dict(Counter(row[0] for row in selected))}, ensure_ascii=False))
        return
    review_path = OUT / "batch011_pronunciation_review.tsv"
    reviewed = {}
    for line in review_path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            key, word, reviewer, note = line.split("\t")
            assert word not in reviewed and reviewer and note and re.fullmatch("[a-z]+", key), line
            reviewed[word] = (key, reviewer, note)
    required = {word for _, _, word, _ in selected if needs_review(word)}
    assert set(reviewed) == required, ("review membership", sorted(required - set(reviewed)), sorted(set(reviewed) - required))
    final = []
    for category, key, word, weight in selected:
        key = reviewed[word][0] if word in reviewed else key
        positions = {0}
        for char in word:
            choices = pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0]
            positions = {i + len(c) for i in positions for c in choices if key.startswith(c, i)}
        assert len(key) in positions, ("Impossible character reading", key, word)
        assert word not in answers and weight in (5000, 5500, 6000)
        final.append((category, key, word, weight))
    final.sort(key=lambda row: (row[0], row[1], row[2]))
    assert len(final) == len({(k, w) for _, k, w, _ in final}) == len({w for _, _, w, _ in final})
    lines, previous = [], None
    for category, key, word, weight in final:
        if previous != category:
            lines.append(f"# category: {category}")
            previous = category
        lines.append(f"{key}\t{word}\t{weight}")
    output = OUT / "lexicon_batch_011.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    overrides = {w: k for w, (k, _, _) in reviewed.items() if k != default_key(w)}
    metadata = {
        "batch": "lexicon_batch_011", "authoring_source": "first_party_authored", "external_lexicon_import": False,
        "license": "MIT", "pronunciation_tool": "pypinyin 0.55.0 authoring-only; individually reviewed compounds",
        "authoring_files_sha256": {p.name: sha(p) for p in FILES}, "authored_candidates": authored,
        "new_entries": len(final), "unique_words": len(final), "sha256": sha(output),
        "category_counts": dict(sorted(Counter(row[0] for row in final).items())),
        "length_distribution": dict(sorted(Counter(str(len(row[2])) for row in final).items())),
        "frequency_range": sorted({row[3] for row in final}),
        "frequency_distribution": dict(sorted(Counter(str(row[3]) for row in final).items())),
        "frequency_policy": "Coarse project-relative 5000/5500/6000 bands by everyday utility, fixed before native evaluation; no corpus statistics or requirement to force first place.",
        "dedup_policy": "Exclude existing key/display pair and all existing display words across core, local_core, standard and all previous frozen batches",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines}, "benchmark_sha256": {p.name: sha(p) for p in benchmarks},
        "benchmark_full_answers_added": 0, "benchmark_answers_excluded": excluded_benchmark,
        "duplicate_author_candidates_excluded": duplicates, "existing_display_words_excluded": existing,
        "alternate_pronunciation_candidates_excluded": alternate, "quality_candidates_excluded": quality,
        "manual_pronunciation_overrides": overrides,
        "pronunciation_review_file": review_path.name, "pronunciation_review_sha256": sha(review_path),
        "reviewer_worklists_sha256": {f"batch011_{name}_review.tsv": sha(OUT / f"batch011_{name}_review.tsv")
                                      for name in ("creation_media", "community_living", "outdoor_transit")},
        "polyphone_review_detection": "Any character with more than one toneless normal-style possible reading in pinned pypinyin 0.55.0; each compound is reviewed in context.",
        "pronunciation_review_reference_links": PRONUNCIATION_REFERENCE_LINKS,
        "pronunciation_reference_scope": "Individual word pronunciation definitions or explicit single-word/single-character pronunciation notations only; no external lexicon, article text, definition text, or usage examples imported.",
        "validation": {"all_outputs_two_to_six_han": True, "format_valid": True,
            "per_character_possible_readings_valid": True, "canonical_or_previous_batch_collisions": 0,
            "automatic_default_readings_corrected": len(overrides), "polyphone_reviewed_display_words": len(required)},
        "canonical_dictionaries_modified": False, "line_endings": "UTF-8 LF",
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("new_entries", "sha256", "category_counts", "length_distribution", "validation")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
