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
FILES = [OUT / f"batch019_{name}_draft.txt" for name in ("food_kitchen", "messages_schedule")]
WEIGHTS = {
    'food_grocery_selection': 5500,
    'food_refrigeration_and_portions': 5500,
    'food_washing_and_preparation': 5500,
    'food_peeling_and_knife_work': 5500,
    'food_marinating_and_seasoning': 5500,
    'food_heat_and_pan_control': 5500,
    'food_stir_frying_and_pan_frying': 5500,
    'food_soups_stews_and_sauces': 5500,
    'food_steaming_and_boiling': 5500,
    'food_rice_and_porridge': 5500,
    'food_leftovers_and_kitchen_cleanup': 5500,
    'food_package_and_pantry': 5000,
    'food_cutting_shapes': 5000,
    'food_noodles_dough_and_wrapping': 5000,
    'food_baking_and_cold_dishes': 5000,
    'food_kitchen_utensils_and_setup': 5000,
    'food_serving_and_mealtime': 6000,
    'work_email_compose': 5500,
    'work_email_inbox': 5500,
    'work_email_delivery': 5500,
    'work_calendar_events': 5500,
    'work_calendar_invitations': 5500,
    'work_calendar_views': 5500,
    'work_image_annotations': 5500,
    'work_design_collaboration': 5500,
    'software_input_method': 5500,
    'software_keyboard_input': 5500,
    'software_accessibility_navigation': 5500,
    'software_network_file_transfer': 5500,
    'work_design_assets': 5000,
    'software_screen_reader': 5000,
    'work_mail_schedule_messages': 6000,
}
QUALITY_EXCLUSIONS: dict[str, str] = {}
PRONUNCIATION_REFERENCE_LINKS: dict[str, str] = {
    '盛（把饭菜汤粥装入容器cheng）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=32353&la=0&powerMode=0',
    '调（调味、混合或调整tiao）': 'https://dict.mini.moe.edu.tw/SearchIndex/searchResult?dictSearchField=%E8%AA%BF&searchType=one',
    '调整': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=11011&powerMode=1',
    '和（混合面粉和水huo）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=5858&la=1',
    '长（段长或长按持续chang）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=32100&la=0&powerMode=0',
    '削（xiao语音及xue读音同义）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=26873&la=0&powerMode=0',
    '剥（bao语音及bo读音同义）': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=210&la=0&powerMode=0',
    '饧（面团变软xing及tang合法又音）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=75492&la=1',
    '择（ze及zhai合法又音）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=18153&la=1',
    '择菜': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=115175&la=0&powerMode=0',
    '薄（bao语音及bo读音同义；本批默认bao未改）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=38865&la=1',
    '黏／粘（黏性nian）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=12838&la=0&powerMode=0',
    '行（文字或读屏行hang）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=19765&la=0&powerMode=0',
    '重（再次操作chong）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=33068&la=0&powerMode=0',
    '没（否定存在或发生mei）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=3846&la=0&powerMode=0',
    '粘（作者核读的单字nian页）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=66214&la=1',
    'Microsoft粘滞键正式字形（只核功能术语，不导入正文）': 'https://support.microsoft.com/zh-cn/accessibility/windows/make-your-mouse-keyboard-and-other-input-devices-easier-to-use',
}
READING_VARIANT_WORDS: set[str] = {'剥去外皮', '削好土豆', '削去外皮', '剥开菜叶', '剥掉外皮', '削好苹果', '择净菜叶', '饧面备用', '削掉外皮', '剥好虾仁', '剥去虾壳'}



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
    parser.add_argument("--owner", choices=("food_kitchen", "messages_schedule"), help="Prepare one author worklist only; never freeze a partial batch")
    parser.add_argument("--combine-reviewed", action="store_true", help="Combine the two completed contextual worklists before freezing")
    args = parser.parse_args()
    assert pypinyin.__version__ == "0.55.0"
    if (OUT / "lexicon_batch_019.applied.json").exists():
        raise SystemExit("Batch 019 is applied; retain its frozen authoring baselines.")
    if args.owner:
        assert args.prepare, "--owner is prepare-only"
        assert not args.combine_reviewed, "A partial author worklist cannot be frozen"
        global FILES
        FILES = [OUT / f"batch019_{args.owner}_draft.txt"]
    selected, baselines, benchmarks, answers, authored, alternate, excluded_benchmark, duplicates, existing, quality = select()
    if args.prepare:
        pending = [f"{key}\t{word}\t{category}" for category, key, word, _ in selected if needs_review(word)]
        suffix = f"_{args.owner}" if args.owner else ""
        (OUT / f"batch019_pronunciation_pending{suffix}.tsv").write_text("# default pinyin\tword\tcategory; authoring review worklist only\n" + "\n".join(pending) + "\n", encoding="utf-8", newline="\n")
        (OUT / f"batch019_selected_preview{suffix}.tsv").write_text("# authoring preview only; not release data\n" + "\n".join(f"{key}\t{word}\t{category}" for category, key, word, _ in selected) + "\n", encoding="utf-8", newline="\n")
        print(json.dumps({"authored": authored, "selected": len(selected), "polyphone_review_required": len(pending), "duplicate_candidates": len(duplicates), "existing": len(existing), "alternate": len(alternate), "quality_rejected": len(quality), "categories": dict(Counter(row[0] for row in selected))}, ensure_ascii=False))
        return
    review_path = OUT / "batch019_pronunciation_review.tsv"
    required = {word for _, _, word, _ in selected if needs_review(word)}
    if args.combine_reviewed:
        assert not args.prepare, "Combining reviewed compounds is a freeze operation"
        combined = {}
        for owner in ("food_kitchen", "messages_schedule"):
            worklist = OUT / f"batch019_{owner}_review.tsv"
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
    output = OUT / "lexicon_batch_019.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    overrides = {w: k for w, (k, _, _) in reviewed.items() if k != default_key(w)}
    variant_choices = {w: k for w, k in overrides.items() if w in READING_VARIANT_WORDS}
    metadata = {
        "batch": "lexicon_batch_019", "authoring_source": "first_party_authored", "external_lexicon_import": False,
        "license": "MIT", "pronunciation_tool": "pypinyin 0.55.0 authoring-only; individually reviewed compounds",
        "authoring_files_sha256": {p.name: sha(p) for p in FILES}, "authored_candidates": authored,
        "authoring_tools_sha256": {name: sha(OUT / name) for name in ("build_lexicon_batch_019.py", "apply_lexicon_batch_019.py", "validate_lexicon_batch_019_native.py")},
        "new_entries": len(final), "unique_words": len(final), "sha256": sha(output),
        "category_counts": dict(sorted(Counter(row[0] for row in final).items())),
        "length_distribution": dict(sorted(Counter(str(len(row[2])) for row in final).items())),
        "frequency_range": sorted({row[3] for row in final}),
        "frequency_distribution": dict(sorted(Counter(str(row[3]) for row in final).items())),
        "frequency_policy": "Coarse project-relative 5000/5500/6000 bands by everyday utility, fixed before native evaluation; no corpus statistics or requirement to force first place.",
        "dedup_policy": "Exclude existing key/display pair and all existing display words across the three current runtime dictionaries; previously merged remote batches are already represented by those frozen baseline hashes",
        "dedup_baseline_sha256": {p.name: sha(p) for p in baselines}, "benchmark_sha256": {p.name: sha(p) for p in benchmarks},
        "benchmark_frozen_case_count": sum(1 for p in benchmarks for line in p.read_text(encoding="utf-8-sig").splitlines() if line and not line.startswith("#")),
        "benchmark_unique_full_answers": len(answers),
        "benchmark_full_answers_added": 0, "benchmark_answers_excluded": excluded_benchmark,
        "duplicate_author_candidates_excluded": duplicates, "existing_display_words_excluded": existing,
        "alternate_pronunciation_candidates_excluded": alternate, "quality_candidates_excluded": quality,
        "manual_pronunciation_overrides": overrides,
        "reading_variant_choices": variant_choices,
        "reading_variant_policy": "Eleven food main-key choices are legitimate spoken/literary or regional variants: four 削 compounds use xiao while xue is a same-meaning literary reading; five 剥 compounds use everyday bao while bo is a same-meaning literary reading; 饧面备用 uses xing while the primary dictionary also records tang; 择净菜叶 uses the conventional vegetable-picking zhai while ze is also a legitimate reading. Default 薄 bao, 熟 shu and 血水 xue are retained and are not counted as changed choices. No aliases are added for existing displays.",
        "pronunciation_review_file": review_path.name, "pronunciation_review_sha256": sha(review_path),
        "reviewer_worklists_sha256": {f"batch019_{name}_review.tsv": sha(OUT / f"batch019_{name}_review.tsv")
                                      for name in ("food_kitchen", "messages_schedule")},
        "pronunciation_review_record_method": "Each author read every required compound with its draft key in context before recording the row. Inline scripts only formatted the checked choices and notes; default agreement alone was not marked as review.",
        "polyphone_review_detection": "Any character with more than one toneless normal-style possible reading in pinned pypinyin 0.55.0; each compound is reviewed in context.",
        "pronunciation_review_reference_links": PRONUNCIATION_REFERENCE_LINKS,
        "pronunciation_reference_scope": "Individual word pronunciation definitions or explicit single-word/single-character pronunciation notations only; no external lexicon, article text, definition text, or usage examples imported.",
        "validation": {"all_outputs_two_to_six_han": True, "format_valid": True,
            "per_character_possible_readings_valid": True, "canonical_or_previous_batch_collisions": 0,
            "automatic_default_readings_changed": len(overrides),
            "semantic_pronunciation_corrections": len(overrides) - len(variant_choices),
            "reading_variant_choices": len(variant_choices), "polyphone_reviewed_display_words": len(required)},

        "author_revision_provenance": {"food_kitchen": {"early_candidates": 1244, "quality_removed_before_prepare": 13, "redundancy_screen_removed": 79, "naturalness_rewrites": 25, "final_draft_candidates": 1152}, "messages_schedule": {"early_candidates": 1329, "quality_or_redundancy_removed_before_final_draft": 248, "naturalness_rewrites": 61, "existing_filtered_before_final_draft": 21, "duplicates_filtered_before_final_draft": 1, "overlength_filtered_before_final_draft": 1, "final_draft_candidates": 1058}},
        "candidate_count_scope": "Authored candidate count refers to the final frozen drafts after ordinary author revisions, not an unrevised early brainstorming count.",
        "canonical_dictionaries_modified": False, "line_endings": "UTF-8 LF",
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("new_entries", "sha256", "category_counts", "length_distribution", "validation")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
