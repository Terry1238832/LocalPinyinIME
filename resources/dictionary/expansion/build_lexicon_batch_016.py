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
FILES = [OUT / f"batch016_{name}_draft.txt" for name in ("school_work", "services_travel")]
WEIGHTS = {
    "school_daily_routines": 5500,
    "classroom_materials": 5000,
    "homework_and_feedback": 5500,
    "reading_and_language_learning": 5500,
    "math_and_science_study": 5000,
    "foreign_language_learning": 5000,
    "examination_and_revision": 5500,
    "teaching_design_and_research": 5000,
    "teacher_professional_development": 5000,
    "class_management_and_wellbeing": 5500,
    "campus_and_extracurricular": 5500,
    "study_planning_and_college": 5500,
    "workplace_meetings_and_coordination": 5500,
    "workplace_tasks_and_delivery": 5500,
    "office_documents_and_files": 5500,
    "office_communication_and_followup": 6000,
    "onboarding_and_staff_development": 5500,
    "office_administration_and_expenses": 5500,
    "service_quality_and_customer_care": 5500,
    "community_and_volunteering": 5500,
    "practical_workplace_skills": 5000,
    "civil_documents_and_service_counter": 5500,
    "residence_and_public_records": 5500,
    "public_benefits_and_funds": 5500,
    "utilities_and_neighborhood_service": 5500,
    "banking_and_payment_service": 5500,
    "postal_and_shipping_counter": 5500,
    "parcel_delivery_and_collection": 5500,
    "freight_and_moving_arrangements": 5500,
    "rail_and_intercity_ticketing": 5500,
    "city_transport_and_transfer": 5500,
    "taxi_and_shared_mobility": 5500,
    "flights_and_airport_service": 5500,
    "trip_itinerary_and_lodging": 5500,
    "border_documents_and_overseas_plans": 5500,
    "community_venues_and_public_facilities": 5000,
    "swimming_and_water_activity": 5000,
    "gym_strength_and_training": 5000,
    "running_walking_and_track": 5000,
    "racket_ball_and_court": 5000,
    "team_sport_and_recreation": 5000,
    "classes_yoga_and_indoor_activity": 5000,
}
QUALITY_EXCLUSIONS: dict[str, str] = {
    "请假条子": "学校书面场景采用更自然的请假条；不收为凑数延长的同义词面。",
    "白板擦子": "优先自然短名白板擦；不收非必要口语后缀。",
    "试卷袋子": "考务场景采用试卷袋。",
    "节目单册": "常用表达节目单已可组成，不收生硬加册词面。",
    "成绩单册": "常用表达成绩单已可组成，不收生硬加册词面。",
}
PRONUNCIATION_REFERENCE_LINKS: dict[str, str] = {
    "校（学校）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=28506&la=0&powerMode=0",
    "行（成排/表格行高）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=19765&la=0&powerMode=0",
    "差（派遣、出差）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=31705&la=0&powerMode=0",
    "重（再做）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=33068&la=0&powerMode=0",
    "削（削铅笔）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=26873&la=0&powerMode=0",
    "模板（大陆普通话测试教材单词标音）": "https://hsfz.hainnu.edu.cn/_upload/article/files/62/35/5640eab04e22b040834cbf0fbb9c/5bf0def9-ea37-46d2-a2ae-a33baf13d404.pdf",
    "瑜伽": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=44227&la=0&powerMode=0",
    "调（档案调取）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=8732&la=0&powerMode=0",
    "银行、支行（Collins出版者单词拼音）": "https://resources.collins.co.uk/Dictionary/CD%20Resources/Collins_Chinese_3000_words_and_phrases.pdf",
    "归还": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=17223&la=0&powerMode=0",
    "弹（弹开）": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=10484&la=0&powerMode=0",
    "行走": "https://dict.revised.moe.edu.tw/dictView.jsp?ID=110966&la=0&powerMode=0",
    "调换": "https://dict.concised.moe.edu.tw/dictView.jsp?ID=8748&q=1",
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
    parser.add_argument("--owner", choices=("school_work", "services_travel"), help="Prepare one author worklist only; never freeze a partial batch")
    parser.add_argument("--combine-reviewed", action="store_true", help="Combine the two completed contextual worklists before freezing")
    args = parser.parse_args()
    assert pypinyin.__version__ == "0.55.0"
    if (OUT / "lexicon_batch_016.applied.json").exists():
        raise SystemExit("Batch 016 is applied; retain its frozen authoring baselines.")
    if args.owner:
        assert args.prepare, "--owner is prepare-only"
        assert not args.combine_reviewed, "A partial author worklist cannot be frozen"
        global FILES
        FILES = [OUT / f"batch016_{args.owner}_draft.txt"]
    selected, baselines, benchmarks, answers, authored, alternate, excluded_benchmark, duplicates, existing, quality = select()
    if args.prepare:
        pending = [f"{key}\t{word}\t{category}" for category, key, word, _ in selected if needs_review(word)]
        suffix = f"_{args.owner}" if args.owner else ""
        (OUT / f"batch016_pronunciation_pending{suffix}.tsv").write_text("# default pinyin\tword\tcategory; authoring review worklist only\n" + "\n".join(pending) + "\n", encoding="utf-8", newline="\n")
        (OUT / f"batch016_selected_preview{suffix}.tsv").write_text("# authoring preview only; not release data\n" + "\n".join(f"{key}\t{word}\t{category}" for category, key, word, _ in selected) + "\n", encoding="utf-8", newline="\n")
        print(json.dumps({"authored": authored, "selected": len(selected), "polyphone_review_required": len(pending), "duplicate_candidates": len(duplicates), "existing": len(existing), "alternate": len(alternate), "quality_rejected": len(quality), "categories": dict(Counter(row[0] for row in selected))}, ensure_ascii=False))
        return
    review_path = OUT / "batch016_pronunciation_review.tsv"
    required = {word for _, _, word, _ in selected if needs_review(word)}
    if args.combine_reviewed:
        assert not args.prepare, "Combining reviewed compounds is a freeze operation"
        combined = {}
        for owner in ("school_work", "services_travel"):
            worklist = OUT / f"batch016_{owner}_review.tsv"
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
    output = OUT / "lexicon_batch_016.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    overrides = {w: k for w, (k, _, _) in reviewed.items() if k != default_key(w)}
    metadata = {
        "batch": "lexicon_batch_016", "authoring_source": "first_party_authored", "external_lexicon_import": False,
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
        "pronunciation_review_file": review_path.name, "pronunciation_review_sha256": sha(review_path),
        "reviewer_worklists_sha256": {f"batch016_{name}_review.tsv": sha(OUT / f"batch016_{name}_review.tsv")
                                      for name in ("school_work", "services_travel")},
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
