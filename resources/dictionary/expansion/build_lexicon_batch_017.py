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
FILES = [OUT / f"batch017_{name}_draft.txt" for name in ("repairs_devices", "care_family")]
WEIGHTS = {
    "repair_booking_and_visits": 5500,
    "air_conditioning_service": 5500,
    "refrigeration_and_laundry": 5500,
    "kitchen_appliance_service": 5000,
    "water_heating_and_drinking": 5500,
    "lighting_and_home_electricity": 5500,
    "plumbing_and_bathroom_fixtures": 5500,
    "doors_windows_and_furniture": 5500,
    "household_cleaning_service": 5500,
    "cleaning_tools_and_laundry_care": 5000,
    "mobile_device_repairs": 5500,
    "computer_hardware_service": 5000,
    "home_network_and_connections": 5500,
    "printing_and_scanning_devices": 5000,
    "displays_audio_and_accessories": 5000,
    "device_data_and_settings": 5500,
    "television_and_playback_controls": 5500,
    "daily_fault_reports": 6000,
    "hospital_visit_and_counter": 5500,
    "appointments_and_waiting": 5500,
    "billing_and_visit_documents": 5500,
    "ordinary_tests_and_reports": 5500,
    "childcare_entry_and_contact": 5500,
    "nursery_routines_and_pickup": 5500,
    "elder_services_and_registration": 5500,
    "ward_visits_and_admission": 5000,
    "family_patient_assistance": 5000,
    "home_care_and_accessibility": 5000,
    "family_routines_and_coordination": 5000,
    "parent_child_talk_and_play": 5000,
}
QUALITY_EXCLUSIONS: dict[str, str] = {"冰箱门关严": "词面不够自然，保留既有自然关门表达和冰箱门没关。"}
PRONUNCIATION_REFERENCE_LINKS: dict[str, str] = {
    "行（表格或打印行）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=19765&la=0&powerMode=0",
    "重（重新执行）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=33068&la=0&powerMode=0",
    "归还": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=17223&la=0&powerMode=0",
    "卡住（大陆单词标音，不导入正文）": "https://www.succ.edu.cn/yywz/Home/News/view/xwid/0a6844f71ed449a14c9aa7337f94369f",
    "钥匙": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=41195&la=0&powerMode=0",
    "校（校色校准）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=22514&la=0&powerMode=0",
    "调（调试、调节）": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=52655&q=1&word=%E8%AA%BF",
    "采血（中国语检定协会作者刊物单词拼音）": "https://www.chuken.gr.jp/study/wa/wa_128.pdf",
    "采血（华文语言研究机构自撰单词课堂）": "https://www.arsomsiam.com/caixue/",
}
READING_VARIANT_WORDS = {
    "采血窗口", "采血楼层", "采血地点", "采血时间", "采血排队号", "采血排队",
    "采血登记", "采血叫号", "采血凭证", "采血等候", "采血顺序", "采血记录",
}



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
                    assert category in WEIGHTS, f"Unreviewed frequency category: {category}"
                    yield category, word, WEIGHTS[category]


def select():
    base = ROOT / "resources/dictionary"
    baselines = [base / name for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")]
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
    parser.add_argument("--owner", choices=("repairs_devices", "care_family"), help="Prepare one author worklist only; never freeze a partial batch")
    parser.add_argument("--combine-reviewed", action="store_true", help="Combine the two completed contextual worklists before freezing")
    args = parser.parse_args()
    assert pypinyin.__version__ == "0.55.0"
    if (OUT / "lexicon_batch_017.applied.json").exists():
        raise SystemExit("Batch 017 is applied; retain its frozen authoring baselines.")
    if args.owner:
        assert args.prepare, "--owner is prepare-only"
        assert not args.combine_reviewed, "A partial author worklist cannot be frozen"
        global FILES
        FILES = [OUT / f"batch017_{args.owner}_draft.txt"]
    selected, baselines, benchmarks, answers, authored, alternate, excluded_benchmark, duplicates, existing, quality = select()
    if args.prepare:
        pending = [f"{key}\t{word}\t{category}" for category, key, word, _ in selected if needs_review(word)]
        suffix = f"_{args.owner}" if args.owner else ""
        (OUT / f"batch017_pronunciation_pending{suffix}.tsv").write_text("# default pinyin\tword\tcategory; authoring review worklist only\n" + "\n".join(pending) + "\n", encoding="utf-8", newline="\n")
        (OUT / f"batch017_selected_preview{suffix}.tsv").write_text("# authoring preview only; not release data\n" + "\n".join(f"{key}\t{word}\t{category}" for category, key, word, _ in selected) + "\n", encoding="utf-8", newline="\n")
        print(json.dumps({"authored": authored, "selected": len(selected), "polyphone_review_required": len(pending), "duplicate_candidates": len(duplicates), "existing": len(existing), "alternate": len(alternate), "quality_rejected": len(quality), "categories": dict(Counter(row[0] for row in selected))}, ensure_ascii=False))
        return
    review_path = OUT / "batch017_pronunciation_review.tsv"
    required = {word for _, _, word, _ in selected if needs_review(word)}
    if args.combine_reviewed:
        assert not args.prepare, "Combining reviewed compounds is a freeze operation"
        combined = {}
        for owner in ("repairs_devices", "care_family"):
            worklist = OUT / f"batch017_{owner}_review.tsv"
            for line in worklist.read_text(encoding="utf-8").splitlines():
                if not line or line.startswith("#"):
                    continue
                key, word, reviewer, note = line.split("\t")
                assert reviewer and note and re.fullmatch("[a-z]+", key), line
                if word in combined:
                    assert combined[word][0] == key, ("Authors disagree on compound reading", word)
                else:
                    combined[word] = (key, reviewer, note)
        assert required <= set(combined), ("Unreviewed selected compounds", sorted(required - set(combined)))
        lines = ["# pinyin\tword\treviewer\tcontextual pronunciation decision"]
        for word in sorted(required):
            key, reviewer, note = combined[word]
            lines.append(f"{key}\t{word}\t{reviewer}\t{note}")
        review_path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    reviewed = {}
    for line in review_path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            key, word, reviewer, note = line.split("\t")
            assert word not in reviewed and reviewer and note and re.fullmatch("[a-z]+", key), line
            reviewed[word] = (key, reviewer, note)
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
    output = OUT / "lexicon_batch_017.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    overrides = {w: k for w, (k, _, _) in reviewed.items() if k != default_key(w)}
    variant_choices = {w: k for w, k in overrides.items() if w in READING_VARIANT_WORDS}
    metadata = {
        "batch": "lexicon_batch_017", "authoring_source": "first_party_authored", "external_lexicon_import": False,
        "license": "MIT", "pronunciation_tool": "pypinyin 0.55.0 authoring-only; individually reviewed compounds",
        "authoring_files_sha256": {p.name: sha(p) for p in FILES}, "authored_candidates": authored,
        "new_entries": len(final), "unique_words": len(final), "sha256": sha(output),
        "category_counts": dict(sorted(Counter(row[0] for row in final).items())),
        "length_distribution": dict(sorted(Counter(str(len(row[2])) for row in final).items())),
        "frequency_range": sorted({row[3] for row in final}),
        "frequency_distribution": dict(sorted(Counter(str(row[3]) for row in final).items())),
        "frequency_policy": "Coarse project-relative 5000/5500/6000 bands by everyday utility, fixed before native evaluation; no corpus statistics or requirement to force first place.",
        "dedup_policy": "Exclude existing key/display pair and all existing display words across the three current runtime dictionaries; previously merged remote batches are already represented by those frozen baseline hashes",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines}, "benchmark_sha256": {p.name: sha(p) for p in benchmarks},
        "benchmark_full_answers_added": 0, "benchmark_answers_excluded": excluded_benchmark,
        "duplicate_author_candidates_excluded": duplicates, "existing_display_words_excluded": existing,
        "alternate_pronunciation_candidates_excluded": alternate, "quality_candidates_excluded": quality,
        "manual_pronunciation_overrides": overrides,
        "reading_variant_choices": variant_choices,
        "reading_variant_policy": "For these formal service terms, 血 uses the literary xue main key; default colloquial xie is a legitimate reading, not a pronunciation error. No alternate aliases are added for existing displays.",
        "pronunciation_review_file": review_path.name, "pronunciation_review_sha256": sha(review_path),
        "reviewer_worklists_sha256": {f"batch017_{name}_review.tsv": sha(OUT / f"batch017_{name}_review.tsv")
                                      for name in ("repairs_devices", "care_family")},
        "polyphone_review_detection": "Any character with more than one toneless normal-style possible reading in pinned pypinyin 0.55.0; each compound is reviewed in context.",
        "pronunciation_review_reference_links": PRONUNCIATION_REFERENCE_LINKS,
        "pronunciation_reference_scope": "Individual word pronunciation definitions or explicit single-word/single-character pronunciation notations only; no external lexicon, article text, definition text, or usage examples imported.",
        "validation": {"all_outputs_two_to_six_han": True, "format_valid": True,
            "per_character_possible_readings_valid": True, "canonical_or_previous_batch_collisions": 0,
            "automatic_default_readings_changed": len(overrides),
            "semantic_pronunciation_corrections": len(overrides) - len(variant_choices),
            "reading_variant_choices": len(variant_choices), "polyphone_reviewed_display_words": len(required)},
        "canonical_dictionaries_modified": False, "line_endings": "UTF-8 LF",
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("new_entries", "sha256", "category_counts", "length_distribution", "validation")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
