#!/usr/bin/env python3
"""Build the usage-scene word packs (常用场景) from the authored word lists.

resources/dictionary/authoring/domains/<id>.txt holds words, one or more per
line. Each word takes the reading the bundled dictionary already gives it (the
most frequent reading when it has several); a word the dictionary lacks is
read with pypinyin, an authoring-only dependency, and listed for review.
The packs are written to resources/dictionary/domains/<id>.tsv as
pinyin<TAB>word, in the order of the list.

Usage: python tools/build_domain_packs.py
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DICTIONARY = ROOT / "resources/dictionary"
LAYERS = ["core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv"]
HAN = re.compile(r"^[㐀-鿿]+$")


def readings() -> dict[str, dict[str, int]]:
    """word -> {pinyin key: best frequency over the layers}."""
    result: dict[str, dict[str, int]] = {}
    for name in LAYERS:
        for line in (DICTIONARY / name).read_text(encoding="utf-8").splitlines():
            parts = line.split("\t")
            if line.startswith("#") or len(parts) < 3:
                continue
            try:
                frequency = int(parts[2])
            except ValueError:
                continue
            key = parts[0].replace("'", "")
            keys = result.setdefault(parts[1], {})
            keys[key] = max(keys.get(key, 0), frequency)
    return result


def main() -> int:
    known = readings()
    review: list[str] = []
    for source in sorted((DICTIONARY / "authoring/domains").glob("*.txt")):
        words: list[str] = []
        for line in source.read_text(encoding="utf-8").splitlines():
            if line.startswith("#"):
                continue
            for word in line.split():
                if not HAN.match(word):
                    raise SystemExit(f"{source.name}: not a Chinese word: {word}")
                if word not in words:
                    words.append(word)
        rows = []
        for word in words:
            if word in known:
                key = max(known[word].items(), key=lambda item: item[1])[0]
            else:
                from pypinyin import Style, lazy_pinyin  # authoring only
                key = "".join(lazy_pinyin(word, style=Style.NORMAL, v_to_u=False))
                review.append(f"{source.stem}\t{word}\t{key}")
            rows.append(f"{key}\t{word}")
        header = [f"# Usage-scene pack '{source.stem}', built by tools/build_domain_packs.py from",
                  f"# resources/dictionary/authoring/domains/{source.name}. Project-authored (MIT).",
                  "# pinyin<TAB>word"]
        output = DICTIONARY / "domains" / f"{source.stem}.tsv"
        output.parent.mkdir(exist_ok=True)
        output.write_text("\n".join(header + rows) + "\n", encoding="utf-8", newline="\n")
        print(f"{source.stem}: {len(rows)} words")
    if review:
        print("Read with pypinyin (not in the dictionary), check these readings:")
        print("\n".join(review))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
