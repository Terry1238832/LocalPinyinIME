"""Build the chat phrase resources used for short input and for initials.

phrase_zh.lm: how often a two to six character phrase is a whole clause of real
chat text (LCCC-base, tools/fetch_context_corpus.py) says more about what someone
means by nicai than the lexicon does: 你猜 occurs 9,121 times and 你才 287. Only
hashes and rounded counts are stored.

Format (little endian): "LPPC", u32 version 1, u32 n, n sorted u32 keys (high
half of FNV-1a over UTF-16 units), n u8 levels (log10(count) * 32).

phrase_initials_zh.tsv: the most frequent whole clauses for each string of
consonant initials (ndlm: 你到了吗), which no decoding can rank as well as simply
having seen them. This one holds text: clauses of two to ten characters seen at
least 20 times, so everyday expressions rather than anyone's own words. One
line for each key, sorted by UTF-8 bytes: key, a tab, phrases best first
separated by |.

Evaluation sentences are not held out here. The counts come from the chat text,
not from the tests, and an input method has to know a sentence as common as 你吃
了吗 (held out, 你吃了么 would win with a fifth of its count). The chat evaluation
sets are held out by dialogue, so they remain the fair measure.

Authoring-time requirement: pypinyin.

  python tools/build_phrase_counts.py --input <work>/train/lccc.txt --output resources/dictionary/phrase_zh.lm
                                      --initials-output resources/dictionary/phrase_initials_zh.tsv
"""
from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from build_context_lm import HAN_RUN  # noqa: E402
from pypinyin import Style, lazy_pinyin  # noqa: E402

COUNT_LENGTHS = 2, 6
COUNT_MINIMUM = 5
INITIALS_LENGTHS = 2, 10
INITIALS_MINIMUM = 20
INITIALS_LONGEST_KEY = 12
PHRASES_PER_KEY = 9
# A phrase with one of these is never preferred or offered from these tables.
UNWANTED = ("傻逼", "煞笔", "傻b", "妈的", "你妈", "尼玛", "泥马", "他妈", "特么", "草泥马", "贱人", "婊", "鸡巴", "屌", "屄", "操你", "日你", "滚蛋", "去死", "脑残", "智障",
            "狗日", "王八蛋", "混蛋", "变态", "骚货", "约炮", "做爱", "睡你", "关你屁事", "放屁", "屁事", "傻叉", "二逼", "牛逼", "装逼", "逗逼", "苦逼", "傻子", "白痴")


def key(text: str) -> int:
    value = 0xCBF29CE484222325
    data = text.encode("utf-16-le")
    for index in range(0, len(data), 2):
        value = ((value ^ (data[index] | data[index + 1] << 8)) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value >> 32


def initials(text: str) -> str:
    return "".join(syllable[0] for syllable in lazy_pinyin(text, style=Style.NORMAL, errors="ignore"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--initials-output", type=Path, required=True)
    args = parser.parse_args()
    counts: Counter[str] = Counter()
    with args.input.open(encoding="utf-8") as lines:
        for line in lines:
            for clause in HAN_RUN.findall(line):
                if min(COUNT_LENGTHS[0], INITIALS_LENGTHS[0]) <= len(clause) <= max(COUNT_LENGTHS[1], INITIALS_LENGTHS[1]):
                    counts[clause] += 1
    wanted = {phrase: count for phrase, count in counts.items() if count >= min(COUNT_MINIMUM, INITIALS_MINIMUM) and not any(word in phrase for word in UNWANTED)}

    kept: dict[int, int] = {}
    for phrase, count in wanted.items():
        if COUNT_LENGTHS[0] <= len(phrase) <= COUNT_LENGTHS[1] and count >= COUNT_MINIMUM:
            hashed = key(phrase)
            kept[hashed] = max(kept.get(hashed, 0), min(255, round(math.log10(count) * 32)))
    keys = sorted(kept)
    with args.output.open("wb") as out:
        out.write(b"LPPC")
        array.array("I", [1, len(keys)]).tofile(out)
        array.array("I", keys).tofile(out)
        out.write(bytes(kept[item] for item in keys))

    by_initials: dict[str, list[tuple[int, str]]] = defaultdict(list)
    for phrase, count in wanted.items():
        if not INITIALS_LENGTHS[0] <= len(phrase) <= INITIALS_LENGTHS[1] or count < INITIALS_MINIMUM:
            continue
        typed = initials(phrase)
        if len(typed) == len(phrase) and len(typed) <= INITIALS_LONGEST_KEY and typed.isascii() and typed.isalpha() and not set(typed) & set("aeiouv"):
            by_initials[typed].append((-count, phrase))
    with args.initials_output.open("wb") as out:
        for typed in sorted(by_initials, key=lambda item: item.encode("utf-8")):
            phrases = [phrase for _, phrase in sorted(by_initials[typed])[:PHRASES_PER_KEY]]
            out.write((typed + "\t" + "|".join(phrases) + "\n").encode("utf-8"))

    metadata = {"format": "LPPC", "format_version": 1, "phrases": len(keys), "minimum_count": COUNT_MINIMUM, "lengths": list(COUNT_LENGTHS),
                "initials_keys": len(by_initials), "initials_minimum_count": INITIALS_MINIMUM, "initials_lengths": list(INITIALS_LENGTHS),
                "phrases_per_key": PHRASES_PER_KEY, "input": args.input.name, "input_sha256": hashlib.sha256(args.input.read_bytes()).hexdigest()}
    Path(str(args.output) + ".metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(metadata))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
