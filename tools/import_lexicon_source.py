#!/usr/bin/env python3
"""Offline-only lexicon import preparation for reviewed local sources."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path


PINYIN_RE = re.compile(r"^[a-z]+$")
REQUIRED_SOURCE_FIELDS = {
    "source_id",
    "source_name",
    "source_url",
    "license",
    "license_url",
    "redistribution_allowed",
    "attribution_required",
    "share_alike_required",
    "commercial_use_allowed",
    "derivative_work_allowed",
    "review_status",
    "reviewed_at",
    "included_in_release",
    "notes",
}


@dataclass
class ImportStats:
    source_rows: int = 0
    comment_rows: int = 0
    blank_rows: int = 0
    valid_entries: int = 0
    invalid_rows: int = 0
    duplicate_rows: int = 0


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def load_source(manifest_path: Path, source_id: str) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    sources = manifest.get("sources")
    if not isinstance(sources, list):
        raise ValueError("manifest must contain a sources list")

    for source in sources:
        if not isinstance(source, dict):
            continue
        missing = REQUIRED_SOURCE_FIELDS.difference(source)
        if missing:
            raise ValueError(f"source entry missing fields: {', '.join(sorted(missing))}")
        if source.get("source_id") == source_id:
            return source
    raise ValueError(f"source_id not found: {source_id}")


def require_release_approved(source: dict) -> None:
    if source.get("review_status") != "approved" or source.get("included_in_release") is not True:
        raise ValueError("source is not approved for release import")


def parse_tsv(path: Path) -> tuple[list[tuple[str, str, int]], ImportStats]:
    stats = ImportStats()
    entries: dict[tuple[str, str], int] = {}
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

            fields = line.split("\t")
            if len(fields) != 3:
                stats.invalid_rows += 1
                continue
            pinyin, word, frequency_text = (field.strip() for field in fields)
            try:
                frequency = int(frequency_text, 10)
            except ValueError:
                stats.invalid_rows += 1
                continue
            if not PINYIN_RE.fullmatch(pinyin) or not word or "\t" in word or "\n" in word or frequency < 0:
                stats.invalid_rows += 1
                continue

            key = (pinyin, word)
            if key in entries:
                stats.duplicate_rows += 1
            entries[key] = frequency

    ordered = [(pinyin, word, frequency) for (pinyin, word), frequency in sorted(entries.items())]
    stats.valid_entries = len(ordered)
    return ordered, stats


def write_outputs(output_dir: Path, source_id: str, entries: list[tuple[str, str, int]], stats: ImportStats) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    tsv_path = output_dir / f"{source_id}.normalized.tsv"
    lines = [f"# LocalPinyinIME normalized import for source_id={source_id}"]
    lines.extend(f"{pinyin}\t{word}\t{frequency}" for pinyin, word, frequency in entries)
    data = ("\n".join(lines) + "\n").encode("utf-8")
    tsv_path.write_bytes(data)

    report = {
        "source_id": source_id,
        "source_rows": stats.source_rows,
        "comment_rows": stats.comment_rows,
        "blank_rows": stats.blank_rows,
        "valid_entries": stats.valid_entries,
        "invalid_rows": stats.invalid_rows,
        "duplicate_rows": stats.duplicate_rows,
        "sha256": sha256_bytes(data),
        "output_file": tsv_path.name,
    }
    (output_dir / f"{source_id}.import-report.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    print(f"source_id: {source_id}")
    print(f"valid_entries: {stats.valid_entries}")
    print(f"invalid_rows: {stats.invalid_rows}")
    print(f"duplicate_rows: {stats.duplicate_rows}")
    print(f"sha256: {report['sha256']}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare a reviewed local lexicon source for import.")
    parser.add_argument("--input", type=Path, required=True, help="Local UTF-8 TSV source file.")
    parser.add_argument("--manifest", type=Path, required=True, help="Lexicon source manifest JSON.")
    parser.add_argument("--source-id", required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    try:
        source = load_source(args.manifest, args.source_id)
        require_release_approved(source)
        entries, stats = parse_tsv(args.input)
        write_outputs(args.output_dir, args.source_id, entries, stats)
        return 0 if stats.invalid_rows == 0 else 1
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"error: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
