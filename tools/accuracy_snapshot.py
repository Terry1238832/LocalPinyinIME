#!/usr/bin/env python3
"""Top-1 / top-9 accuracy of the native engine on the frozen test sets.

Run it before and after a lexicon change and compare: a new entry must not push
a correct candidate down. The test sets are evaluation data only; never take
vocabulary from them.

Usage:
  python tools/accuracy_snapshot.py [--engine .build/bin/LocalPinyinImeEval.exe]
                                    [--resources .build/bin] [--output snapshot.json]
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent
# Name -> (case file, extra engine flags). The same word cases without their
# preceding text measure the order of a word typed on its own (start of a
# field, or an application that does not expose its text).
SETS = {
    "逐词输入·参考前文": ("heldout_context.tsv", []),
    "逐词输入·无前文": ("heldout_context.tsv", ["--no-context"]),
    "留出整句": ("heldout_sentences.tsv", []),
    # Everyday dialogue (KdConv, CrossWOZ): only clauses absent from training.
    "口语对话·整句": ("heldout_dialogue_sentences.tsv", []),
    "口语对话·逐词": ("heldout_dialogue_context.tsv", []),
    "项目留出": ("typing_holdout.tsv", []),
    "新增用例": ("typing_fresh.tsv", []),
    "独立用例": ("typing_independent.tsv", []),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path, default=ROOT / ".build/bin/LocalPinyinImeEval.exe")
    parser.add_argument("--resources", type=Path, default=ROOT / ".build/bin")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = {}
    for name, (filename, flags) in SETS.items():
        result = subprocess.run([str(args.engine), str(args.resources), str(ROOT / "tests/data" / filename), *flags],
                                capture_output=True, check=True)
        ranks = [json.loads(line)["expected_rank"] for line in result.stdout.decode("utf-8").splitlines()
                 if line.startswith("{")]
        report[name] = {"cases": len(ranks),
                        "top1": round(100 * sum(rank == 1 for rank in ranks) / max(1, len(ranks)), 2),
                        "top9": round(100 * sum(rank > 0 for rank in ranks) / max(1, len(ranks)), 2)}
        print(f"{name}\t{len(ranks)} 例\t首选 {report[name]['top1']}%\t前九 {report[name]['top9']}%")
    if args.output:
        args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
