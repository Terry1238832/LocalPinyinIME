#!/usr/bin/env python3
"""Offline quality audit for the first-party LocalPinyinIME built-in lexicon."""

from __future__ import annotations

import argparse
import hashlib
import json
import random
import re
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


PINYIN_RE = re.compile(r"^[a-z]+$")
VALID_FREQUENCIES = {9000, 7000, 5000, 3000, 1000, 0}

EXACT_MATCH_BONUS = 100000
COMPLETE_SEGMENTATION_BONUS = 2000
EXCESSIVE_SEGMENTATION_PENALTY = 5000

BENCHMARK_CASES = [
    ("kaihui", "\u5f00\u4f1a"),
    ("fangjia", "\u653e\u5047"),
    ("biancheng", "\u53d8\u6210"),
    ("shenle", "\u795e\u4e86"),
    ("zhiyou", "\u53ea\u6709"),
    ("bucuo", "\u4e0d\u9519"),
    ("bianji", "\u7f16\u8f91"),
    ("huode", "\u83b7\u5f97"),
    ("jintian", "\u4eca\u5929"),
    ("jintiantianqi", "\u4eca\u5929\u5929\u6c14"),
    ("jintiantianqihenhao", "\u4eca\u5929\u5929\u6c14\u5f88\u597d"),
    ("woxiang", "\u6211\u60f3"),
    ("woxiangchiwanfan", "\u6211\u60f3\u5403\u665a\u996d"),
    ("qingbangwo", "\u8bf7\u5e2e\u6211"),
    ("qingbangwokankan", "\u8bf7\u5e2e\u6211\u770b\u770b"),
    ("zhegeshi", "\u8fd9\u4e2a\u662f"),
    ("zhegeshishenme", "\u8fd9\u4e2a\u662f\u4ec0\u4e48"),
    ("woxianzai", "\u6211\u73b0\u5728"),
    ("woxianzaiyouyidianmang", "\u6211\u73b0\u5728\u6709\u4e00\u70b9\u5fd9"),
]


@dataclass(frozen=True)
class Entry:
    pinyin: str
    word: str
    frequency: int
    category: str
    source: str
    line_number: int


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def has_control_character(value: str) -> bool:
    return any((ord(ch) < 32 and ch not in "\t\r\n") or ord(ch) == 127 for ch in value)


def parse_tsv(path: Path, source: str) -> tuple[list[Entry], dict[str, int], list[dict[str, object]]]:
    entries: list[Entry] = []
    category_counts: Counter[str] = Counter()
    invalid_rows: list[dict[str, object]] = []
    current_category = "uncategorized"

    with path.open("r", encoding="utf-8", newline="") as handle:
        for line_number, raw_line in enumerate(handle, start=1):
            line = raw_line.rstrip("\r\n")
            stripped = line.strip()
            if not stripped:
                continue
            if stripped.startswith("#"):
                prefix = "# category:"
                if stripped.lower().startswith(prefix):
                    current_category = stripped[len(prefix):].strip() or "uncategorized"
                continue

            fields = line.split("\t")
            if len(fields) != 3:
                invalid_rows.append({"line": line_number, "reason": "field_count"})
                continue
            pinyin, word, frequency_text = (field.strip() for field in fields)
            reason = ""
            if not pinyin or not PINYIN_RE.fullmatch(pinyin):
                reason = "invalid_pinyin"
            elif not word or "\t" in word or "\n" in word or "\r" in word or has_control_character(word):
                reason = "invalid_word"
            else:
                try:
                    frequency = int(frequency_text, 10)
                except ValueError:
                    reason = "invalid_frequency"
                    frequency = -1
                if not reason and frequency < 0:
                    reason = "negative_frequency"

            if reason:
                invalid_rows.append({"line": line_number, "reason": reason})
                continue

            entry = Entry(
                pinyin=pinyin,
                word=word,
                frequency=frequency,
                category=current_category,
                source=source,
                line_number=line_number,
            )
            entries.append(entry)
            category_counts[current_category] += 1

    return entries, dict(category_counts), invalid_rows


def merge_dictionary(core_entries: Iterable[Entry], local_entries: Iterable[Entry]) -> dict[str, list[Entry]]:
    merged: dict[tuple[str, str], Entry] = {}
    for entry in core_entries:
        merged.setdefault((entry.pinyin, entry.word), entry)
    for entry in local_entries:
        merged[(entry.pinyin, entry.word)] = entry

    by_pinyin: dict[str, list[Entry]] = defaultdict(list)
    for entry in merged.values():
        by_pinyin[entry.pinyin].append(entry)
    for pinyin in by_pinyin:
        by_pinyin[pinyin].sort(key=lambda item: item.frequency, reverse=True)
    return by_pinyin


def word_length_distribution(entries: Iterable[Entry]) -> dict[str, int]:
    counts = Counter()
    for entry in entries:
        length = len(entry.word)
        if length == 1:
            counts["single"] += 1
        elif length == 2:
            counts["two"] += 1
        elif length == 3:
            counts["three"] += 1
        elif length == 4:
            counts["four"] += 1
        else:
            counts["five_plus"] += 1
    return dict(counts)


def pinyin_length_distribution(entries: Iterable[Entry]) -> dict[str, int]:
    ranges = {
        "1_4": 0,
        "5_8": 0,
        "9_12": 0,
        "13_16": 0,
        "17_24": 0,
        "25_plus": 0,
    }
    for entry in entries:
        length = len(entry.pinyin)
        if length <= 4:
            ranges["1_4"] += 1
        elif length <= 8:
            ranges["5_8"] += 1
        elif length <= 12:
            ranges["9_12"] += 1
        elif length <= 16:
            ranges["13_16"] += 1
        elif length <= 24:
            ranges["17_24"] += 1
        else:
            ranges["25_plus"] += 1
    return ranges


def find_duplicates(entries: Iterable[Entry]) -> list[dict[str, object]]:
    seen: dict[tuple[str, str], Entry] = {}
    duplicates: list[dict[str, object]] = []
    for entry in entries:
        key = (entry.pinyin, entry.word)
        previous = seen.get(key)
        if previous is not None:
            duplicates.append({
                "pinyin": entry.pinyin,
                "line": entry.line_number,
                "previous_line": previous.line_number,
            })
        else:
            seen[key] = entry
    return duplicates


def find_near_duplicates(entries: list[Entry], limit: int = 60) -> list[dict[str, object]]:
    by_word: dict[str, list[Entry]] = defaultdict(list)
    by_pinyin: dict[str, list[Entry]] = defaultdict(list)
    for entry in entries:
        by_word[entry.word].append(entry)
        by_pinyin[entry.pinyin].append(entry)

    findings: list[dict[str, object]] = []
    for word, grouped in by_word.items():
        pinyins = sorted({entry.pinyin for entry in grouped})
        if len(pinyins) > 1:
            findings.append({"kind": "same_word_multiple_pinyin", "word": word, "pinyins": pinyins[:8]})
    for pinyin, grouped in by_pinyin.items():
        words = sorted({entry.word for entry in grouped})
        if len(words) > 6:
            findings.append({"kind": "many_words_same_pinyin", "pinyin": pinyin, "word_count": len(words)})
    return findings[:limit]


def repeated_bigram(word: str) -> bool:
    if len(word) < 4:
        return False
    return any(word[index:index + 2] == word[index + 2:index + 4] for index in range(len(word) - 3))


def suspicious_entries(entries: list[Entry]) -> dict[str, list[dict[str, object]]]:
    abnormal_long = [
        entry for entry in entries
        if len(entry.word) >= 9 or len(entry.pinyin) >= 28
    ]
    templated = [
        entry for entry in entries
        if entry.category in {"daily_phrases", "common_connectors"} and len(entry.word) >= 8
    ]
    repeated = [entry for entry in entries if repeated_bigram(entry.word)]
    unusual_frequency = [
        entry for entry in entries
        if entry.frequency not in VALID_FREQUENCIES or (len(entry.word) >= 8 and entry.frequency >= 9000)
    ]

    def serialize(items: Iterable[Entry], limit: int = 80) -> list[dict[str, object]]:
        return [
            {
                "pinyin": entry.pinyin,
                "word": entry.word,
                "frequency": entry.frequency,
                "category": entry.category,
                "line": entry.line_number,
            }
            for entry in list(items)[:limit]
        ]

    return {
        "abnormal_long": serialize(abnormal_long),
        "templated_long_phrase_review": serialize(templated),
        "repeated_bigram_review": serialize(repeated),
        "unusual_frequency_review": serialize(unusual_frequency),
    }


def matching_pinyins_at(dictionary: dict[str, list[Entry]], value: str, offset: int) -> list[str]:
    matches = [pinyin for pinyin in dictionary if value.startswith(pinyin, offset)]
    matches.sort(key=len, reverse=True)
    return matches


def best_segmentation(dictionary: dict[str, list[Entry]], pinyin: str) -> dict[str, object] | None:
    paths: list[dict[str, object] | None] = [None] * (len(pinyin) + 1)
    paths[0] = {"score": 0, "parts": []}

    for offset in range(len(pinyin)):
        current = paths[offset]
        if current is None:
            continue
        for part_pinyin in matching_pinyins_at(dictionary, pinyin, offset):
            entries = dictionary.get(part_pinyin, [])
            if not entries:
                continue
            best_entry = entries[0]
            next_index = offset + len(part_pinyin)
            parts = list(current["parts"]) + [best_entry]
            score = int(current["score"]) + best_entry.frequency
            previous = paths[next_index]
            if previous is None or score > int(previous["score"]) or (
                score == int(previous["score"]) and len(parts) < len(previous["parts"])
            ):
                paths[next_index] = {"score": score, "parts": parts}
    return paths[-1]


def lookup_simulated(dictionary: dict[str, list[Entry]], pinyin: str) -> dict[str, object]:
    candidates: list[dict[str, object]] = []
    exact_entries = dictionary.get(pinyin, [])
    for entry in exact_entries:
        candidates.append({
            "text": entry.word,
            "source": "exact",
            "exact_hit": True,
            "single_entry": True,
            "segmented": False,
            "score": entry.frequency + EXACT_MATCH_BONUS + COMPLETE_SEGMENTATION_BONUS,
            "parts": [entry.word],
        })

    segmented = best_segmentation(dictionary, pinyin)
    if segmented is not None and len(segmented["parts"]) > 1:
        parts = list(segmented["parts"])
        candidates.append({
            "text": "".join(entry.word for entry in parts),
            "source": "segmented",
            "exact_hit": False,
            "single_entry": False,
            "segmented": True,
            "score": int(segmented["score"]) + COMPLETE_SEGMENTATION_BONUS -
                     (len(parts) - 1) * EXCESSIVE_SEGMENTATION_PENALTY,
            "parts": [entry.word for entry in parts],
        })

    if not candidates:
        return {
            "text": pinyin,
            "source": "fallback",
            "exact_hit": False,
            "single_entry": False,
            "segmented": False,
            "score": 1,
            "parts": [pinyin],
        }
    candidates.sort(key=lambda item: int(item["score"]), reverse=True)
    return candidates[0]


def ranking_benchmarks(dictionary: dict[str, list[Entry]]) -> list[dict[str, object]]:
    results: list[dict[str, object]] = []
    for pinyin, expected in BENCHMARK_CASES:
        exact_hit = any(entry.word == expected for entry in dictionary.get(pinyin, []))
        actual = lookup_simulated(dictionary, pinyin)
        results.append({
            "pinyin": pinyin,
            "target_first_candidate": expected,
            "actual_first_candidate": actual["text"],
            "passed": actual["text"] == expected,
            "complete_dictionary_hit": exact_hit,
            "single_complete_entry": actual["source"] == "exact",
            "multi_entry_segmentation": actual["source"] == "segmented",
            "ranking_reason": actual["source"],
            "score": actual["score"],
            "parts": actual["parts"],
        })
    return results


def stable_samples(entries: list[Entry], seed: int) -> dict[str, list[dict[str, object]]]:
    rng = random.Random(seed)

    def pick(items: list[Entry], count: int) -> list[dict[str, object]]:
        if not items:
            return []
        selected = rng.sample(items, min(count, len(items)))
        selected.sort(key=lambda entry: (entry.category, entry.pinyin, entry.word))
        return [
            {
                "category": entry.category,
                "pinyin": entry.pinyin,
                "word": entry.word,
                "frequency": entry.frequency,
            }
            for entry in selected
        ]

    by_category: dict[str, list[Entry]] = defaultdict(list)
    for entry in entries:
        by_category[entry.category].append(entry)

    return {
        "by_category": [
            sample
            for category in sorted(by_category)
            for sample in pick(by_category[category], 2)
        ],
        "high_frequency": pick([entry for entry in entries if entry.frequency >= 9000], 12),
        "long_phrases": pick([entry for entry in entries if len(entry.word) >= 5], 12),
    }


def make_report(data: dict[str, object]) -> str:
    stats = data["stats"]
    risk = data["risk_summary"]
    lines = [
        "# LocalPinyinIME First-Party Lexicon v2 Audit",
        "",
        "This offline report is a deterministic review aid. It does not claim formal linguistic correctness.",
        "",
        "## Summary",
        f"- Valid entries: {stats['valid_entries']}",
        f"- SHA-256: {stats['sha256']}",
        f"- Duplicate pinyin+word rows: {risk['duplicate_rows']}",
        f"- Invalid rows: {risk['invalid_rows']}",
        f"- Suggested keep: {risk['suggested_keep_count']}",
        f"- Suggested manual review: {risk['suggested_manual_review_count']}",
        f"- Suggested remove: {risk['suggested_remove_count']}",
        "",
        "## Ranking Benchmarks",
    ]
    for item in data["ranking_benchmarks"]:
        status = "PASS" if item["passed"] else "FAIL"
        lines.append(
            f"- {status} `{item['pinyin']}`: expected `{item['target_first_candidate']}`, "
            f"actual `{item['actual_first_candidate']}`, reason `{item['ranking_reason']}`"
        )
    lines.extend([
        "",
        "## Category Counts",
    ])
    for category, count in sorted(data["category_counts"].items()):
        lines.append(f"- `{category}`: {count}")
    lines.extend([
        "",
        "## Review Notes",
        "- Long and templated phrase findings are review prompts, not automatic removal decisions.",
        "- The tool reads only project dictionary resources and does not read user private lexicons.",
    ])
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--core", type=Path, required=True)
    parser.add_argument("--dictionary", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--sources", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=20260701)
    args = parser.parse_args()

    core_entries, _, core_invalid = parse_tsv(args.core, "core")
    local_entries, category_counts, local_invalid = parse_tsv(args.dictionary, "local_core")
    metadata = json.loads(args.metadata.read_text(encoding="utf-8"))
    sources = json.loads(args.sources.read_text(encoding="utf-8"))
    merged = merge_dictionary(core_entries, local_entries)
    duplicates = find_duplicates(local_entries)
    suspicious = suspicious_entries(local_entries)
    benchmarks = ranking_benchmarks(merged)
    digest = sha256_file(args.dictionary)

    word_lengths = word_length_distribution(local_entries)
    pinyin_lengths = pinyin_length_distribution(local_entries)
    frequency_distribution = dict(sorted(Counter(entry.frequency for entry in local_entries).items()))
    daily_five_plus = sum(
        1 for entry in local_entries
        if entry.category == "daily_phrases" and len(entry.word) >= 5
    )
    metadata_categories = metadata.get("category_counts", {})
    category_mismatch = {
        category: {"metadata": metadata_categories.get(category), "actual": count}
        for category, count in category_counts.items()
        if int(metadata_categories.get(category, -1)) != count
    }
    benchmark_failures = [item for item in benchmarks if not item["passed"]]

    review_count = (
        len(suspicious["abnormal_long"]) +
        len(suspicious["templated_long_phrase_review"]) +
        len(suspicious["repeated_bigram_review"]) +
        len(suspicious["unusual_frequency_review"])
    )
    remove_count = len(core_invalid) + len(local_invalid) + len(duplicates)

    report = {
        "format_version": 1,
        "seed": args.seed,
        "stats": {
            "valid_entries": len(local_entries),
            "sha256": digest,
            "metadata_entry_count": metadata.get("entry_count"),
            "metadata_sha256": metadata.get("sha256"),
            "source_count": len(sources.get("sources", [])),
        },
        "category_counts": dict(sorted(category_counts.items())),
        "metadata_category_mismatch": category_mismatch,
        "word_length_distribution": word_lengths,
        "pinyin_length_distribution": pinyin_lengths,
        "frequency_distribution": frequency_distribution,
        "daily_phrases_five_plus_count": daily_five_plus,
        "duplicate_rows": duplicates,
        "near_duplicate_review": find_near_duplicates(local_entries),
        "suspicious": suspicious,
        "samples": stable_samples(local_entries, args.seed),
        "ranking_benchmarks": benchmarks,
        "risk_summary": {
            "invalid_rows": len(core_invalid) + len(local_invalid),
            "duplicate_rows": len(duplicates),
            "ranking_failures": len(benchmark_failures),
            "suggested_keep_count": len(local_entries) - remove_count,
            "suggested_manual_review_count": review_count,
            "suggested_remove_count": remove_count,
        },
    }

    args.output_dir.mkdir(parents=True, exist_ok=True)
    json_path = args.output_dir / "first_party_lexicon_v2_audit.json"
    markdown_path = args.output_dir / "first_party_lexicon_v2_audit.md"
    json_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    markdown_path.write_text(make_report(report), encoding="utf-8", newline="\n")

    hard_failures = []
    if local_invalid or core_invalid:
        hard_failures.append("invalid_rows")
    if duplicates:
        hard_failures.append("duplicate_rows")
    if category_mismatch:
        hard_failures.append("category_mismatch")
    if int(metadata.get("entry_count", -1)) != len(local_entries):
        hard_failures.append("metadata_entry_count")
    if str(metadata.get("sha256", "")).upper() != digest:
        hard_failures.append("metadata_sha256")
    if benchmark_failures:
        hard_failures.append("ranking_benchmarks")

    print(f"audit_json: {json_path}")
    print(f"audit_markdown: {markdown_path}")
    print(f"valid_entries: {len(local_entries)}")
    print(f"suggested_manual_review_count: {review_count}")
    if hard_failures:
        print("audit_failed: " + ",".join(hard_failures))
        return 1
    print("audit_passed: true")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
