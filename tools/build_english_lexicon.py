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
    the Wikipedia extract. One sentence in forty (chosen by a hash of its
    text) is held out of the counts; --write-eval writes some of them as
    evaluation sentences.

Possessives (dog's) are left out: the engine accepts a word followed by 's.
Only word counts are distributed; no sentence text is packaged.

  python tools/build_english_lexicon.py --work <dir> [--write-eval tests/data]
"""
from __future__ import annotations

import argparse
import collections
import hashlib
import re
import tarfile
import urllib.request
from pathlib import Path

SCOWL_VERSION = "2020.12.07"
COMMON_VOICE_COMMIT = "7015419eb71f82ac8121f91d3f4c5338b83d6844"
CV_BASE = f"https://raw.githubusercontent.com/common-voice/common-voice/{COMMON_VOICE_COMMIT}/server/data/en"
DOWNLOADS = {
    f"scowl-{SCOWL_VERSION}.tar.gz": (
        f"https://sourceforge.net/projects/wordlist/files/SCOWL/{SCOWL_VERSION}/scowl-{SCOWL_VERSION}.tar.gz/download",
        "5587667caa20c4891390c2d42dbb4d5c4c3f41bee77af1457ece3ba23fb859cc"),
    "cv_en_sentence-collector.txt": (f"{CV_BASE}/sentence-collector.txt",
                                     "31ac8e200449ebd7aee4a8c6bd16d4dafd076cf11f7c3efe1035f0283712d136"),
    "cv_en_wiki.en.txt": (f"{CV_BASE}/wiki.en.txt",
                          "438e55e8bc91dfa6cbe098a7233702c6b177c5ae07137a01d66b9c75c9789ae5"),
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
            urllib.request.urlretrieve(url, target)
        actual = hashlib.sha256(target.read_bytes()).hexdigest()
        if actual != digest:
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


def sentences(directory: Path) -> list[tuple[str, int]]:
    """Each sentence with the weight its source counts with."""
    lines: list[tuple[str, int]] = []
    for path in sorted(directory.glob("cv_en_*.txt")):
        weight = WEIGHTS.get(path.name, 1)
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.strip().replace("’", "'").replace("‘", "'")
            if line:
                lines.append((line, weight))
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
    for line, weight in sentences(downloads):
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
        handle.write(f"# Mozilla Common Voice English sentence text (CC0 1.0), {total} weighted words.\n")
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
