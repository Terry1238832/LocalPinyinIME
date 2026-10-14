#!/usr/bin/env python3
"""Query every frozen batch key in the native engine without learning answers."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
NAMES = ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv", "context_zh.tsv", "continuations_zh.tsv")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def entries(path: Path):
    return [tuple(line.split("\t")) for line in path.read_text(encoding="utf-8").splitlines() if line and not line.startswith("#")]


def write(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, default=ROOT / ".build/bin/LocalPinyinImeEval.exe")
    parser.add_argument("--resources", type=Path, default=ROOT / ".build/bin")
    args = parser.parse_args()
    engine, resources = args.engine.resolve(), args.resources.resolve()
    batch = OUT / "lexicon_batch_011.tsv"
    metadata_path, applied_path = batch.with_suffix(".json"), batch.with_suffix(".applied.json")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    applied = json.loads(applied_path.read_text(encoding="utf-8"))
    frozen_sha = sha(batch)
    assert frozen_sha == metadata["sha256"] == applied["frozen_batch_sha256"]
    source = ROOT / "resources/dictionary"
    resource_sha = {name: sha(resources / "dictionary" / name) for name in NAMES}
    assert resource_sha == {name: sha(source / name) for name in NAMES}, "Runtime resources do not match frozen source; build first."
    engine_sha = sha(engine)
    with tempfile.TemporaryDirectory(prefix="batch011-native-", dir=OUT) as temporary:
        isolated = Path(temporary)
        cases = isolated / "cases.tsv"
        cases.write_bytes(batch.read_bytes())
        env = dict(os.environ, LOCALPINYINIME_LOCALAPPDATA=str(isolated.resolve()),
                   LOCALPINYINIME_USER_LEXICON_PATH=str((isolated / "user_lexicon.tsv").resolve()))
        process = subprocess.run([str(engine), str(resources), str(cases)], stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, check=True, env=env)
    rows = [json.loads(line) for line in process.stdout.decode("utf-8").splitlines() if line]
    expected = {(key, word) for key, word, _ in entries(batch)}
    assert len(rows) == len(expected) == metadata["new_entries"]
    assert {(row["pinyin"], row["expected"]) for row in rows} == expected
    assert sha(batch) == frozen_sha and sha(engine) == engine_sha
    assert resource_sha == {name: sha(resources / "dictionary" / name) for name in NAMES}
    assert resource_sha == {name: sha(source / name) for name in NAMES}
    durations = sorted(row["lookup_us"] for row in rows)
    missing = [{"pinyin": row["pinyin"], "word": row["expected"], "expected_rank": row["expected_rank"], "candidates": row["candidates"]} for row in rows if not 1 <= row["expected_rank"] <= 9]
    summary = {
        "cases": len(rows), "top1_correct": sum(row["expected_rank"] == 1 for row in rows),
        "top9_present": sum(1 <= row["expected_rank"] <= 9 for row in rows),
        "top1_percent": round(100 * sum(row["expected_rank"] == 1 for row in rows) / len(rows), 4),
        "top9_percent": round(100 * sum(1 <= row["expected_rank"] <= 9 for row in rows) / len(rows), 4),
        "missing_from_top9_count": len(missing), "missing_from_top9": missing,
        "non_top1_count": sum(row["expected_rank"] != 1 for row in rows),
        "max_returned_candidate_count": max(len(row["candidates"]) for row in rows),
        "p50_engine_lookup_ms": round(durations[len(durations) // 2] / 1000, 3),
        "p95_engine_lookup_ms": round(durations[min(len(durations)-1, math.floor(.95 * len(durations)))] / 1000, 3),
        "max_engine_lookup_ms": round(durations[-1] / 1000, 3),
    }
    all_rows = [row for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv") for row in entries(source / name)]
    facts = {
        "engine_sha256": engine_sha, "resource_sha256": resource_sha, "frozen_batch_sha256": frozen_sha,
        "frozen_batch_unchanged": True, "personal_data_isolated": True, "expected_answers_learned": False,
    }
    report = {
        "batch": batch.stem, "evaluation": "native_engine_candidate_coverage",
        "evaluated_at_utc": datetime.now(timezone.utc).isoformat(), "engine": engine.name,
        "resources_directory": resources.relative_to(ROOT).as_posix(), **facts,
        "case_input": "Frozen batch TSV, unchanged; third column is only an evaluation source label.",
        "top9_definition": "Expected display text occurs at rank 1 through 9 inclusive in actual engine.lookup candidates.",
        "local_core_entries": len(entries(source / "local_core_zh_pinyin.tsv")),
        "raw_all_layer_rows": len(all_rows), "effective_unique_key_display_pairs": len({(row[0], row[1]) for row in all_rows}),
        "summary": summary, "rows": rows,
    }
    report_path = batch.with_suffix(".validation.json")
    if report_path.exists():
        previous = json.loads(report_path.read_text(encoding="utf-8"))
        report["previous_pass"] = {"report_sha256": sha(report_path), "evaluated_at_utc": previous["evaluated_at_utc"], "engine_sha256": previous["engine_sha256"], "summary": previous["summary"]}
    write(report_path, report)
    coverage = {"status": "passed" if not missing else "completed_with_missing_candidates", "validation_file": report_path.name, "validation_sha256": sha(report_path), **facts, **summary,
                "non_top1_details": "Complete ranks and all non-top1 cases retained in lexicon_batch_011.validation.json."}
    applied["native_candidate_coverage"] = coverage
    applied["freeze_status"] = "Batch TSV and all five runtime/source TSV hashes retained; full native coverage measured without learning."
    metadata["validation"]["native_engine_top9_coverage"] = {key: coverage[key] for key in ("status", "validation_file", "validation_sha256", "engine_sha256", "cases", "top1_correct", "top9_present", "missing_from_top9_count", "frozen_batch_unchanged")}
    write(applied_path, applied)
    write(metadata_path, metadata)
    print(json.dumps({"summary": summary, "validation_sha256": sha(report_path), "engine_sha256": engine_sha}, ensure_ascii=False))


if __name__ == "__main__":
    main()
