"""Build the chat phrase counts used to order a short phrase typed on its own.

How often a two to six character phrase is a whole clause of real chat text
(LCCC-base, tools/fetch_context_corpus.py) says more about what someone means
by nicai than the lexicon does: 你猜 occurs 9,121 times and 你才 287. Clauses
containing an evaluation sentence are skipped as for the other models, and so
are phrases with the words below. Only hashes and rounded counts are packaged.

Format (little endian): "LPPC", u32 version 1, u32 n, n sorted u32 keys (high
half of FNV-1a over UTF-16 units), n u8 levels (log10(count) * 32).

  python tools/build_phrase_counts.py --input <work>/train/lccc.txt --exclude <case tsv> ... --output resources/dictionary/phrase_zh.lm
"""
from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from build_context_lm import HAN_RUN, excluded_texts  # noqa: E402

MIN_LENGTH, MAX_LENGTH = 2, 6
MIN_COUNT = 5
# A phrase with one of these never moves ahead of its homophones.
UNWANTED = ("傻逼", "煞笔", "傻b", "妈的", "你妈", "尼玛", "泥马", "他妈", "特么", "草泥马", "贱人", "婊", "鸡巴", "屌", "屄", "操你", "日你", "滚蛋", "去死", "脑残", "智障",
            "狗日", "王八蛋", "混蛋", "变态", "骚货", "约炮", "做爱", "睡你", "关你屁事", "放屁", "屁事", "傻叉", "二逼", "牛逼", "装逼", "逗逼", "苦逼", "傻子", "白痴")


def key(text: str) -> int:
    value = 0xCBF29CE484222325
    data = text.encode("utf-16-le")
    for index in range(0, len(data), 2):
        value = ((value ^ (data[index] | data[index + 1] << 8)) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value >> 32


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--exclude", type=Path, nargs="*", default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    by_first: dict[str, list[str]] = {}
    for item in excluded_texts(args.exclude):
        by_first.setdefault(item[0], []).append(item)
    counts: Counter[str] = Counter()
    with args.input.open(encoding="utf-8") as lines:
        for line in lines:
            for clause in HAN_RUN.findall(line):
                if MIN_LENGTH <= len(clause) <= MAX_LENGTH:
                    counts[clause] += 1
    kept = {}
    for phrase, count in counts.items():
        if count < MIN_COUNT or any(word in phrase for word in UNWANTED):
            continue
        if any(item in phrase for item in {i for ch in set(phrase) for i in by_first.get(ch, ())}):
            continue
        hashed = key(phrase)
        level = min(255, round(math.log10(count) * 32))
        kept[hashed] = max(kept.get(hashed, 0), level)
    keys = sorted(kept)
    with args.output.open("wb") as out:
        out.write(b"LPPC")
        array.array("I", [1, len(keys)]).tofile(out)
        array.array("I", keys).tofile(out)
        out.write(bytes(kept[item] for item in keys))
    metadata = {"format": "LPPC", "format_version": 1, "phrases": len(keys), "minimum_count": MIN_COUNT,
                "lengths": [MIN_LENGTH, MAX_LENGTH], "input": args.input.name,
                "input_sha256": hashlib.sha256(args.input.read_bytes()).hexdigest()}
    Path(str(args.output) + ".metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(metadata))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
