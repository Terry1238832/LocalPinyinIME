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
FILES = [OUT / f"batch018_{name}_draft.txt" for name in ("formal_words", "software_work")]
WEIGHTS = {
    'formal_argument_structure': 5500,
    'formal_reasoning_and_qualification': 5500,
    'formal_sequence_and_connection': 6000,
    'formal_evidence_and_inference': 5000,
    'formal_discussion_and_contrast': 5500,
    'formal_scope_and_emphasis': 5500,
    'narrative_perception_and_attention': 5500,
    'narrative_movement_and_posture': 5500,
    'narrative_hands_and_small_actions': 5500,
    'narrative_speech_and_reaction': 5500,
    'narrative_expression_and_emotion': 5500,
    'narrative_thought_and_realization': 5500,
    'narrative_instinct_and_hesitation': 5500,
    'narrative_silence_and_waiting': 5500,
    'descriptive_light_and_weather': 5000,
    'descriptive_season_and_landscape': 5000,
    'descriptive_flowers_and_surroundings': 5000,
    'descriptive_sound_and_senses': 5000,
    'descriptive_appearance_and_texture': 5000,
    'descriptive_mood_and_atmosphere': 5500,
    'narrative_time_and_change': 5500,
    'software_install_update': 5500,
    'software_navigation': 5500,
    'software_preferences': 5500,
    'software_account_access': 5500,
    'software_browser_downloads': 5500,
    'software_files_archives': 5500,
    'software_cloud_sharing': 5500,
    'work_text_editing': 5500,
    'work_document_layout': 5500,
    'work_document_review': 5500,
    'work_spreadsheet_cells': 5500,
    'work_spreadsheet_filter': 5500,
    'work_presentations': 5500,
    'work_remote_meetings': 5500,
    'work_data_cleaning': 5500,
    'work_spreadsheet_calculation': 5000,
    'work_charts_reporting': 5000,
    'software_media_export': 5000,
    'software_code_editors': 5000,
    'software_version_control': 5000,
    'software_review_testing': 5000,
    'software_build_debugging': 5000,
    'work_database_queries': 5000,
    'software_automation': 5000,
    'work_digital_messages': 6000,
    'software_errors_help': 6000,
}
QUALITY_EXCLUSIONS: dict[str, str] = {}
PRONUNCIATION_REFERENCE_LINKS: dict[str, str] = {
    '地（动作修饰结构助词de）': 'https://dict.mini.moe.edu.tw/SearchIndex/searchResult?dictSearchField=%E5%9C%B0&searchType=one',
    '著／着（持续状态zhe）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=29560&q=1',
    '长（持续时间或长度chang）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=32100&la=0&powerMode=0',
    '落（下移luo）': 'https://www.cns11643.gov.tw/wordView.jsp?ID=91695',
    '褪（tun及tui合法又音）': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=2889&la=1&powerMode=0',
    '褪色（大陆院校单词标音tuise，不导入正文或词表）': 'https://www.scst.edu.cn/yywz/50/8d/c1942a20621/page.htm',
    '啁啾': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=45656&la=0&powerMode=0',
    '哼（鼻音声响heng）': 'https://dict.mini.moe.edu.tw/SearchIndex/word_detail?breadcrumbs=Search_%E5%93%BC_one&dictSearchField=%E5%93%BC&wordID=D0002682',
    '行（文本行、代码行及数据行hang）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=19765&la=0&powerMode=0',
    '重（重新执行或重复chong）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=33068&la=0&powerMode=0',
    '弹（弹出窗口tan）': 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=2045&q=1&word=%E5%BD%88',
    '转（改变格式zhuan）': 'https://dict.concised.moe.edu.tw/dictView.jsp?ID=30953&la=0&powerMode=0',
    '钥（yao语音及yue读音同义）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=47761&la=1',
    '密钥（大陆院校单词标音yue，不导入正文或词表）': 'https://yywzw.cqcvc.edu.cn/info.thtml?cid=51719&pn=&t=11478',
    '卡住（大陆院校单词标音qia，不导入正文或词表）': 'https://www.succ.edu.cn/yywz/Home/News/view/xwid/0a6844f71ed449a14c9aa7337f94369f',
    '卡（ka读音义项包括夹住堵塞，合法地区读法）': 'https://dict.variants.moe.edu.tw/dictView.jsp?ID=4805&q=1',
}
READING_VARIANT_WORDS: set[str] = {"慢慢褪去", "逐渐褪去", "安全密钥", "密钥登录", "访问密钥", "软件卡住了"}



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
    parser.add_argument("--owner", choices=("formal_words", "software_work"), help="Prepare one author worklist only; never freeze a partial batch")
    parser.add_argument("--combine-reviewed", action="store_true", help="Combine the two completed contextual worklists before freezing")
    args = parser.parse_args()
    assert pypinyin.__version__ == "0.55.0"
    if (OUT / "lexicon_batch_018.applied.json").exists():
        raise SystemExit("Batch 018 is applied; retain its frozen authoring baselines.")
    if args.owner:
        assert args.prepare, "--owner is prepare-only"
        assert not args.combine_reviewed, "A partial author worklist cannot be frozen"
        global FILES
        FILES = [OUT / f"batch018_{args.owner}_draft.txt"]
    selected, baselines, benchmarks, answers, authored, alternate, excluded_benchmark, duplicates, existing, quality = select()
    if args.prepare:
        pending = [f"{key}\t{word}\t{category}" for category, key, word, _ in selected if needs_review(word)]
        suffix = f"_{args.owner}" if args.owner else ""
        (OUT / f"batch018_pronunciation_pending{suffix}.tsv").write_text("# default pinyin\tword\tcategory; authoring review worklist only\n" + "\n".join(pending) + "\n", encoding="utf-8", newline="\n")
        (OUT / f"batch018_selected_preview{suffix}.tsv").write_text("# authoring preview only; not release data\n" + "\n".join(f"{key}\t{word}\t{category}" for category, key, word, _ in selected) + "\n", encoding="utf-8", newline="\n")
        print(json.dumps({"authored": authored, "selected": len(selected), "polyphone_review_required": len(pending), "duplicate_candidates": len(duplicates), "existing": len(existing), "alternate": len(alternate), "quality_rejected": len(quality), "categories": dict(Counter(row[0] for row in selected))}, ensure_ascii=False))
        return
    review_path = OUT / "batch018_pronunciation_review.tsv"
    required = {word for _, _, word, _ in selected if needs_review(word)}
    if args.combine_reviewed:
        assert not args.prepare, "Combining reviewed compounds is a freeze operation"
        combined = {}
        for owner in ("formal_words", "software_work"):
            worklist = OUT / f"batch018_{owner}_review.tsv"
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
    output = OUT / "lexicon_batch_018.tsv"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    overrides = {w: k for w, (k, _, _) in reviewed.items() if k != default_key(w)}
    variant_choices = {w: k for w, k in overrides.items() if w in READING_VARIANT_WORDS}
    metadata = {
        "batch": "lexicon_batch_018", "authoring_source": "first_party_authored", "external_lexicon_import": False,
        "license": "MIT", "pronunciation_tool": "pypinyin 0.55.0 authoring-only; individually reviewed compounds",
        "authoring_files_sha256": {p.name: sha(p) for p in FILES}, "authored_candidates": authored,
        "authoring_tools_sha256": {name: sha(OUT / name) for name in ("build_lexicon_batch_018.py", "apply_lexicon_batch_018.py", "validate_lexicon_batch_018_native.py")},
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
        "reading_variant_policy": "Six main-key choices are legitimate reading variants, not impossible-reading errors: the two 褪去 compounds use mainland everyday tui while the primary dictionary also records tun; the three 密钥 compounds use formal technical yue while yao is a legitimate spoken reading; 软件卡住了 uses mainland blocked-state qia while ka is a legitimate regional or software-colloquial reading. 软件卡死了 retains colloquial ka. No alternate aliases are added for existing displays.",
        "pronunciation_review_file": review_path.name, "pronunciation_review_sha256": sha(review_path),
        "reviewer_worklists_sha256": {f"batch018_{name}_review.tsv": sha(OUT / f"batch018_{name}_review.tsv")
                                      for name in ("formal_words", "software_work")},
        "pronunciation_review_record_method": "Each author read every required compound with its draft key in context before recording the row. Inline scripts only formatted the checked choices and notes; default agreement alone was not marked as review.",
        "polyphone_review_detection": "Any character with more than one toneless normal-style possible reading in pinned pypinyin 0.55.0; each compound is reviewed in context.",
        "pronunciation_review_reference_links": PRONUNCIATION_REFERENCE_LINKS,
        "pronunciation_reference_scope": "Individual word pronunciation definitions or explicit single-word/single-character pronunciation notations only; no external lexicon, article text, definition text, or usage examples imported.",
        "validation": {"all_outputs_two_to_six_han": True, "format_valid": True,
            "per_character_possible_readings_valid": True, "canonical_or_previous_batch_collisions": 0,
            "automatic_default_readings_changed": len(overrides),
            "semantic_pronunciation_corrections": len(overrides) - len(variant_choices),
            "reading_variant_choices": len(variant_choices), "polyphone_reviewed_display_words": len(required)},
        "author_revision_provenance": {"formal_words": {"early_candidates": 2115, "quality_removed_before_prepare": 35, "naturalness_rewrites": 12, "final_draft_candidates": 2080}, "software_work": {"early_candidates": 2417, "quality_removed_before_prepare": 74, "naturalness_rewrites": 28, "existing_filtered_before_final_draft": 356, "duplicates_filtered_before_final_draft": 2, "final_draft_candidates": 1985}},
        "candidate_count_scope": "Authored candidate count refers to the final frozen drafts after ordinary author revisions, not an unrevised early brainstorming count.",
        "canonical_dictionaries_modified": False, "line_endings": "UTF-8 LF",
    }
    output.with_suffix(".json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("new_entries", "sha256", "category_counts", "length_distribution", "validation")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
