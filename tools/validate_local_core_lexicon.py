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
    source_policy = str(metadata["source_policy"]).lower()
    for token in ("third-party", "rime", "sogou", "baidu", "microsoft", "google"):
        if token not in source_policy:
            raise ValueError(f"metadata source_policy does not mention {token}")
    frequency_policy = str(metadata["frequency_policy"]).lower()
    if "relative" not in frequency_policy or "not" not in frequency_policy or "corpus" not in frequency_policy:
        raise ValueError("metadata frequency_policy must describe relative non-corpus weights")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--min-entries", type=int, default=1500)
    parser.add_argument("--max-entries", type=int, default=2500)
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
