#!/usr/bin/env python3
"""Merge a reviewed first-party patch into local_core_zh_pinyin.tsv.

The tool is intentionally offline and conservative.  It never reads the
current user's private lexicon or learning database.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import tempfile
from dataclasses import dataclass
from datetime import date
from pathlib import Path


PINYIN_RE = re.compile(r"^[a-z]+$")
SOURCE_ID = "first_party_manual_patch"


@dataclass(frozen=True)
class Entry:
    pinyin: str
    word: str
    frequency: int
    category: str
    line_number: int
    line_index: int = -1


@dataclass
class MergeReport:
    added: int = 0
    skipped: int = 0
    updated: int = 0
    conflicts: int = 0
    invalid_rows: int = 0


def project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def default_dictionary_path() -> Path:
    return project_root() / "resources/dictionary/local_core_zh_pinyin.tsv"


def default_metadata_path() -> Path:
    return project_root() / "resources/dictionary/local_core_zh_pinyin.metadata.json"


def validate_entry_fields(pinyin: str, word: str, frequency_text: str, line_number: int) -> Entry:
    if not pinyin or not PINYIN_RE.fullmatch(pinyin):
        raise ValueError(f"line {line_number}: invalid pinyin")
    if not word or "\t" in word or "\r" in word or "\n" in word:
        raise ValueError(f"line {line_number}: invalid word")
    try:
        frequency = int(frequency_text, 10)
    except ValueError as exc:
        raise ValueError(f"line {line_number}: invalid frequency") from exc
    if frequency < 0:
        raise ValueError(f"line {line_number}: negative frequency")
    return Entry(pinyin=pinyin, word=word, frequency=frequency, category="", line_number=line_number)


def parse_tsv(path: Path, *, keep_line_indexes: bool) -> tuple[list[str], list[Entry]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    entries: list[Entry] = []
    seen: set[tuple[str, str]] = set()
    current_category = "uncategorized"

    for index, line in enumerate(lines):
        line_number = index + 1
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
            raise ValueError(f"line {line_number}: malformed TSV row")
        pinyin, word, frequency_text = (field.strip() for field in fields)
        entry = validate_entry_fields(pinyin, word, frequency_text, line_number)
        key = (entry.pinyin, entry.word)
        if key in seen:
            raise ValueError(f"line {line_number}: duplicate pinyin+word")
        seen.add(key)
        entries.append(Entry(
            pinyin=entry.pinyin,
            word=entry.word,
            frequency=entry.frequency,
            category=current_category,
            line_number=line_number,
            line_index=index if keep_line_indexes else -1,
        ))
    return lines, entries


def parse_patch(path: Path) -> list[Entry]:
    _, entries = parse_tsv(path, keep_line_indexes=False)
    seen: set[tuple[str, str]] = set()
    for entry in entries:
        key = (entry.pinyin, entry.word)
        if key in seen:
            raise ValueError(f"patch duplicate pinyin+word at line {entry.line_number}")
        seen.add(key)
    return entries


def render_entry(entry: Entry) -> str:
    return f"{entry.pinyin}\t{entry.word}\t{entry.frequency}"


def category_counts(entries: list[Entry]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for entry in entries:
        counts[entry.category] = counts.get(entry.category, 0) + 1
    return dict(sorted(counts.items()))


def length_distribution(entries: list[Entry]) -> dict[str, int]:
    counts = {"two": 0, "three": 0, "single": 0, "fourplus": 0}
    for entry in entries:
        length = len(entry.word)
        if length == 1:
            counts["single"] += 1
        elif length == 2:
            counts["two"] += 1
        elif length == 3:
            counts["three"] += 1
        else:
            counts["fourplus"] += 1
    return counts


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def atomic_write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=str(path.parent))
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        os.replace(temp_name, path)
    except Exception:
        try:
            os.unlink(temp_name)
        except OSError:
            pass
        raise


def merge(dictionary_path: Path, patch_path: Path, allow_update: bool) -> tuple[str, list[Entry], MergeReport]:
    lines, dictionary_entries = parse_tsv(dictionary_path, keep_line_indexes=True)
    patch_entries = parse_patch(patch_path)
    report = MergeReport()
    by_key = {(entry.pinyin, entry.word): entry for entry in dictionary_entries}
    merged_entries = list(dictionary_entries)
    additions: list[Entry] = []

    for patch_entry in patch_entries:
        existing = by_key.get((patch_entry.pinyin, patch_entry.word))
        if existing is None:
            additions.append(Entry(
                pinyin=patch_entry.pinyin,
                word=patch_entry.word,
                frequency=patch_entry.frequency,
                category=patch_entry.category,
                line_number=patch_entry.line_number,
            ))
            report.added += 1
            continue
        if existing.frequency == patch_entry.frequency:
            report.skipped += 1
            continue
        if not allow_update:
            report.conflicts += 1
            continue
        if existing.line_index < 0:
            raise ValueError("internal error: existing entry missing line index")
        lines[existing.line_index] = render_entry(Entry(
            pinyin=existing.pinyin,
            word=existing.word,
            frequency=patch_entry.frequency,
            category=existing.category,
            line_number=existing.line_number,
        ))
        merged_entries[merged_entries.index(existing)] = Entry(
            pinyin=existing.pinyin,
            word=existing.word,
            frequency=patch_entry.frequency,
            category=existing.category,
            line_number=existing.line_number,
            line_index=existing.line_index,
        )
        report.updated += 1

    if report.conflicts:
        return "\n".join(lines) + "\n", merged_entries, report

    if additions:
        if lines and lines[-1].strip():
            lines.append("")
        lines.append("# category: manual_patch")
        for entry in additions:
            lines.append(render_entry(entry))
            merged_entries.append(Entry(
                pinyin=entry.pinyin,
                word=entry.word,
                frequency=entry.frequency,
                category="manual_patch",
                line_number=len(lines),
            ))

    return "\n".join(lines) + "\n", merged_entries, report


def updated_metadata(metadata_path: Path, entries: list[Entry], dictionary_text: str) -> str:
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    metadata["entry_count"] = len(entries)
    metadata["sha256"] = sha256_bytes(dictionary_text.encode("utf-8"))
    metadata["generated_at"] = date.today().isoformat()
    source_ids = list(metadata.get("source_ids", []))
    if SOURCE_ID not in source_ids:
        source_ids.append(SOURCE_ID)
    metadata["source_ids"] = source_ids
    metadata["length_distribution"] = length_distribution(entries)
    metadata["category_counts"] = category_counts(entries)
    return json.dumps(metadata, ensure_ascii=False, indent=2) + "\n"


def print_report(report: MergeReport, mode: str) -> None:
    print(f"mode: {mode}")
    print(f"added: {report.added}")
    print(f"skipped: {report.skipped}")
    print(f"updated: {report.updated}")
    print(f"conflicts: {report.conflicts}")
    print(f"invalid_rows: {report.invalid_rows}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--patch", type=Path, required=True)
    parser.add_argument("--dictionary", type=Path, default=default_dictionary_path())
    parser.add_argument("--metadata", type=Path, default=default_metadata_path())
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--allow-update", action="store_true")
    args = parser.parse_args()

    try:
        dictionary_text, entries, report = merge(args.dictionary, args.patch, args.allow_update)
    except ValueError as exc:
        print(f"error: {exc}")
        print_report(MergeReport(invalid_rows=1), "apply" if args.apply else "dry-run")
        return 2

    print_report(report, "apply" if args.apply else "dry-run")
    if report.conflicts:
        print("error: frequency update requires --allow-update")
        return 3
    if not args.apply:
        return 0

    metadata_text = updated_metadata(args.metadata, entries, dictionary_text)
    atomic_write_text(args.dictionary, dictionary_text)
    atomic_write_text(args.metadata, metadata_text)
    print(f"sha256: {sha256_bytes(dictionary_text.encode('utf-8'))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
