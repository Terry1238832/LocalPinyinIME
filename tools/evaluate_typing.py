#!/usr/bin/env python3
"""Evaluate frozen typing cases without feeding expected sentences to the engine."""
from __future__ import annotations

import argparse
import collections
import json
import math
import os
import subprocess
import tempfile
import hashlib
from pathlib import Path


def edit_distance(left: str, right: str) -> int:
    previous = list(range(len(right) + 1))
    for index, character in enumerate(left, 1):
        current = [index]
        for offset, other in enumerate(right, 1):
            current.append(min(current[-1] + 1, previous[offset] + 1, previous[offset - 1] + (character != other)))
        previous = current
    return previous[-1]


def summarize(rows: list[dict]) -> dict:
    if not rows:
        return {}
    durations = sorted(row["lookup_us"] for row in rows)
    characters = sum(len(row["expected"]) for row in rows)
    errors = sum(edit_distance(row["candidates"][0] if row["candidates"] else "", row["expected"]) for row in rows)
    return {
        "cases": len(rows),
        "top1_correct": sum(row["expected_rank"] == 1 for row in rows),
        "top9_present": sum(row["expected_rank"] > 0 for row in rows),
        "top1_percent": round(100 * sum(row["expected_rank"] == 1 for row in rows) / len(rows), 2),
        "top9_percent": round(100 * sum(row["expected_rank"] > 0 for row in rows) / len(rows), 2),
        "character_error_rate": round(errors / max(1, characters), 4),
        "p50_lookup_ms": round(durations[len(durations) // 2] / 1000, 3),
        "p95_lookup_ms": round(durations[min(len(durations) - 1, math.floor(.95 * len(durations)))] / 1000, 3),
        "max_lookup_ms": round(durations[-1] / 1000, 3),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--resources", type=Path, required=True)
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--keystrokes", action="store_true", help="Also time every growing pinyin prefix, as in real typing")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="typing-eval-", dir=args.output.parent) as temporary:
        isolated_cases = Path(temporary) / "cases.tsv"
        isolated_cases.write_bytes(args.cases.read_bytes())
        env = dict(os.environ, LOCALPINYINIME_LOCALAPPDATA=str(Path(temporary).resolve()),
                   LOCALPINYINIME_USER_LEXICON_PATH=str((Path(temporary) / "user_lexicon.tsv").resolve()))
        command = [str(args.engine.absolute()), str(args.resources.absolute()), str(isolated_cases.absolute())]
        if args.keystrokes: command.append("--keystrokes")
        process = subprocess.run(command,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True, env=env)
    rows = [json.loads(line) for line in process.stdout.decode("utf-8").splitlines() if line]
    groups: dict[str, list[dict]] = collections.defaultdict(list)
    for row in rows:
        groups[row["source"].split(":")[0]].append(row)
    report = {
        "evaluation": {"personal_data_isolated": True, "expected_answers_learned": False,
                       "engine_sha256": hashlib.sha256(args.engine.read_bytes()).hexdigest(),
                       "cases_sha256": hashlib.sha256(args.cases.read_bytes()).hexdigest(),
                       "resource_sha256": {name: hashlib.sha256(path.read_bytes()).hexdigest()
                           for name in ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv",
                                        "standard_zh_pinyin.tsv", "context_zh.tsv")
                           for path in [args.resources / "dictionary" / name] if path.is_file()}},
        "summary": summarize(rows),
        "groups": {group: summarize(values) for group, values in groups.items()},
        "failures": [{"source": row["source"], "pinyin": row["pinyin"], "expected": row["expected"],
                      "actual": row["candidates"][0] if row["candidates"] else "", "rank": row["expected_rank"]}
                     for row in rows if row["expected_rank"] != 1],
        "rows": rows,
    }
    if args.keystrokes:
        times = sorted(t for row in rows for t in row["typing_lookup_us"])
        report["keystrokes"] = {"count": len(times), "first_key_ms": rows[0]["typing_lookup_us"][0]/1000,
            "p50_ms": times[len(times)//2]/1000, "p95_ms": times[min(len(times)-1, math.floor(.95*len(times)))]/1000,
            "max_ms": times[-1]/1000, "measurement": "Engine lookup only; no OS event dispatch or edit-session time"}
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"summary": report["summary"], "groups": report["groups"],
                      "keystrokes": report.get("keystrokes"), "failures": report["failures"][:8]}, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
