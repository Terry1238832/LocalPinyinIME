#!/usr/bin/env python3
"""CTest checks for the first-party lexicon audit tool."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def run_audit(script: Path, root: Path, output_dir: Path, seed: int) -> dict[str, object]:
    completed = subprocess.run(
        [
            sys.executable,
            str(script),
            "--core", str(root / "resources/dictionary/core_zh_pinyin.tsv"),
            "--dictionary", str(root / "resources/dictionary/local_core_zh_pinyin.tsv"),
            "--metadata", str(root / "resources/dictionary/local_core_zh_pinyin.metadata.json"),
            "--sources", str(root / "resources/dictionary/sources/lexicon_sources.json"),
            "--output-dir", str(output_dir),
            "--seed", str(seed),
        ],
        check=False,
        text=True,
        encoding="utf-8",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    report_path = output_dir / "first_party_lexicon_v2_audit.json"
    markdown_path = output_dir / "first_party_lexicon_v2_audit.md"
    assert report_path.exists()
    assert markdown_path.exists()
    return json.loads(report_path.read_text(encoding="utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()

    script_text = args.script.read_text(encoding="utf-8")
    forbidden = ("LOCALAPPDATA", "user_lexicon.tsv", "urllib", "requests", "socket", "curl", "wget", "git clone")
    for token in forbidden:
        assert token not in script_text, f"audit tool must not touch user/private/network data: {token}"

    with tempfile.TemporaryDirectory() as temp_name:
        temp = Path(temp_name)
        first = run_audit(args.script, args.root, temp / "first", 20260701)
        second = run_audit(args.script, args.root, temp / "second", 20260701)

    stats = first["stats"]
    assert stats["valid_entries"] == 10943
    assert stats["sha256"] == stats["metadata_sha256"]
    assert stats["metadata_entry_count"] == 10943
    assert first["metadata_category_mismatch"] == {}

    category_counts = first["category_counts"]
    assert category_counts["existing_v1"] == 2029
    assert category_counts["time_weather"] == 238
    assert category_counts["daily_phrases"] == 5934
    assert category_counts["targeted_product_baseline_036"] == 26

    word_lengths = first["word_length_distribution"]
    assert sum(word_lengths.values()) == 10943
    assert word_lengths["five_plus"] >= 8000
    assert first["daily_phrases_five_plus_count"] >= 5000

    frequency_distribution = {int(key): value for key, value in first["frequency_distribution"].items()}
    for frequency in (9000, 7000, 5000, 3000):
        assert frequency in frequency_distribution
    assert sum(frequency_distribution.values()) == 10943

    risk_summary = first["risk_summary"]
    assert risk_summary["invalid_rows"] == 0
    assert risk_summary["duplicate_rows"] == 0
    assert risk_summary["ranking_failures"] == 0
    assert risk_summary["suggested_remove_count"] == 0
    assert risk_summary["suggested_manual_review_count"] > 0

    assert first["samples"] == second["samples"], "fixed seed samples must be reproducible"
    assert first["suspicious"]["abnormal_long"] or first["suspicious"]["templated_long_phrase_review"]
    assert first["near_duplicate_review"] is not None

    benchmarks = first["ranking_benchmarks"]
    assert len(benchmarks) == 19
    for item in benchmarks:
        assert item["passed"], item
        assert item["actual_first_candidate"] == item["target_first_candidate"], item
    exact_cases = {item["pinyin"] for item in benchmarks if item["single_complete_entry"]}
    assert "kaihui" in exact_cases
    assert "fangjia" in exact_cases
    assert "biancheng" in exact_cases
    assert "shenle" in exact_cases
    assert "zhiyou" in exact_cases
    assert "bucuo" in exact_cases
    assert "bianji" in exact_cases
    assert "huode" in exact_cases
    assert "jintiantianqihenhao" in exact_cases
    assert "woxianzaiyouyidianmang" in exact_cases
    segmented_cases = {item["pinyin"] for item in benchmarks if item["multi_entry_segmentation"]}
    assert "zhegeshi" in segmented_cases
    assert "woxianzai" in segmented_cases

    print("first-party lexicon audit checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
