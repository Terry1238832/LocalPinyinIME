#!/usr/bin/env python3
"""Tests for lexicon source manifests and the offline import tool."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path


REQUIRED_FIELDS = {
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


def run_tool(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *args],
        check=False,
        text=True,
        encoding="utf-8",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    sources = manifest.get("sources")
    assert isinstance(sources, list) and sources, "manifest must contain sources"
    by_id = {}
    for source in sources:
        missing = REQUIRED_FIELDS.difference(source)
        assert not missing, f"source missing fields: {sorted(missing)}"
        by_id[source["source_id"]] = source
    first_party = by_id["first_party_curated"]
    assert first_party["review_status"] == "approved"
    assert first_party["included_in_release"] is True
    first_party_v2 = by_id["first_party_curated_v2"]
    assert first_party_v2["source_name"] == "LocalPinyinIME First-Party Curated Lexicon v2"
    assert first_party_v2["review_status"] == "approved"
    assert first_party_v2["included_in_release"] is True
    assert "third-party" in first_party_v2["notes"]
    assert "relative weights" in first_party_v2["notes"]

    script_text = args.script.read_text(encoding="utf-8")
    for forbidden in ("urllib", "requests", "socket", "http://", "https://", "git clone", "curl", "wget"):
        assert forbidden not in script_text, f"import tool must remain offline-only: {forbidden}"
    assert "user_lexicon.tsv" not in script_text

    with tempfile.TemporaryDirectory() as temp_name:
        temp = Path(temp_name)
        source_tsv = temp / "source.tsv"
        source_tsv.write_text(
            "# sample\n"
            "nihao\t你好\t9000\n"
            "nihao\t你好\t7000\n"
            "bad pinyin\t坏\t1\n",
            encoding="utf-8",
            newline="\n",
        )
        pending_manifest = temp / "pending.json"
        pending = {
            "format_version": 1,
            "sources": [
                {
                    **first_party,
                    "source_id": "pending_source",
                    "review_status": "pending",
                    "included_in_release": False,
                }
            ],
        }
        pending_manifest.write_text(json.dumps(pending), encoding="utf-8")
        rejected = run_tool(
            args.script,
            "--input", str(source_tsv),
            "--manifest", str(pending_manifest),
            "--source-id", "pending_source",
            "--output-dir", str(temp / "rejected"),
        )
        assert rejected.returncode != 0, "pending source must be rejected"

        accepted = run_tool(
            args.script,
            "--input", str(source_tsv),
            "--manifest", str(args.manifest),
            "--source-id", "first_party_curated",
            "--output-dir", str(temp / "out"),
        )
        assert accepted.returncode == 1, accepted.stdout + accepted.stderr
        report_path = temp / "out" / "first_party_curated.import-report.json"
        report = json.loads(report_path.read_text(encoding="utf-8"))
        assert report["valid_entries"] == 1
        assert report["duplicate_rows"] == 1
        assert report["invalid_rows"] == 1
        assert len(report["sha256"]) == 64
        assert (temp / "out" / "first_party_curated.normalized.tsv").exists()

    print("lexicon source pipeline checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
