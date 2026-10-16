#!/usr/bin/env python3
"""Export the lexicon gaps found by report_lexicon_coverage.py as worklists.

Writes resources/dictionary/expansion/gaps/:
  missing_characters.tsv    通用规范汉字表 characters that cannot be typed as a
                            single character by their standard reading
  missing_common_words.tsv  words of the jieba frequency list (MIT) within the
                            top N that no built-in layer contains

These are checklists for authoring batches, not lexicon data: every entry must
still be reviewed (reading in context, whether it is worth adding) before it
goes into a batch. Held-out test texts are deliberately not used, so adding
words cannot leak evaluation answers.

Authoring-time requirements: jieba, pypinyin. Usage:
  python tools/export_lexicon_gaps.py --unihan <Unihan dir> [--top 30000]
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re

import jieba
from pypinyin import Style, lazy_pinyin

from report_lexicon_coverage import load_lexicon, standard_characters

ROOT = Path(__file__).resolve().parent.parent
DICTIONARY = ROOT / "resources" / "dictionary"
HAN = re.compile(r"^[㐀-鿿]+$")
LEVELS = ((1, 3500, "一级"), (3501, 6500, "二级"), (6501, 8105, "三级"))
POS = {"nr": "人名", "nrfg": "人名", "nrt": "音译人名", "ns": "地名", "nt": "机构", "nz": "其他专名"}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--unihan", type=Path, required=True)
    parser.add_argument("--top", type=int, default=30000)
    parser.add_argument("--output", type=Path, default=DICTIONARY / "expansion" / "gaps")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    pairs, words = load_lexicon(DICTIONARY)

    lines = ["# 级别\t序号\t字\t标准读音（无声调，ü 写作 v）\t已有的单字读音"]
    single = {}
    for key, word in pairs:
        if len(word) == 1:
            single.setdefault(word, set()).add(key)
    for char, number, readings in standard_characters(args.unihan):
        level = next(name for low, high, name in LEVELS if low <= number <= high)
        # Input methods spell lüe/nüe as lue/nue; accept either spelling.
        missing = [reading for reading in readings
                   if not {(reading, char), (reading.replace("lve", "lue").replace("nve", "nue"), char)} & pairs]
        if readings and len(missing) == len(readings):
            lines.append(f"{level}\t{number}\t{char}\t{'/'.join(readings)}\t{'/'.join(sorted(single.get(char, ())))}")
    (args.output / "missing_characters.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")

    ranked = []
    for line in (Path(jieba.__file__).parent / "dict.txt").read_text(encoding="utf-8").splitlines():
        parts = line.split()
        if len(parts) >= 3 and len(parts[0]) >= 2 and HAN.match(parts[0]):
            ranked.append((parts[0], int(parts[1]), parts[2]))
    ranked.sort(key=lambda item: -item[1])
    lines = ["# 排名\t词\tjieba词频\tjieba词性\t专名类型\t默认拼音（pypinyin，多音字需人工确认）"]
    for rank, (word, frequency, tag) in enumerate(ranked[:args.top], 1):
        if word not in words:
            key = "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))
            lines.append(f"{rank}\t{word}\t{frequency}\t{tag}\t{POS.get(tag, '')}\t{key}")
    (args.output / "missing_common_words.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
