#!/usr/bin/env python3
"""Manage the current user's LocalPinyinIME private lexicon."""

from __future__ import annotations

import argparse
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


HEADER = (
    "# LocalPinyinIME user lexicon\n"
    "# UTF-8 TSV format: pinyin<TAB>word<TAB>frequency\n"
    "# Lines beginning with # are comments. Keep this file private to the current Windows user.\n"
)


@dataclass(frozen=True, order=True)
class Entry:
    pinyin: str
    word: str
    frequency: int


@dataclass
class LexiconStats:
    source_rows: int = 0
    comment_rows: int = 0
    blank_rows: int = 0
    invalid_rows: int = 0
    duplicate_rows: int = 0
    valid_entries: int = 0


def default_lexicon_path() -> Path:
    local_app_data = os.environ.get("LOCALAPPDATA")
    if not local_app_data:
        local_app_data = str(Path.home() / "AppData" / "Local")
    return Path(local_app_data) / "LocalPinyinIME" / "user_lexicon.tsv"


def normalize_pinyin(value: str) -> str:
    return "".join(ch for ch in value.strip().lower() if ("a" <= ch <= "z") or ("0" <= ch <= "9"))


def reject_control_text(name: str, value: str) -> None:
    if not value.strip():
        raise ValueError(f"{name} must not be empty")
    if "\t" in value or "\r" in value or "\n" in value:
        raise ValueError(f"{name} must not contain tabs or newlines")


def ensure_file(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        path.write_text(HEADER, encoding="utf-8", newline="\n")


def parse_frequency(value: str) -> int | None:
    try:
        frequency = int(value.strip(), 10)
    except ValueError:
        return None
    if frequency < 0:
        return None
    return frequency


def read_entries(path: Path) -> tuple[list[Entry], LexiconStats]:
    ensure_file(path)
    stats = LexiconStats()
    entries: dict[tuple[str, str], Entry] = {}
    with path.open("r", encoding="utf-8", newline="") as handle:
        for raw_line in handle:
            stats.source_rows += 1
            line = raw_line.rstrip("\r\n")
            stripped = line.strip()
            if not stripped:
                stats.blank_rows += 1
                continue
            if stripped.startswith("#"):
                stats.comment_rows += 1
                continue
            parts = line.split("\t")
            if len(parts) != 3:
                stats.invalid_rows += 1
                continue
            pinyin = normalize_pinyin(parts[0])
            word = parts[1].strip()
            frequency = parse_frequency(parts[2])
            if not pinyin or not word or frequency is None:
                stats.invalid_rows += 1
                continue
            key = (pinyin, word)
            if key in entries:
                stats.duplicate_rows += 1
            entries[key] = Entry(pinyin, word, frequency)
    result = sorted(entries.values(), key=lambda entry: (entry.pinyin, entry.word))
    stats.valid_entries = len(result)
    return result, stats


def write_entries(path: Path, entries: Iterable[Entry]) -> None:
    ensure_file(path)
    ordered = sorted(entries, key=lambda entry: (entry.pinyin, entry.word))
    lines = [HEADER.rstrip("\n")]
    for entry in ordered:
        lines.append(f"{entry.pinyin}\t{entry.word}\t{entry.frequency}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def print_stats(stats: LexiconStats) -> None:
    print(f"source_rows: {stats.source_rows}")
    print(f"comment_rows: {stats.comment_rows}")
    print(f"blank_rows: {stats.blank_rows}")
    print(f"invalid_rows: {stats.invalid_rows}")
    print(f"duplicate_rows: {stats.duplicate_rows}")
    print(f"valid_entries: {stats.valid_entries}")


def command_validate(path: Path) -> int:
    _, stats = read_entries(path)
    print_stats(stats)
    return 0 if stats.invalid_rows == 0 else 1


def command_report(path: Path) -> int:
    _, stats = read_entries(path)
    print_stats(stats)
    return 0


def command_list(path: Path) -> int:
    entries, _ = read_entries(path)
    for entry in entries:
        print(f"{entry.pinyin}\t{entry.word}\t{entry.frequency}")
    return 0


def command_add(path: Path, pinyin: str, word: str, frequency: int) -> int:
    reject_control_text("pinyin", pinyin)
    reject_control_text("word", word)
    normalized_pinyin = normalize_pinyin(pinyin)
    if not normalized_pinyin:
        raise ValueError("pinyin must contain at least one ASCII letter or digit")
    if frequency < 0:
        raise ValueError("frequency must be a non-negative integer")

    entries, _ = read_entries(path)
    by_key = {(entry.pinyin, entry.word): entry for entry in entries}
    by_key[(normalized_pinyin, word.strip())] = Entry(normalized_pinyin, word.strip(), frequency)
    write_entries(path, by_key.values())
    print("updated_entries: 1")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Manage LocalPinyinIME private user lexicon.")
    parser.add_argument("--path", type=Path, default=default_lexicon_path(), help="Override user_lexicon.tsv path.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("validate", help="Validate the lexicon and report statistics.")
    subparsers.add_parser("list", help="List valid lexicon entries in stable order.")
    subparsers.add_parser("report", help="Report lexicon statistics.")

    add_parser = subparsers.add_parser("add", help="Add or update one lexicon entry.")
    add_parser.add_argument("--pinyin", required=True)
    add_parser.add_argument("--word", required=True)
    add_parser.add_argument("--frequency", required=True, type=int)
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        path = args.path
        if args.command == "validate":
            return command_validate(path)
        if args.command == "list":
            return command_list(path)
        if args.command == "report":
            return command_report(path)
        if args.command == "add":
            return command_add(path, args.pinyin, args.word, args.frequency)
    except ValueError as exc:
        print(f"error: {exc}")
        return 2
    parser.error("unknown command")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
