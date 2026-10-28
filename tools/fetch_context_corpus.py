"""Download, clean and split the attributed corpora used by the context model.

Sources (see resources/dictionary/authoring/context_lm_sources.json):
  * Tatoeba Mandarin sentences, CC BY 2.0 FR
  * Chinese Wikinews 2026-10-01 article dump, CC BY 2.5 / CC BY 4.0
  * Mozilla Common Voice zh-CN/zh-TW sentence text, CC0 1.0
  * KdConv and CrossWOZ dialogue utterances (Tsinghua thu-coai), Apache-2.0
  * FineWeb-2 Mandarin web text (Hugging Face), ODC-By 1.0; spam sites and
    Traditional-script pages dropped, lines deduplicated. The held-out shard
    trains the character model; it and every 8th row group of the first
    training shard (read with HTTP range requests, two interleaved sets)
    train the word model.

Every line is converted to simplified Chinese. One line in forty (chosen by a
hash of its text) is held out of training; held-out Tatoeba and Common Voice
lines become the evaluation cases in tests/data, held-out dialogue lines the
heldout_dialogue_* cases. Only n-gram statistics built from the remaining
lines are distributed; no sentence text is packaged.

Authoring-time requirements: opencc and pyarrow, plus pypinyin and jieba for --write-eval.

  python tools/fetch_context_corpus.py --work <dir> [--write-eval tests/data]
                                        [--write-dialogue-eval tests/data]
"""
from __future__ import annotations

import argparse
import bz2
import hashlib
import io
import json
import re
import urllib.request
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path

import opencc
import pyarrow.parquet as pq

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
# Everyday dialogue: chit-chat about films, music and travel (KdConv) and
# task-oriented requests for hotels, restaurants, sights, metro and taxis
# (CrossWOZ). Both repositories are Apache-2.0; pinned to these commits.
KDCONV_COMMIT = "653db76432de09a004ba708a68f8bbd5500e6bec"
CROSSWOZ_COMMIT = "df82c9fdff91b9b130f2d6b89110d3870ba6260e"
DIALOGUE_DOWNLOADS = {
    **{f"kdconv_{domain}_{split}.json": (
        f"https://raw.githubusercontent.com/thu-coai/KdConv/{KDCONV_COMMIT}/data/{domain}/{split}.json", digest)
       for (domain, split), digest in {
           ("film", "train"): "b48a491c30ace856914c6863679ba5c8ea95d7e3a1734f45177f69afbcd88e74",
           ("film", "dev"): "2f156d8d34bdd3eac31c196772156d36e017f8cb2255a2c312bd98e367ed363e",
           ("film", "test"): "5ba2b79f9791bedbe5fe8a1bf5d7cbf277697743ee426ecd05459d9e28bcf21b",
           ("music", "train"): "d5835bb6a61da0067ab2d0dc3ba90059f07cfa781a70206a8266ecafd22641ad",
           ("music", "dev"): "ee34a4af167195db43fdcc910f8d125956222517f0572d248a069057493d557f",
           ("music", "test"): "46f048c5db9dde45127cf6630b2fa06b685be369b71c5bc2dc3022d3f4bbad65",
           ("travel", "train"): "b582a7171f8a7bb6b579bbf8d31a06cebd0966e520c95301a143b1629d20fb2f",
           ("travel", "dev"): "8d1f31295888cf61eff398bab8aa042f7fccd04a088bd63c939f0b79625a9093",
           ("travel", "test"): "730426b0997b8375c8477c7e658c0ec645fced6ab8d546adda114b21cf30faac",
       }.items()},
    **{f"crosswoz_{split}.json.zip": (
        f"https://raw.githubusercontent.com/thu-coai/CrossWOZ/{CROSSWOZ_COMMIT}/data/crosswoz/{split}.json.zip", digest)
       for split, digest in {
           "train": "b81f6a0845011bb4593d21dd58e2d76af4dd5d52e300ecc68cb8dee17ccc3cf8",
           "val": "b2b717990f6f0a1c98daaf5dc465e61d42f7de69c0d31e88e674275b1f34c94c",
           "test": "e57703894995211bba8638f130dc7df81e8992b71f3c05c10022ad23d6ca5310",
       }.items()},
}
# General web text: the 87 MB held-out shard of the Mandarin subset, which is
# enough text to widen the model without a multi-gigabyte training shard.
FINEWEB_DOWNLOADS = {
    "fineweb2_cmn_Hani_test_000.parquet": (
        "https://huggingface.co/datasets/HuggingFaceFW/fineweb-2/resolve/main/data/cmn_Hani/test/000_00000.parquet",
        "22a90c471e09c8e938f3e070ff590864e74e01cc3c5cdf01c2713157fcdb43aa"),
}
# More web text for the word model: every 8th row group (1,000 pages each)
# of the first 4.8 GB training shard, as two sets starting at groups 0 and 8,
# read with range requests, text and url columns only (about 830 MB
# transferred).
FINEWEB_TRAIN_SHARD = (
    "https://huggingface.co/datasets/HuggingFaceFW/fineweb-2/resolve/main/data/cmn_Hani/train/000_00000.parquet",
    4841624662, "3e43fefabc3ee500f9874655ece1776f96b81568cf33e0a6376835425ce42598")
FINEWEB_TRAIN_STRIDE = 16
FINEWEB_TRAIN_SETS = {0: "fineweb2_cmn_Hani_train_000_every16.parquet", 8: "fineweb2_cmn_Hani_train_000_every16_from8.parquet"}
# Gambling and adult spam, whose generated pages splice unrelated sentences.
WEB_SPAM = re.compile(r"博彩|娱乐城|六合彩|时时彩|赌场|赌博|百家乐|彩票|开奖|真人视讯|老虎机|棋牌|色情|成人电影|av女优|威尼斯人|葡京|"
                      r"皇冠体育|投注")
# Characters written only in Traditional script: such pages read as Taiwan or
# Hong Kong usage even after conversion.
TRADITIONAL_ONLY = set("們這個說來時會對國過還麼為學無點東車問門開關書見長馬魚鳥體與從後經發現電話種當應實樣讓認識變處歡聽買賣錢"
                       "讀寫親愛幾員聯網話機場內產業廣號歲區際線級頭聲夠斷雖單鐘傳險獨極態響質議請論辦謝備課語萬數價職權條領導裡")
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


def download(directory: Path, downloads: dict[str, tuple[str, str | None]] = DOWNLOADS) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    for name, (url, digest) in downloads.items():
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


def kdconv_lines(directory: Path) -> list[str]:
    lines = []
    for path in sorted(directory.glob("kdconv_*.json")):
        for dialogue in json.loads(path.read_text(encoding="utf-8")):
            lines.extend(T2S.convert(turn["message"].strip()) for turn in dialogue["messages"] if turn.get("message", "").strip())
    return lines


def crosswoz_lines(directory: Path) -> list[str]:
    lines = []
    for path in sorted(directory.glob("crosswoz_*.json.zip")):
        with zipfile.ZipFile(path) as archive:
            for name in archive.namelist():
                for dialogue in json.loads(archive.read(name).decode("utf-8")).values():
                    lines.extend(T2S.convert(turn["content"].strip()) for turn in dialogue["messages"]
                                 if turn.get("content", "").strip())
    return lines


def generated_host(url: str) -> bool:
    # Spam farms use generated host names such as l50ozqi.com or 7ij9cfdiw.com.
    host = re.sub(r"^[a-z]+://", "", url.lower()).split("/", 1)[0]
    return any(sum(ch.isdigit() for ch in label) >= 2 and sum(ch.isalpha() for ch in label) >= 3
               for label in host.split("."))


class RangeReader(io.RawIOBase):
    """A seekable remote file read with HTTP range requests in 1 MB blocks."""

    def __init__(self, url: str, size: int, block: int = 1 << 20) -> None:
        self.url, self.size, self.block, self.position = url, size, block, 0
        self.cache: dict[int, bytes] = {}

    def readable(self) -> bool: return True
    def seekable(self) -> bool: return True
    def tell(self) -> int: return self.position

    def seek(self, offset: int, whence: int = 0) -> int:
        self.position = offset if whence == 0 else self.position + offset if whence == 1 else self.size + offset
        return self.position

    def fetch(self, start: int, end: int) -> bytes:
        request = urllib.request.Request(self.url, headers={"Range": f"bytes={start}-{end - 1}"})
        with urllib.request.urlopen(request, timeout=300) as response:
            return response.read()

    def read(self, count: int = -1) -> bytes:
        count = self.size - self.position if count < 0 else max(0, min(count, self.size - self.position))
        if count > self.block:
            data = self.fetch(self.position, self.position + count)
        else:
            data = b""
            while len(data) < count:
                index = (self.position + len(data)) // self.block
                if index not in self.cache:
                    if len(self.cache) > 4:
                        self.cache.clear()
                    start = index * self.block
                    self.cache[index] = self.fetch(start, min(self.size, start + self.block))
                chunk = self.cache[index][self.position + len(data) - index * self.block:]
                data += chunk[:count - len(data)]
        self.position += len(data)
        return data

    def readinto(self, buffer) -> int:
        data = self.read(len(buffer))
        buffer[:len(data)] = data
        return len(data)


def download_row_groups(directory: Path, first: int, name: str) -> None:
    target = directory / name
    if target.exists():
        return
    url, size, _ = FINEWEB_TRAIN_SHARD
    print("download every", FINEWEB_TRAIN_STRIDE, "th row group from", first, "of", url)
    source = pq.ParquetFile(RangeReader(url, size))
    writer = None
    for group in range(first, source.metadata.num_row_groups, FINEWEB_TRAIN_STRIDE):
        table = source.read_row_group(group, columns=["text", "url"])
        if writer is None:
            writer = pq.ParquetWriter(target, table.schema, compression="zstd")
        writer.write_table(table)
    writer.close()


def fineweb_lines(paths: list[Path]) -> list[str]:
    lines, seen = [], set()
    for path in paths:
        table = pq.ParquetFile(path)
        for group in range(table.num_row_groups):
            for row in table.read_row_group(group, columns=["text", "url"]).to_pylist():
                text = row["text"]
                if generated_host(row["url"]) or WEB_SPAM.search(text):
                    continue
                han = HAN_CLAUSE.findall(text)
                characters = sum(map(len, han))
                if not characters or sum(ch in TRADITIONAL_ONLY for run in han for ch in run) > 0.01 * characters:
                    continue
                for line in text.splitlines():
                    line = line.strip()
                    # Prose lines only: menus, tables and link lists are mostly not Han text.
                    count = sum(map(len, HAN_CLAUSE.findall(line)))
                    if count < 6 or count < 0.5 * len(line) or line in seen:
                        continue
                    seen.add(line)
                    lines.append(T2S.convert(line))
    return lines


def unseen(clauses: set[str], training: list[str]) -> set[str]:
    """The clauses that occur nowhere in the training text."""
    by_first: dict[str, list[str]] = {}
    for clause in clauses:
        by_first.setdefault(clause[0], []).append(clause)
    seen = set()
    for line in training:
        for run in HAN_CLAUSE.findall(line):
            for item in {item for ch in set(run) for item in by_first.get(ch, ())}:
                if item in run:
                    seen.add(item)
    return clauses - seen


def write_eval(held: dict[str, list[str]], directory: Path, stem: str = "heldout",
               header: str = "# Held-out Tatoeba (CC BY 2.0 FR) and Common Voice (CC0) text; generated by tools/fetch_context_corpus.py\n",
               training: list[str] | None = None) -> None:
    import jieba
    from pypinyin import Style, lazy_pinyin
    jieba.setLogLevel(60)

    def pinyin(text: str) -> str | None:
        syllables = lazy_pinyin(text, style=Style.NORMAL, errors="ignore")
        if len(syllables) != len(text) or any(not s.isascii() or not s.isalpha() for s in syllables):
            return None
        return "".join(syllables)

    sentences, contexts = [], []
    # Dialogue repeats set phrases (谢谢你啊, 营业时间是什么): a held-out
    # clause that also occurs in training text would only test recall, so
    # with `training` given only unseen clauses become cases.
    novel = None
    if training is not None:
        novel = unseen({clause for lines in held.values() for line in lines for clause in HAN_CLAUSE.findall(line)
                        if 4 <= len(clause) <= 24}, training)
    for name, lines in held.items():
        seen = set()
        clauses = [clause for line in lines for clause in HAN_CLAUSE.findall(line)
                   if 4 <= len(clause) <= 24 and (novel is None or clause in novel)]
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
    (directory / f"{stem}_sentences.tsv").write_text(header + "# pinyin\texpected\tsource\n" + "\n".join(sentences) + "\n",
                                                     encoding="utf-8", newline="\n")
    (directory / f"{stem}_context.tsv").write_text(header + "# pinyin\texpected\tsource\tpreceding\n" + "\n".join(contexts) + "\n",
                                                   encoding="utf-8", newline="\n")
    print(len(sentences), "sentence cases;", len(contexts), "context cases")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--work", type=Path, required=True, help="download and output directory")
    parser.add_argument("--write-eval", type=Path, help="directory for heldout_*.tsv evaluation cases")
    parser.add_argument("--write-dialogue-eval", type=Path, help="directory for heldout_dialogue_*.tsv evaluation cases")
    args = parser.parse_args()
    downloads = args.work / "downloads"
    download(downloads)
    dialogue = args.work / "dialogue"
    download(dialogue, DIALOGUE_DOWNLOADS)
    web = args.work / "web"
    download(web, FINEWEB_DOWNLOADS)
    for first, name in FINEWEB_TRAIN_SETS.items():
        download_row_groups(web, first, name)
    sources = {
        "tatoeba": tatoeba_lines(downloads / "cmn_sentences.tsv.bz2"),
        "cv": common_voice_lines(downloads),
        "wikinews": wikinews_lines(downloads / "zhwikinews.xml.bz2"),
        "kdconv": kdconv_lines(dialogue),
        "crosswoz": crosswoz_lines(dialogue),
        "fineweb": fineweb_lines([web / name for name in FINEWEB_DOWNLOADS]),
        "fineweb_extra": fineweb_lines([web / FINEWEB_TRAIN_SETS[0]]),
        "fineweb_extra2": fineweb_lines([web / FINEWEB_TRAIN_SETS[8]]),
    }
    train = args.work / "train"
    train.mkdir(parents=True, exist_ok=True)
    held, training = {}, []
    for name, lines in sources.items():
        kept = [line for line in lines if not held_out(line)]
        held[name] = [line for line in lines if held_out(line)]
        training.extend(kept)
        (train / f"{name}.txt").write_text("\n".join(kept) + "\n", encoding="utf-8", newline="\n")
        print(name, len(kept), "training lines;", len(held[name]), "held out")
    if args.write_eval:
        write_eval({name: held[name] for name in ("tatoeba", "cv")}, args.write_eval)
    if args.write_dialogue_eval:
        write_eval({name: held[name] for name in ("kdconv", "crosswoz")}, args.write_dialogue_eval, "heldout_dialogue",
                   "# Held-out KdConv and CrossWOZ dialogue text (Apache-2.0, Tsinghua thu-coai), only clauses that occur "
                   "in no training text; generated by tools/fetch_context_corpus.py\n", training)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
