#!/usr/bin/env python3
"""Estimate how much of everyday Chinese the built-in lexicon covers.

Three views:
  1. Characters of the Table of General Standard Chinese Characters (通用规范
     汉字表, 8105 characters, from Unicode Unihan kTGH/kTGHZ2013): is each one a
     candidate for its standard reading?
  2. Common words: the jieba frequency list (MIT), ranked by frequency; what
     share of the top N words of two or more characters is in the lexicon,
     and can missing ones still be typed (the engine composes them)?
  3. Running text: held-out sentences segmented with jieba; share of word
     tokens found in the lexicon.

Authoring-time requirements: jieba, pypinyin. Usage:
  python tools/report_lexicon_coverage.py --unihan <Unihan dir> --eval-exe <LocalPinyinImeEval.exe>
      --resources <dir containing dictionary/> --text tests/data/heldout_sentences.tsv [--text more.txt]
      --output docs/lexicon-coverage.json
"""
from __future__ import annotations

import argparse
import collections
import json
import os
import re
import subprocess
import tempfile
import unicodedata
from pathlib import Path

import jieba
from pypinyin import Style, lazy_pinyin

HAN = re.compile(r"^[㐀-鿿]+$")
HAN_RUN = re.compile(r"[㐀-鿿]+")
LEXICONS = ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")


def toneless(reading: str) -> str:
    # Decompose first: ǚ is one precomposed character (u + diaeresis + caron).
    decomposed = unicodedata.normalize("NFD", reading).replace("ü", "v").replace("Ü", "v")
    return "".join(ch for ch in decomposed if not unicodedata.combining(ch)).lower()


def load_lexicon(directory: Path) -> tuple[set[tuple[str, str]], set[str]]:
    pairs, words = set(), set()
    for name in LEXICONS:
        for line in (directory / name).read_text(encoding="utf-8").splitlines():
            parts = line.split("\t")
            if line.startswith("#") or len(parts) < 2:
                continue
            pairs.add((parts[0], parts[1]))
            words.add(parts[1])
    return pairs, words


def standard_characters(unihan: Path) -> list[tuple[str, int, list[str]]]:
    serial, readings = {}, {}
    for name in ("Unihan_OtherMappings.txt", "Unihan_Readings.txt", "Unihan_IRGSources.txt", "Unihan_DictionaryLikeData.txt"):
        path = unihan / name
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            parts = line.split("\t")
            if len(parts) != 3:
                continue
            char = chr(int(parts[0][2:], 16))
            if parts[1] == "kTGH":
                serial[char] = int(parts[2].split(":")[1])
            elif parts[1] == "kTGHZ2013":
                readings[char] = sorted({toneless(item.split(":")[1]) for item in parts[2].split()})
    return sorted(((char, number, readings.get(char, [])) for char, number in serial.items()), key=lambda item: item[1])


def evaluate_typing(exe: Path, resources: Path, cases: list[tuple[str, str]]) -> list[int]:
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "cases.tsv"
        path.write_text("".join(f"{key}\t{word}\tcoverage:{index}\n" for index, (key, word) in enumerate(cases)), encoding="utf-8")
        env = dict(os.environ, LOCALPINYINIME_LOCALAPPDATA=temporary,
                   LOCALPINYINIME_USER_LEXICON_PATH=str(Path(temporary) / "user_lexicon.tsv"))
        output = subprocess.run([str(exe), str(resources), str(path)], stdout=subprocess.PIPE, check=True, env=env).stdout
    return [json.loads(line)["expected_rank"] for line in output.decode("utf-8").splitlines() if line]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--unihan", type=Path, required=True)
    parser.add_argument("--eval-exe", type=Path, required=True)
    parser.add_argument("--resources", type=Path, required=True)
    parser.add_argument("--text", type=Path, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    jieba.setLogLevel(60)
    pairs, words = load_lexicon(args.resources / "dictionary")
    report: dict = {"lexicon": {"entries": len(pairs), "distinct_words": len(words)}}

    # 1. Standard characters, by level.
    levels = {"一级 (1-3500)": (1, 3500), "二级 (3501-6500)": (3501, 6500), "三级 (6501-8105)": (6501, 8105)}
    characters = standard_characters(args.unihan)
    char_report = {}
    missing_chars: dict[str, list[str]] = {}
    for level, (low, high) in levels.items():
        chosen = [item for item in characters if low <= item[1] <= high]
        # Input methods spell lüe/nüe as lue/nue; accept either spelling.
        spellings = lambda reading: {reading, reading.replace("lve", "lue").replace("nve", "nue")}
        by_reading = [item for item in chosen if any((spelling, item[0]) in pairs
                                                     for reading in item[2] for spelling in spellings(reading))]
        anywhere = [item for item in chosen if item[0] in words]
        char_report[level] = {"characters": len(chosen), "typeable_by_standard_reading": len(by_reading),
                              "percent": round(100 * len(by_reading) / max(1, len(chosen)), 2), "single_entry_any_reading": len(anywhere)}
        missing_chars[level] = [f"{item[0]}({'/'.join(item[2])})" for item in chosen if item not in by_reading][:40]
    report["standard_characters"] = char_report
    report["standard_characters_missing_examples"] = missing_chars

    # 2. Common words from the jieba frequency list.
    dictionary = Path(jieba.__file__).parent / "dict.txt"
    ranked = []
    for line in dictionary.read_text(encoding="utf-8").splitlines():
        parts = line.split()
        if len(parts) >= 2 and len(parts[0]) >= 2 and HAN.match(parts[0]):
            ranked.append((parts[0], int(parts[1])))
    ranked.sort(key=lambda item: -item[1])
    word_report = {}
    for top in (1000, 5000, 10000, 30000, 50000, 100000):
        chosen = ranked[:top]
        found = sum(word in words for word, _ in chosen)
        word_report[f"top {top}"] = {"words": len(chosen), "in_lexicon": found, "percent": round(100 * found / len(chosen), 2)}
    report["common_words"] = word_report
    missing_top = [word for word, _ in ranked[:10000] if word not in words]
    report["common_words_missing_examples"] = missing_top[:80]
    # Missing common words may still be typeable: the engine composes them
    # from characters and shorter words.
    cases = [("".join(lazy_pinyin(word, style=Style.NORMAL)), word) for word in missing_top]
    cases = [(key, word) for key, word in cases if key.isascii() and key.isalpha()]
    ranks = evaluate_typing(args.eval_exe, args.resources, cases)
    report["common_words_missing_but_typed"] = {
        "words": len(ranks), "first_candidate": sum(rank == 1 for rank in ranks),
        "within_nine": sum(rank > 0 for rank in ranks),
        "percent_within_nine": round(100 * sum(rank > 0 for rank in ranks) / max(1, len(ranks)), 2)}

    # 3. Running text.
    text_report = {}
    for path in args.text:
        lines = path.read_text(encoding="utf-8").splitlines()
        if path.suffix == ".tsv":
            lines = [line.split("\t")[1] for line in lines if line and not line.startswith("#") and "\t" in line]
        tokens = collections.Counter()
        for line in lines:
            for run in HAN_RUN.findall(line):
                tokens.update(token for token in jieba.cut(run) if len(token) >= 2)
        total = sum(tokens.values())
        covered = sum(count for token, count in tokens.items() if token in words)
        missing = collections.Counter({token: count for token, count in tokens.items() if token not in words})
        text_report[path.name] = {
            "lines": len(lines), "word_tokens": total, "word_types": len(tokens),
            "token_coverage_percent": round(100 * covered / max(1, total), 2),
            "type_coverage_percent": round(100 * sum(token in words for token in tokens) / max(1, len(tokens)), 2),
            "frequent_missing": [f"{token}×{count}" for token, count in missing.most_common(40)]}
    report["running_text"] = text_report
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: report[key] for key in ("lexicon", "standard_characters", "common_words", "common_words_missing_but_typed")}, ensure_ascii=False, indent=1))
    for name, item in text_report.items():
        print(name, {k: v for k, v in item.items() if k != "frequent_missing"})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
