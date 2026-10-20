"""Build the English word list for 本地英文 (resources/dictionary/english_words.tsv).

Sources (resources/dictionary/authoring/english_lexicon_sources.json):
  * SCOWL 2020.12.07 by Kevin Atkinson and contributors (permissive license,
    notice in resources/THIRD_PARTY_NOTICES.txt): which spellings are words.
    The english, american and special lists up to size 60 are offered as
    corrections and completions. British, Canadian and Australian spellings up
    to size 60 and every list's size 70 words are accepted as typed (colour
    and romanized are not corrected) but never offered, and neither are the
    words SCOWL lists as profane or offensive.
  * Mozilla Common Voice English sentence text (CC0 1.0): how often each word
    is used, Sentence Collector lines (prose and dialogue) weighted 8 against
    the Wikipedia extract.
  * Tatoeba English sentences (CC BY 2.0 FR, by the Tatoeba contributors):
    everyday short sentences, so common spoken words (I'm, hello) count.
  * English Wikinews articles (CC BY 2.5 / 4.0, by the Wikinews
    contributors): news prose, with the names and words of current events.
  One sentence in forty of every source (chosen by a hash of its text) is
  held out of the counts; --write-eval writes Common Voice ones as
  evaluation sentences.

Possessives (dog's) are left out: the engine accepts a word followed by 's.
Only word counts are distributed; no sentence text is packaged.

  python tools/build_english_lexicon.py --work <dir> [--write-eval tests/data]
"""
from __future__ import annotations

import argparse
import bz2
import collections
import hashlib
import re
import tarfile
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path

SCOWL_VERSION = "2020.12.07"
COMMON_VOICE_COMMIT = "7015419eb71f82ac8121f91d3f4c5338b83d6844"
CV_BASE = f"https://raw.githubusercontent.com/common-voice/common-voice/{COMMON_VOICE_COMMIT}/server/data/en"
# Tatoeba's export is renewed weekly: this is the one of 2026-10-17.
TATOEBA_SHA256 = "bb3dcc28a8984a26dc466ee14eb53c4210b0dd9bb111b9d686d1efc83ecf091c"
WIKINEWS_DUMP = "20261001"
WIKINEWS_SHA256 = "2fb488c718b176bebf6f8c5ca7378731d29758f21ad7c9147dd78e558d600ad7"
DOWNLOADS = {
    f"scowl-{SCOWL_VERSION}.tar.gz": (
        f"https://sourceforge.net/projects/wordlist/files/SCOWL/{SCOWL_VERSION}/scowl-{SCOWL_VERSION}.tar.gz/download",
        "5587667caa20c4891390c2d42dbb4d5c4c3f41bee77af1457ece3ba23fb859cc"),
    "cv_en_sentence-collector.txt": (f"{CV_BASE}/sentence-collector.txt",
                                     "31ac8e200449ebd7aee4a8c6bd16d4dafd076cf11f7c3efe1035f0283712d136"),
    "cv_en_wiki.en.txt": (f"{CV_BASE}/wiki.en.txt",
                          "438e55e8bc91dfa6cbe098a7233702c6b177c5ae07137a01d66b9c75c9789ae5"),
    "tatoeba_eng_sentences.tsv.bz2": ("https://downloads.tatoeba.org/exports/per_language/eng/eng_sentences.tsv.bz2",
                                      TATOEBA_SHA256),
    f"enwikinews-{WIKINEWS_DUMP}-pages-articles.xml.bz2": (
        f"https://dumps.wikimedia.org/enwikinews/{WIKINEWS_DUMP}/enwikinews-{WIKINEWS_DUMP}-pages-articles.xml.bz2",
        WIKINEWS_SHA256),
}
SIZES = (10, 20, 35, 40, 50, 55, 60)
# Rarer words (size 70) are accepted as typed, so they are not corrected
# into common ones (romanized is not organized), but never offered.
ACCEPTED_SIZE = 70
OFFERED = ("english", "american", "special")
ACCEPTED = ("british", "british_z", "canadian", "australian")
TABOO = ("profane.1", "offensive.1", "offensive.2")
# Sentence Collector lines are mostly prose and dialogue from books; the far
# larger Wikipedia extract alone would make everyday words (I'm, hello) rare.
WEIGHTS = {"cv_en_sentence-collector.txt": 8, "cv_en_wiki.en.txt": 1}
# Tatoeba's short everyday sentences count twice, Wikinews' news prose once:
# measured on held-out sentences of all three sources, more Tatoeba helps
# everyday text but costs Wikipedia-like text more than it gains.
TATOEBA_WEIGHT = 2
WIKINEWS_WEIGHT = 1
# Counts are scaled to this many weighted words, the Common Voice total the
# speller's fixed thresholds (a word counted at least 3 times may be a
# correction; a user's word counts 200) were measured with. More text then
# changes which words are common, not how common every word looks.
NORMALIZED_TOTAL = 17_339_963
HELD_OUT_MODULO = 40
EVAL_SENTENCES = 2000
WORD = re.compile(r"[A-Za-z]+(?:'[A-Za-z]+)*")
SPELLING = re.compile(r"[A-Za-z]+(?:'[A-Za-z]+)*")
# SCOWL lists these 's contractions among the possessives, which are dropped.
S_CONTRACTIONS = {"it's", "he's", "she's", "that's", "let's", "what's", "there's", "here's", "who's",
                  "where's", "how's", "when's", "why's", "one's", "everyone's", "everybody's",
                  "someone's", "somebody's", "nobody's"}


def held_out(text: str) -> bool:
    return int(hashlib.sha256(text.encode("utf-8")).hexdigest()[:8], 16) % HELD_OUT_MODULO == 0


def download(directory: Path) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    for name, (url, digest) in DOWNLOADS.items():
        target = directory / name
        if not target.exists():
            print("download", url)
            # Wikimedia refuses requests without a descriptive User-Agent.
            request = urllib.request.Request(url, headers={
                "User-Agent": "LocalPinyinIME-lexicon-builder/1.0 (https://github.com/Terry1238832/LocalPinyinIME)"})
            with urllib.request.urlopen(request) as response, target.open("wb") as handle:
                while chunk := response.read(1 << 20):
                    handle.write(chunk)
        actual = hashlib.sha256(target.read_bytes()).hexdigest()
        if digest is None:
            print(f"{name}: sha256 {actual} (record it)")
        elif actual != digest:
            raise SystemExit(f"{name}: sha256 {actual} differs from the recorded {digest}")


def scowl_lists(archive: Path) -> tuple[set[str], set[str], set[str]]:
    """Offered words, accepted-only words and taboo words."""
    offered: set[str] = set()
    accepted: set[str] = set()
    taboo: set[str] = set()
    prefix = f"scowl-{SCOWL_VERSION}/"
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            if not member.isfile() or not member.name.startswith(prefix):
                continue
            path = member.name[len(prefix):]
            folder, _, name = path.partition("/")
            if folder == "misc" and name in TABOO:
                target = taboo
            elif folder == "final" and "." in name:
                stem, size = name.rsplit(".", 1)
                category = stem.split("-", 1)[0]
                if not size.isdigit() or int(size) not in SIZES + (ACCEPTED_SIZE,):
                    continue
                if category in OFFERED and int(size) in SIZES:
                    target = offered
                elif category in OFFERED + ACCEPTED:
                    target = accepted
                else:
                    continue
            else:
                continue
            # Possessives (dog's) are dropped; contractions (it's, let's) stay.
            contractions = "-contractions." in name
            text = tar.extractfile(member).read().decode("latin-1")
            for line in text.splitlines():
                word = line.strip()
                if SPELLING.fullmatch(word) and (contractions or not word.endswith("'s") or word in S_CONTRACTIONS):
                    target.add(word)
    return offered, accepted - offered, taboo


# Wikitext to plain text (as tools/fetch_context_corpus.py does for Chinese).
COMMENT = re.compile(r"<!--.*?-->", re.S)
REF = re.compile(r"<ref[^>/]*?/>|<ref[^>]*?>.*?</ref>", re.S | re.I)
TAG = re.compile(r"<[^>]+>")
TABLE = re.compile(r"\{\|.*?\|\}", re.S)
FILE_LINK = re.compile(r"\[\[(?:File|Image|Category|Template|Media)\s*:[^\[\]]*(?:\[\[[^\]]*\]\][^\[\]]*)*\]\]", re.I)
LINK = re.compile(r"\[\[(?:[^\]|]*\|)?([^\]]*)\]\]")
INTERWIKI = re.compile(r"\[\[\s*(?:w|wp|wikipedia|:w)\s*:([^\]|]*)\]\]", re.I)
EXT_LINK = re.compile(r"\[(?:https?:)?//[^\s\]]+\s*([^\]]*)\]")
URL = re.compile(r"https?://\S+")
HEADING = re.compile(r"^=+.*?=+\s*$", re.M)
QUOTES = re.compile(r"'{2,}")
INLINE_TEMPLATE = re.compile(r"\{\{\s*(?:w|wp|wikipedia|lang(?:-\w+)?)\s*\|([^{}]*)\}\}", re.I)
SENTENCE_END = re.compile(r"(?<=[.!?])\s+(?=[A-Z\"'])")


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


def clean_wikitext(text: str) -> str:
    text = COMMENT.sub("", text)
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
            if len(parts) == 3 and parts[1] == "eng" and parts[2].strip():
                lines.append(parts[2].strip())
    return lines


def wikinews_lines(path: Path) -> list[str]:
    """Article text, one sentence per line (paragraphs split at . ! ?)."""
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
                if len(body) > 80:
                    for paragraph in body.splitlines():
                        lines.extend(part.strip() for part in SENTENCE_END.split(paragraph) if len(part.strip()) > 20)
            elif tag == "page":
                element.clear()
    return lines


def sentences(directory: Path, tatoeba_weight: int = TATOEBA_WEIGHT,
              wikinews_weight: int = WIKINEWS_WEIGHT) -> list[tuple[str, int]]:
    """Each sentence with the weight its source counts with."""
    lines: list[tuple[str, int]] = []
    def add(texts, weight):
        if weight <= 0:
            return
        for line in texts:
            line = line.strip().replace("’", "'").replace("‘", "'")
            if line:
                lines.append((line, weight))
    for path in sorted(directory.glob("cv_en_*.txt")):
        add(path.read_text(encoding="utf-8").splitlines(), WEIGHTS.get(path.name, 1))
    add(tatoeba_lines(directory / "tatoeba_eng_sentences.tsv.bz2"), tatoeba_weight)
    add(wikinews_lines(directory / f"enwikinews-{WIKINEWS_DUMP}-pages-articles.xml.bz2"), wikinews_weight)
    return lines


def written_form(spellings: list[str], case_counts: collections.Counter) -> str:
    # A lowercase spelling is the word itself; capitals then come from how it
    # is typed (Shift). A word SCOWL lists only with capitals keeps them
    # (English, Monday, I'm); among several, the most common inside sentences.
    lower = [spelling for spelling in spellings if spelling == spelling.lower()]
    # SCOWL also lists the letter i, but the word is always I.
    if "I" in spellings:
        return "I"
    capitalized = max(sorted(set(spellings) - set(lower)), key=lambda spelling: case_counts[spelling], default=None)
    if lower:
        # A rare lowercase sense (the billiards term english) gives way when
        # the capitalized word is written ten times as often inside sentences.
        if capitalized and len(capitalized) > 1 and case_counts[capitalized] >= 20 and \
                case_counts[capitalized] >= 10 * case_counts[lower[0]]:
            return capitalized
        return lower[0]
    return capitalized


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).resolve().parent.parent / "resources/dictionary/english_words.tsv")
    parser.add_argument("--write-eval", type=Path)
    parser.add_argument("--tatoeba-weight", type=int, default=TATOEBA_WEIGHT)
    parser.add_argument("--wikinews-weight", type=int, default=WIKINEWS_WEIGHT)
    parser.add_argument("--no-normalize", action="store_true")
    args = parser.parse_args()

    downloads = args.work / "downloads"
    download(downloads)
    offered, accepted, taboo = scowl_lists(downloads / f"scowl-{SCOWL_VERSION}.tar.gz")

    spellings: dict[str, list[str]] = collections.defaultdict(list)
    for word in offered | accepted:
        spellings[word.lower()].append(word)

    counts: collections.Counter = collections.Counter()
    case_counts: collections.Counter = collections.Counter()
    held: list[str] = []
    for line, weight in sentences(downloads, args.tatoeba_weight, args.wikinews_weight):
        if held_out(line):
            held.append(line)
            continue
        for index, match in enumerate(WORD.finditer(line)):
            token = match.group(0)
            key = token.lower()
            # A possessive counts for its word (Pantanal's for Pantanal).
            if key not in spellings and key.endswith("'s"):
                key = key[:-2]
            counts[key] += weight
            if index > 0:
                case_counts[token] += weight
    raw_total = sum(counts.values())
    if not args.no_normalize and raw_total > NORMALIZED_TOTAL:
        scale = NORMALIZED_TOTAL / raw_total
        counts = collections.Counter({key: round(count * scale) for key, count in counts.items()})
    rows = []
    for key, forms in spellings.items():
        form = written_form(forms, case_counts)
        offer = any(spelling in offered for spelling in forms) and \
            not any(spelling.lower() in taboo or spelling in taboo for spelling in forms)
        rows.append((form, counts.get(key, 0), "S" if offer else "V"))
    rows.sort(key=lambda row: (-row[1], row[0].lower(), row[0]))

    total = sum(counts.values())
    with args.output.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write("# English words for 本地英文. Generated by tools/build_english_lexicon.py; do not edit.\n")
        handle.write(f"# SCOWL {SCOWL_VERSION} spellings (notice in THIRD_PARTY_NOTICES.txt); counts from\n")
        handle.write("# Mozilla Common Voice English sentence text (CC0 1.0), Tatoeba English sentences (CC BY 2.0 FR,\n")
        handle.write(f"# Tatoeba contributors) and English Wikinews articles (CC BY 2.5, Wikinews contributors),\n")
        handle.write(f"# {total} weighted words.\n")
        handle.write("# written form\tcount\tS = offered, V = accepted as typed only\n")
        for form, count, flag in rows:
            handle.write(f"{form}\t{count}\t{flag}\n")
    print(f"{len(rows)} words ({sum(1 for row in rows if row[2] == 'S')} offered), {total} counted tokens,"
          f" {len(held)} held-out sentences -> {args.output}")

    if args.write_eval:
        args.write_eval.mkdir(parents=True, exist_ok=True)
        picked = sorted(held, key=lambda text: hashlib.sha256(text.encode("utf-8")).hexdigest())[:EVAL_SENTENCES]
        target = args.write_eval / "english_heldout_sentences.txt"
        with target.open("w", encoding="utf-8", newline="\n") as handle:
            handle.write("# Held-out Mozilla Common Voice English sentences (CC0 1.0), never counted in\n")
            handle.write("# english_words.tsv. Written by tools/build_english_lexicon.py --write-eval.\n")
            for line in sorted(picked):
                handle.write(line + "\n")
        print(f"{len(picked)} evaluation sentences -> {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
