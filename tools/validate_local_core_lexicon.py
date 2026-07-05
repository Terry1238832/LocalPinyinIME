#!/usr/bin/env python3
"""Validate the project-maintained LocalPinyinIME built-in lexicon."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path


PINYIN_RE = re.compile(r"^[a-z]+$")

REQUIRED_ENTRIES = {
    "kaihui": "\u5f00\u4f1a",
    "fangjia": "\u653e\u5047",
    "biancheng": "\u53d8\u6210",
    "shenle": "\u795e\u4e86",
    "zhiyou": "\u53ea\u6709",
    "bucuo": "\u4e0d\u9519",
    "bianji": "\u7f16\u8f91",
    "huode": "\u83b7\u5f97",
    "an": "\u6309",
    "anxia": "\u6309\u4e0b",
    "xiamian": "\u4e0b\u9762",
    "mian": "\u9762",
    "mianfei": "\u514d\u8d39",
    "mianfen": "\u9762\u7c89",
    "shouxian": "\u9996\u5148",
    "xian": "\u5148",
    "xianjiuzheng": "\u5148\u7ea0\u6b63",
    "yixie": "\u4e00\u4e9b",
    "qisiwole": "\u6c14\u6b7b\u6211\u4e86",
    "nihao": "你好",
    "xiexie": "谢谢",
    "zaijian": "再见",
    "jintian": "今天",
    "mingtian": "明天",
    "xianzai": "现在",
    "xuexiao": "学校",
    "daxue": "大学",
    "diannao": "电脑",
    "shouji": "手机",
    "wangluo": "网络",
    "xuexi": "学习",
    "kaoshi": "考试",
    "zuoye": "作业",
    "nihaoshijie": "你好世界",
    "woxiangqubeijing": "我想去北京",
    "jintiantianqihenhao": "今天天气很好",
    "woxiangchiwanfan": "我想吃晚饭",
    "qingbangwokankan": "请帮我看看",
    "zhegeshishenme": "这个是什么",
    "woxianzaiyouyidianmang": "我现在有一点忙",
}

REQUIRED_CATEGORIES = {
    "common_words_2char",
    "common_words_3char",
    "common_words_4plus",
    "daily_phrases",
    "spoken_patterns",
    "time_weather",
    "shopping_food_transport",
    "study_work_technology",
    "common_actions",
    "common_questions",
    "common_connectors",
    "targeted_product_baseline_036",
    "targeted_input_mode_037",
}

REQUIRED_METADATA_FIELDS = {
    "format_version",
    "dictionary_name",
    "maintainer",
    "intended_distribution",
    "entry_count",
    "sha256",
    "generated_at",
    "source_policy",
    "frequency_policy",
    "review_status",
}


@dataclass
class ValidationStats:
    source_rows: int = 0
    comment_rows: int = 0
    blank_rows: int = 0
    valid_entries: int = 0
    invalid_rows: int = 0
    duplicate_rows: int = 0


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def validate_lexicon(path: Path, min_entries: int, max_entries: int) -> tuple[ValidationStats, dict[str, str]]:
    stats = ValidationStats()
    seen: set[tuple[str, str]] = set()
    entries: dict[str, set[str]] = {}
    category_counts: dict[str, int] = {}
    current_category = ""

    with path.open("r", encoding="utf-8", newline="") as handle:
        for line_number, raw_line in enumerate(handle, start=1):
            stats.source_rows += 1
            line = raw_line.rstrip("\r\n")
            stripped = line.strip()
            if not stripped:
                stats.blank_rows += 1
                continue
            if stripped.startswith("#"):
                stats.comment_rows += 1
                prefix = "# category:"
                if stripped.lower().startswith(prefix):
                    current_category = stripped[len(prefix):].strip()
                    category_counts.setdefault(current_category, 0)
                continue

            fields = line.split("\t")
            if len(fields) != 3:
                raise ValueError(f"invalid field count at line {line_number}")
            pinyin, word, frequency_text = (field.strip() for field in fields)
            if not pinyin or not word:
                raise ValueError(f"empty pinyin or word at line {line_number}")
            if not PINYIN_RE.fullmatch(pinyin):
                raise ValueError(f"invalid pinyin at line {line_number}")
            try:
                frequency = int(frequency_text, 10)
            except ValueError as exc:
                raise ValueError(f"invalid frequency at line {line_number}") from exc
            if frequency < 0:
                raise ValueError(f"negative frequency at line {line_number}")

            key = (pinyin, word)
            if key in seen:
                raise ValueError(f"duplicate pinyin+word at line {line_number}")
            seen.add(key)
            entries.setdefault(pinyin, set()).add(word)
            stats.valid_entries += 1
            if current_category:
                category_counts[current_category] = category_counts.get(current_category, 0) + 1

    if not (min_entries <= stats.valid_entries <= max_entries):
        raise ValueError(f"valid entry count out of range: {stats.valid_entries}")

    missing = {
        pinyin: word
        for pinyin, word in REQUIRED_ENTRIES.items()
        if word not in entries.get(pinyin, set())
    }
    if missing:
        names = ", ".join(sorted(missing))
        raise ValueError(f"missing required entries: {names}")
    missing_categories = {
        category
        for category in REQUIRED_CATEGORIES
        if category_counts.get(category, 0) <= 0
    }
    if missing_categories:
        raise ValueError(f"missing required categories: {', '.join(sorted(missing_categories))}")
    return stats, {pinyin: next(iter(words)) for pinyin, words in entries.items()}


def validate_metadata(metadata_path: Path, entry_count: int, expected_sha256: str) -> None:
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    missing = REQUIRED_METADATA_FIELDS.difference(metadata)
    if missing:
        raise ValueError(f"metadata missing fields: {', '.join(sorted(missing))}")
    if int(metadata["entry_count"]) != entry_count:
        raise ValueError("metadata entry_count does not match TSV")
    if str(metadata["sha256"]).upper() != expected_sha256:
        raise ValueError("metadata sha256 does not match TSV")
    if str(metadata.get("review_status", "")).lower().find("approved") < 0:
        raise ValueError("metadata review_status must state approved")
    category_counts = metadata.get("category_counts")
    if not isinstance(category_counts, dict):
        raise ValueError("metadata category_counts must be present")
    missing_categories = {
        category
        for category in REQUIRED_CATEGORIES
        if int(category_counts.get(category, 0)) <= 0
    }
    if missing_categories:
        raise ValueError(f"metadata category_counts missing: {', '.join(sorted(missing_categories))}")
    source_policy = str(metadata["source_policy"]).lower()
    for token in ("third-party", "rime", "sogou", "baidu", "microsoft", "google", "github"):
        if token not in source_policy:
            raise ValueError(f"metadata source_policy does not mention {token}")
    frequency_policy = str(metadata["frequency_policy"]).lower()
    for token in ("relative", "not", "corpus", "internet", "search", "commercial"):
        if token not in frequency_policy:
            raise ValueError("metadata frequency_policy must describe relative non-corpus weights")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--min-entries", type=int, default=8000)
    parser.add_argument("--max-entries", type=int, default=12000)
    args = parser.parse_args()

    stats, _ = validate_lexicon(args.path, args.min_entries, args.max_entries)
    digest = sha256_file(args.path)
    validate_metadata(args.metadata, stats.valid_entries, digest)
    print(f"valid_entries: {stats.valid_entries}")
    print(f"source_rows: {stats.source_rows}")
    print(f"comment_rows: {stats.comment_rows}")
    print(f"blank_rows: {stats.blank_rows}")
    print(f"sha256: {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
