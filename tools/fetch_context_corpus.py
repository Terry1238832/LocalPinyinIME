"""Download, clean and split the attributed corpora used by the context model.

Sources (see resources/dictionary/authoring/context_lm_sources.json):
  * Tatoeba Mandarin sentences, CC BY 2.0 FR
  * Chinese Wikinews 2026-10-01 article dump, CC BY 2.5 / CC BY 4.0
  * Mozilla Common Voice zh-CN/zh-TW sentence text, CC0 1.0

Every line is converted to simplified Chinese. One line in forty (chosen by a
hash of its text) is held out of training; held-out Tatoeba and Common Voice
lines become the evaluation cases in tests/data. Only n-gram statistics built
from the remaining lines are distributed; no sentence text is packaged.

Authoring-time requirements: opencc, plus pypinyin and jieba for --write-eval.

  python tools/fetch_context_corpus.py --work <dir> [--write-eval tests/data]
"""
from __future__ import annotations

import argparse
import bz2
import hashlib
import re
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path

import opencc

COMMON_VOICE_COMMIT = "7015419eb71f82ac8121f91d3f4c5338b83d6844"
CV_BASE = f"https://raw.githubusercontent.com/common-voice/common-voice/{COMMON_VOICE_COMMIT}/server/data"
DOWNLOADS = {
    # Tatoeba publishes a rolling weekly export; the digest records the copy used.
    "cmn_sentences.tsv.bz2": ("https://downloads.tatoeba.org/exports/per_language/cmn/cmn_sentences.tsv.bz2",
                              "91ded8d50ee05db5aca76e23dd9c7f2eeb2e948bcb538fa301cc82f8f4d8048e"),
    "zhwikinews.xml.bz2": ("https://dumps.wikimedia.org/zhwikinews/20261001/zhwikinews-20261001-pages-articles.xml.bz2",
                           "d82f9ffc1bfb298989c58258032f49dde495669a830ad086eecbce32e00c0d3f"),
    "cv_zhcn_cn.txt": (f"{CV_BASE}/zh-CN/cn.txt", None),
    "cv_zhcn_sentence-collector.txt": (f"{CV_BASE}/zh-CN/sentence-collector.txt", None),
    "cv_zhcn_wiki.zh-cn.txt": (f"{CV_BASE}/zh-CN/wiki.zh-cn.txt", None),
    "cv_zhtw_sentence-collector.txt": (f"{CV_BASE}/zh-TW/sentence-collector.txt", None),
    "cv_zhtw_setences.txt": (f"{CV_BASE}/zh-TW/setences.txt", None),
    "cv_zhtw_chatlogs.txt": (f"{CV_BASE}/zh-TW/chatlogs.txt", None),
}
HELD_OUT_MODULO = 40


class Simplifier:
    """OpenCC conversion plus a few variants OpenCC leaves in simplified text."""
    REPLACEMENTS = (("妳", "你"), ("甚麽", "什么"), ("甚么", "什么"), ("祢", "你"))

    def __init__(self, profile: str) -> None:
        self.converter = opencc.OpenCC(profile)

    def convert(self, text: str) -> str:
        text = self.converter.convert(text)
        for old, new in self.REPLACEMENTS:
            text = text.replace(old, new)
        return text


T2S = Simplifier("t2s")
TW2SP = Simplifier("tw2sp")

VARIANT = re.compile(r"-\{(.*?)\}-", re.S)
COMMENT = re.compile(r"<!--.*?-->", re.S)
REF = re.compile(r"<ref[^>/]*?/>|<ref[^>]*?>.*?</ref>", re.S | re.I)
TAG = re.compile(r"<[^>]+>")
TABLE = re.compile(r"\{\|.*?\|\}", re.S)
FILE_LINK = re.compile(r"\[\[(?:File|Image|文件|檔案|档案|图像|圖像|Category|分类|分類|Template|模板)[:：][^\[\]]*"
                       r"(?:\[\[[^\]]*\]\][^\[\]]*)*\]\]", re.I)
LINK = re.compile(r"\[\[(?:[^\]|]*\|)?([^\]]*)\]\]")
INTERWIKI = re.compile(r"\[\[\s*(?:w|wp|wikipedia|zh|:w)\s*:([^\]|]*)\]\]", re.I)
EXT_LINK = re.compile(r"\[(?:https?:)?//[^\s\]]+\s*([^\]]*)\]")
URL = re.compile(r"https?://\S+")
HEADING = re.compile(r"^=+.*?=+\s*$", re.M)
QUOTES = re.compile(r"'{2,}")
INLINE_TEMPLATE = re.compile(r"\{\{\s*(?:w|wp|wikipedia|lang(?:-\w+)?|zh|ruby|link-\w+)\s*\|([^{}]*)\}\}", re.I)
HAN_CLAUSE = re.compile(r"[㐀-鿿]+")


def held_out(text: str) -> bool:
    return int(hashlib.sha256(text.encode("utf-8")).hexdigest()[:8], 16) % HELD_OUT_MODULO == 0


def download(directory: Path) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    for name, (url, digest) in DOWNLOADS.items():
        target = directory / name
        if not target.exists():
            print("download", url)
            urllib.request.urlretrieve(url, target)
        actual = hashlib.sha256(target.read_bytes()).hexdigest()
        if digest and actual != digest:
            print(f"warning: {name} sha256 {actual} differs from the recorded {digest}")


def inline_template(match: re.Match) -> str:
    # {{w|target|label}} shows the label; {{lang|code|text}} shows the text.
    parts = [part for part in match.group(1).split("|") if "=" not in part]
    return parts[-1] if parts else ""


def strip_templates(text: str) -> str:
    out, depth, index = [], 0, 0
    while index < len(text):
        if text.startswith("{{", index):
            depth += 1
            index += 2
        elif text.startswith("}}", index) and depth:
            depth -= 1
            index += 2
        else:
            if not depth:
                out.append(text[index])
            index += 1
    return "".join(out)


def variant(match: re.Match) -> str:
    body = match.group(1)
    if ":" not in body and "：" not in body:
        return body
    choices = {}
    for part in body.split(";"):
        if ":" in part:
            key, value = part.split(":", 1)
            choices[key.strip()] = value
    for key in ("zh-hans", "zh-cn", "zh-sg", "zh", "zh-hant", "zh-tw", "zh-hk"):
        if key in choices:
            return choices[key]
    return ""


def clean_wikitext(text: str) -> str:
    text = COMMENT.sub("", text)
    text = VARIANT.sub(variant, text)
    text = REF.sub("", text)
    for _ in range(2):
        text = INLINE_TEMPLATE.sub(inline_template, text)
    text = strip_templates(text)
    text = TABLE.sub("", text)
    text = FILE_LINK.sub("", text)
    text = INTERWIKI.sub(r"\1", text)
    for _ in range(2):
        text = LINK.sub(r"\1", text)
    for pattern, replacement in ((EXT_LINK, r"\1"), (URL, ""), (HEADING, ""), (TAG, ""), (QUOTES, "")):
        text = pattern.sub(replacement, text)
    lines = []
    for line in text.splitlines():
        line = line.strip().lstrip("*#:;").strip()
        if line and not line.startswith(("|", "!", "{", "}", "__")):
            lines.append(line)
    return "\n".join(lines)


def tatoeba_lines(path: Path) -> list[str]:
    lines = []
    with bz2.open(path, "rt", encoding="utf-8") as handle:
        for line in handle:
            parts = line.rstrip("\n").split("\t")
            if len(parts) == 3 and parts[1] == "cmn":
                lines.append(T2S.convert(parts[2].strip()))
    return lines


def common_voice_lines(directory: Path) -> list[str]:
    lines = []
    for path in sorted(directory.glob("cv_*.txt")):
        convert = TW2SP.convert if "_zhtw_" in path.name else T2S.convert
        lines.extend(convert(line.strip()) for line in path.read_text(encoding="utf-8").splitlines() if line.strip())
    return lines


def wikinews_lines(path: Path) -> list[str]:
    lines = []
    with bz2.open(path, "rb") as handle:
        namespace, redirect = None, False
        for event, element in ET.iterparse(handle, events=("start", "end")):
            tag = element.tag.rsplit("}", 1)[-1]
            if event == "start":
                if tag == "page":
                    namespace, redirect = None, False
                continue
            if tag == "ns":
                namespace = element.text
            elif tag == "redirect":
                redirect = True
            elif tag == "text" and namespace == "0" and not redirect and element.text:
                body = clean_wikitext(element.text)
                if len(body) > 40:
                    lines.extend(T2S.convert(body).splitlines())
            elif tag == "page":
                element.clear()
    return lines


def write_eval(held: dict[str, list[str]], directory: Path) -> None:
    import jieba
    from pypinyin import Style, lazy_pinyin
    jieba.setLogLevel(60)

    def pinyin(text: str) -> str | None:
        syllables = lazy_pinyin(text, style=Style.NORMAL, errors="ignore")
        if len(syllables) != len(text) or any(not s.isascii() or not s.isalpha() for s in syllables):
            return None
        return "".join(syllables)

    sentences, contexts = [], []
    for name, lines in held.items():
        seen = set()
        clauses = [clause for line in lines for clause in HAN_CLAUSE.findall(line) if 4 <= len(clause) <= 24]
        for index, clause in enumerate(clauses):
            key = pinyin(clause)
            if clause in seen or not key:
                continue
            seen.add(clause)
            sentences.append(f"{key}\t{clause}\t{name}:{index}")
            words = list(jieba.cut(clause, HMM=False))
            prefix = words[0]
            # Each later word is typed after the preceding words were committed.
            for position, word in enumerate(words[1:], 1):
                word_key = pinyin(word)
                if word_key and len(word) <= 4:
                    contexts.append(f"{word_key}\t{word}\t{name}:{index}:{position}\t{prefix}")
                prefix += word
    header = "# Held-out Tatoeba (CC BY 2.0 FR) and Common Voice (CC0) text; generated by tools/fetch_context_corpus.py\n"
    (directory / "heldout_sentences.tsv").write_text(header + "# pinyin\texpected\tsource\n" + "\n".join(sentences) + "\n",
                                                     encoding="utf-8", newline="\n")
    (directory / "heldout_context.tsv").write_text(header + "# pinyin\texpected\tsource\tpreceding\n" + "\n".join(contexts) + "\n",
                                                   encoding="utf-8", newline="\n")
    print(len(sentences), "sentence cases;", len(contexts), "context cases")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--work", type=Path, required=True, help="download and output directory")
    parser.add_argument("--write-eval", type=Path, help="directory for heldout_*.tsv evaluation cases")
    args = parser.parse_args()
    downloads = args.work / "downloads"
    download(downloads)
    sources = {
        "tatoeba": tatoeba_lines(downloads / "cmn_sentences.tsv.bz2"),
        "cv": common_voice_lines(downloads),
        "wikinews": wikinews_lines(downloads / "zhwikinews.xml.bz2"),
    }
    train = args.work / "train"
    train.mkdir(parents=True, exist_ok=True)
    held = {}
    for name, lines in sources.items():
        kept = [line for line in lines if not held_out(line)]
        held[name] = [line for line in lines if held_out(line)]
        (train / f"{name}.txt").write_text("\n".join(kept) + "\n", encoding="utf-8", newline="\n")
        print(name, len(kept), "training lines;", len(held[name]), "held out")
    if args.write_eval:
        write_eval({name: held[name] for name in ("tatoeba", "cv")}, args.write_eval)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
