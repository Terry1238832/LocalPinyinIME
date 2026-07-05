#!/usr/bin/env python3
"""CTest checks for the first-party lexicon audit tool."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


SHA256_PATTERN = re.compile(r"^[0-9a-fA-F]{64}$")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def load_metadata(root: Path) -> dict[str, object]:
    metadata_path = root / "resources/dictionary/local_core_zh_pinyin.metadata.json"
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))

    entry_count = metadata.get("entry_count")
    assert isinstance(entry_count, int) and entry_count > 0, "metadata entry_count must be a positive integer"

    sha256 = metadata.get("sha256")
    assert isinstance(sha256, str) and SHA256_PATTERN.fullmatch(sha256), "metadata sha256 must be a SHA-256 hex string"

    category_counts = metadata.get("category_counts")
    assert isinstance(category_counts, dict), "metadata category_counts must be an object"
    for category, count in category_counts.items():
        assert isinstance(category, str) and category
        assert isinstance(count, int) and count >= 0, f"invalid category count for {category}"
    assert sum(category_counts.values()) == entry_count, "metadata category_counts must sum to entry_count"

    length_distribution = metadata.get("length_distribution")
    assert isinstance(length_distribution, dict), "metadata length_distribution must be an object"
    for bucket, count in length_distribution.items():
        assert isinstance(bucket, str) and bucket
        assert isinstance(count, int) and count >= 0, f"invalid length distribution count for {bucket}"
    assert sum(length_distribution.values()) == entry_count, "metadata length_distribution must sum to entry_count"

    return metadata


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

    metadata = load_metadata(args.root)
    expected_entry_count = metadata["entry_count"]
    expected_sha256 = str(metadata["sha256"]).upper()
    expected_category_counts = {
        str(category): int(count)
        for category, count in dict(metadata["category_counts"]).items()
    }
    dictionary_path = args.root / "resources/dictionary/local_core_zh_pinyin.tsv"
    assert sha256_file(dictionary_path) == expected_sha256

    with tempfile.TemporaryDirectory() as temp_name:
        temp = Path(temp_name)
        first = run_audit(args.script, args.root, temp / "first", 20260701)
        second = run_audit(args.script, args.root, temp / "second", 20260701)

    stats = first["stats"]
    assert stats["valid_entries"] == expected_entry_count
    assert str(stats["sha256"]).upper() == expected_sha256
    assert str(stats["metadata_sha256"]).upper() == expected_sha256
    assert stats["metadata_entry_count"] == expected_entry_count
    assert first["metadata_category_mismatch"] == {}

    category_counts = first["category_counts"]
    assert category_counts == expected_category_counts
    assert sum(category_counts.values()) == expected_entry_count
    assert category_counts["existing_v1"] == 2029
    assert category_counts["time_weather"] == 238
    assert category_counts["daily_phrases"] == 5934
    assert category_counts["targeted_product_baseline_036"] == 26
    assert category_counts["targeted_input_mode_037"] == 13
    assert category_counts.get("manual_patch", 0) == expected_category_counts.get("manual_patch", 0)

    word_lengths = first["word_length_distribution"]
    assert sum(word_lengths.values()) == expected_entry_count
    assert word_lengths["five_plus"] >= 8000
    assert first["daily_phrases_five_plus_count"] >= 5000

    frequency_distribution = {int(key): value for key, value in first["frequency_distribution"].items()}
    for frequency in (9000, 7000, 5000, 3000):
        assert frequency in frequency_distribution
    assert sum(frequency_distribution.values()) == expected_entry_count

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
    assert len(benchmarks) == 30
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
    assert "an" in exact_cases
    assert "anxia" in exact_cases
    assert "xiamian" in exact_cases
    assert "mian" in exact_cases
    assert "mianfei" in exact_cases
    assert "mianfen" in exact_cases
    assert "shouxian" in exact_cases
    assert "xian" in exact_cases
    assert "xianjiuzheng" in exact_cases
    assert "yixie" in exact_cases
    assert "qisiwole" in exact_cases
    assert "jintiantianqihenhao" in exact_cases
    assert "woxianzaiyouyidianmang" in exact_cases
    assert "woxianzai" in exact_cases
    segmented_cases = {item["pinyin"] for item in benchmarks if item["multi_entry_segmentation"]}
    assert "zhegeshi" in segmented_cases

    print("first-party lexicon audit checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
