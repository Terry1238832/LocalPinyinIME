#!/usr/bin/env python3
"""CTest coverage for the first-party local_core lexicon patch tool."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path


BENCHMARK_ENTRIES = [
    ("kaihui", "\u5f00\u4f1a"),
    ("fangjia", "\u653e\u5047"),
    ("biancheng", "\u53d8\u6210"),
    ("shenle", "\u795e\u4e86"),
    ("zhiyou", "\u53ea\u6709"),
    ("bucuo", "\u4e0d\u9519"),
    ("bianji", "\u7f16\u8f91"),
    ("huode", "\u83b7\u5f97"),
    ("an", "\u6309"),
    ("anxia", "\u6309\u4e0b"),
    ("xiamian", "\u4e0b\u9762"),
    ("mian", "\u9762"),
    ("mianfei", "\u514d\u8d39"),
    ("mianfen", "\u9762\u7c89"),
    ("shouxian", "\u9996\u5148"),
    ("xian", "\u5148"),
    ("xianjiuzheng", "\u5148\u7ea0\u6b63"),
    ("yixie", "\u4e00\u4e9b"),
    ("qisiwole", "\u6c14\u6b7b\u6211\u4e86"),
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


def length_distribution(words: list[str]) -> dict[str, int]:
    counts = {"two": 0, "three": 0, "single": 0, "fourplus": 0}
    for word in words:
        length = len(word)
        if length <= 1:
            counts["single"] += 1
        elif length == 2:
            counts["two"] += 1
        elif length == 3:
            counts["three"] += 1
        else:
            counts["fourplus"] += 1
    return counts


def write_metadata(metadata: Path, dictionary_text: str, category_counts: dict[str, int], words: list[str]) -> None:
    metadata.write_text(
        json.dumps(
            {
                "format_version": 2,
                "dictionary_name": "fixture",
                "maintainer": "test",
                "intended_distribution": "test",
                "entry_count": len(words),
                "sha256": hashlib.sha256(dictionary_text.encode("utf-8")).hexdigest().upper(),
                "generated_at": "2026-07-02",
                "source_ids": ["fixture"],
                "source_policy": "fixture",
                "frequency_policy": "relative",
                "review_status": "approved fixture",
                "length_distribution": length_distribution(words),
                "category_counts": category_counts,
            },
            ensure_ascii=False,
            indent=2,
        )
        + "\n",
        encoding="utf-8",
        newline="\n",
    )


def write_minimal_fixture(root: Path) -> tuple[Path, Path, Path]:
    dictionary = root / "local_core_zh_pinyin.tsv"
    metadata = root / "local_core_zh_pinyin.metadata.json"
    patch = root / "patch.tsv"
    dictionary.write_text(
        "# category: existing\n"
        "nihao\t你好\t9000\n"
        "mian\t面\t9000\n",
        encoding="utf-8",
        newline="\n",
    )
    digest = hashlib.sha256(dictionary.read_bytes()).hexdigest().upper()
    metadata.write_text(
        json.dumps(
            {
                "format_version": 2,
                "dictionary_name": "fixture",
                "maintainer": "test",
                "intended_distribution": "test",
                "entry_count": 2,
                "sha256": digest,
                "generated_at": "2026-07-02",
                "source_ids": ["fixture"],
                "source_policy": "fixture",
                "frequency_policy": "relative",
                "review_status": "approved fixture",
                "length_distribution": {"two": 1, "three": 0, "single": 1, "fourplus": 0},
                "category_counts": {"existing": 2},
            },
            ensure_ascii=False,
            indent=2,
        )
        + "\n",
        encoding="utf-8",
        newline="\n",
    )
    patch.write_text("ceshi\t测试\t1000\n", encoding="utf-8", newline="\n")
    return dictionary, metadata, patch


def write_audit_fixture(root: Path) -> tuple[Path, Path, Path, Path, Path]:
    core = root / "core_zh_pinyin.tsv"
    dictionary = root / "local_core_zh_pinyin.tsv"
    metadata = root / "local_core_zh_pinyin.metadata.json"
    sources = root / "lexicon_sources.json"
    patch = root / "patch.tsv"

    rows = ["# category: fixture_benchmark\n"]
    rows.extend(f"{pinyin}\t{word}\t9000\n" for pinyin, word in BENCHMARK_ENTRIES)
    dictionary_text = "".join(rows)
    dictionary.write_text(dictionary_text, encoding="utf-8", newline="\n")
    write_metadata(
        metadata,
        dictionary_text,
        {"fixture_benchmark": len(BENCHMARK_ENTRIES)},
        [word for _, word in BENCHMARK_ENTRIES],
    )

    core.write_text("# fixture core dictionary intentionally empty\n", encoding="utf-8", newline="\n")
    sources.write_text(
        json.dumps({"sources": [{"source_id": "fixture"}]}, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    patch.write_text("ceshibuding\t\u6d4b\u8bd5\u8865\u4e01\t1000\n", encoding="utf-8", newline="\n")
    return core, dictionary, metadata, sources, patch


def run_tool(script: Path, dictionary: Path, metadata: Path, patch: Path, *extra: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(script),
            "--dictionary", str(dictionary),
            "--metadata", str(metadata),
            "--patch", str(patch),
            *extra,
        ],
        check=False,
        text=True,
        encoding="utf-8",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def run_audit(audit_script: Path, core: Path, dictionary: Path, metadata: Path, sources: Path, output_dir: Path) -> dict[str, object]:
    completed = subprocess.run(
        [
            sys.executable,
            str(audit_script),
            "--core", str(core),
            "--dictionary", str(dictionary),
            "--metadata", str(metadata),
            "--sources", str(sources),
            "--output-dir", str(output_dir),
            "--seed", "20260701",
        ],
        check=False,
        text=True,
        encoding="utf-8",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert completed.returncode == 0, completed.stdout + completed.stderr
    return json.loads((output_dir / "first_party_lexicon_v2_audit.json").read_text(encoding="utf-8"))


def assert_success(completed: subprocess.CompletedProcess[str]) -> None:
    assert completed.returncode == 0, completed.stdout + completed.stderr


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--root", type=Path, required=True)
    args = parser.parse_args()

    script_text = args.script.read_text(encoding="utf-8")
    forbidden_network_tokens = ("requests", "urllib", "socket", "http://", "https://", "curl", "wget")
    for token in forbidden_network_tokens:
        assert token not in script_text, f"patch tool must remain offline: {token}"
    forbidden_user_data_tokens = ("user_lexicon.tsv", "user_learning", "LOCALAPPDATA")
    for token in forbidden_user_data_tokens:
        assert token not in script_text, f"patch tool must not touch private user data: {token}"

    actual_lexicon = (args.root / "resources/dictionary/local_core_zh_pinyin.tsv").read_text(encoding="utf-8")
    example_patch = args.root / "resources/dictionary/patches/example_local_core_patch.tsv"
    for raw_line in example_patch.read_text(encoding="utf-8").splitlines():
        if not raw_line.strip() or raw_line.startswith("#"):
            continue
        pinyin, word, _ = raw_line.split("\t")
        assert f"{pinyin}\t{word}\t" not in actual_lexicon

    with tempfile.TemporaryDirectory() as temp_name:
        temp = Path(temp_name)
        dictionary, metadata, patch = write_minimal_fixture(temp)
        original_dictionary = dictionary.read_text(encoding="utf-8")
        original_metadata = metadata.read_text(encoding="utf-8")
        user_lexicon = temp / "user_lexicon.tsv"
        user_learning = temp / "user_learning.tsv"
        user_lexicon.write_text("siyou\t私有\t9000\n", encoding="utf-8")
        user_learning.write_text("siyou\t私有\t1\t1\n", encoding="utf-8")

        dry_run = run_tool(args.script, dictionary, metadata, patch)
        assert_success(dry_run)
        assert "mode: dry-run" in dry_run.stdout
        assert "added: 1" in dry_run.stdout
        assert dictionary.read_text(encoding="utf-8") == original_dictionary
        assert metadata.read_text(encoding="utf-8") == original_metadata

        applied = run_tool(args.script, dictionary, metadata, patch, "--apply")
        assert_success(applied)
        assert "mode: apply" in applied.stdout
        merged = dictionary.read_text(encoding="utf-8")
        assert "ceshi\t测试\t1000" in merged
        updated_metadata = json.loads(metadata.read_text(encoding="utf-8"))
        assert updated_metadata["entry_count"] == 3
        assert updated_metadata["sha256"] == hashlib.sha256(dictionary.read_bytes()).hexdigest().upper()
        assert updated_metadata["category_counts"]["manual_patch"] == 1
        assert user_lexicon.read_text(encoding="utf-8") == "siyou\t私有\t9000\n"
        assert user_learning.read_text(encoding="utf-8") == "siyou\t私有\t1\t1\n"

    with tempfile.TemporaryDirectory() as temp_name:
        temp = Path(temp_name)
        dictionary, metadata, patch = write_minimal_fixture(temp)

        patch.write_text("nihao\t你好\t7000\n", encoding="utf-8", newline="\n")
        conflict = run_tool(args.script, dictionary, metadata, patch, "--apply")
        assert conflict.returncode != 0
        assert "conflicts: 1" in conflict.stdout
        rows_after_conflict = [
            line.split("\t")
            for line in dictionary.read_text(encoding="utf-8").splitlines()
            if line and not line.startswith("#")
        ]
        assert any(fields[0] == "nihao" and fields[2] == "9000" for fields in rows_after_conflict)
        assert not any(fields[0] == "nihao" and fields[2] == "7000" for fields in rows_after_conflict)

        updated = run_tool(args.script, dictionary, metadata, patch, "--apply", "--allow-update")
        assert_success(updated)
        assert "nihao\t你好\t7000" in dictionary.read_text(encoding="utf-8")

    with tempfile.TemporaryDirectory() as temp_name:
        temp = Path(temp_name)
        core, dictionary, metadata, sources, patch = write_audit_fixture(temp)
        expected_before = len(BENCHMARK_ENTRIES)
        applied = run_tool(args.script, dictionary, metadata, patch, "--apply")
        assert_success(applied)
        assert "mode: apply" in applied.stdout
        assert "added: 1" in applied.stdout

        updated_metadata = json.loads(metadata.read_text(encoding="utf-8"))
        expected_after = expected_before + 1
        expected_sha = hashlib.sha256(dictionary.read_bytes()).hexdigest().upper()
        assert updated_metadata["entry_count"] == expected_after
        assert updated_metadata["sha256"] == expected_sha
        assert updated_metadata["category_counts"]["fixture_benchmark"] == expected_before
        assert updated_metadata["category_counts"]["manual_patch"] == 1

        audit = run_audit(args.root / "tools/audit_first_party_lexicon.py", core, dictionary, metadata, sources, temp / "audit")
        assert audit["stats"]["valid_entries"] == expected_after
        assert audit["stats"]["metadata_entry_count"] == expected_after
        assert audit["stats"]["sha256"] == expected_sha
        assert audit["stats"]["metadata_sha256"] == expected_sha
        assert audit["metadata_category_mismatch"] == {}
        assert audit["risk_summary"]["invalid_rows"] == 0
        assert audit["risk_summary"]["duplicate_rows"] == 0
        assert audit["risk_summary"]["ranking_failures"] == 0

    invalid_rows = {
        "bad1\t坏\t1000\n": "invalid pinyin",
        "hao\t好\t-1\n": "negative frequency",
        "hao\t好\n": "malformed TSV row",
        "dup\t重复\t1000\ndup\t重复\t1000\n": "duplicate",
    }
    for text, expected in invalid_rows.items():
        with tempfile.TemporaryDirectory() as temp_name:
            temp = Path(temp_name)
            dictionary, metadata, patch = write_minimal_fixture(temp)
            patch.write_text(text, encoding="utf-8", newline="\n")
            completed = run_tool(args.script, dictionary, metadata, patch)
            assert completed.returncode != 0
            assert expected in completed.stdout

    print("merge local core lexicon patch checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
