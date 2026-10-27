"""Build the word association model used beside the character language model.

The character 4-gram model sees at most three characters of history, so it
cannot tell 这家店是专门卖 from 这家电视专门买 once the deciding word is
further back. This model scores pairs of adjacent lexicon words instead.

Training text is segmented with the bundled lexicon (unigram dynamic
programming, re-estimated once from the first segmentation). For every
adjacent pair seen often enough, the model stores how much more often the pair
occurs than its words would by chance:

    score(a, b) = ln((count(a b) - D) * N / (count(a) * count(b)))

Only positive associations are kept, so an unseen pair scores zero and a
stored pair is a bonus. Clauses containing an evaluation sentence are skipped
exactly as for the character model. Only pair statistics are packaged.

Binary layout (little endian):
  magic "LPWM", u32 version, u32 scale, u32 word_count, u32 pair_count,
  u32 word_keys[word_count] sorted ascending (words that begin a stored pair),
  u32 pair_keys[pair_count] sorted ascending, i16 pair_scores[pair_count].
Keys are the high 32 bits of 64-bit FNV-1a over UTF-16 code units; a pair
hashes its first word, U+0001, then its second word.

Requires numpy at authoring time only.

  python tools/build_word_model.py --input <text>:<weight> ... --lexicon <tsv> ...
                                   --exclude <case tsv> ... --output resources/dictionary/word_zh.lm
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from collections import Counter
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from build_context_lm import HAN_RUN, excluded_texts  # noqa: E402

MAGIC = b"LPWM"
VERSION = 1
SCALE = 1000
MAX_WORD = 8
DISCOUNT = 0.5
FNV_OFFSET, FNV_PRIME, MASK64 = 0xCBF29CE484222325, 0x100000001B3, (1 << 64) - 1


def fnv(text: str, h: int = FNV_OFFSET) -> int:
    data = text.encode("utf-16-le")
    for index in range(0, len(data), 2):
        h = ((h ^ (data[index] | data[index + 1] << 8)) * FNV_PRIME) & MASK64
    return h


def word_key(word: str) -> int:
    return fnv(word) >> 32


def pair_key(first: str, second: str) -> int:
    return fnv(second, fnv("\x01", fnv(first))) >> 32


# Worker state: word -> (id, cost); set once per process.
_COSTS: dict[str, float] = {}
_IDS: dict[str, int] = {}
_UNKNOWN = 0.0


def _init(costs: dict[str, float], ids: dict[str, int], unknown: float) -> None:
    global _COSTS, _IDS, _UNKNOWN
    _COSTS, _IDS, _UNKNOWN = costs, ids, unknown


def segment(clause: str) -> list[str]:
    n = len(clause)
    best = [0.0] + [math.inf] * n
    back = [1] * (n + 1)
    costs = _COSTS
    for end in range(1, n + 1):
        for length in range(1, min(MAX_WORD, end) + 1):
            cost = costs.get(clause[end - length:end])
            if cost is None:
                if length > 1:
                    continue
                cost = _UNKNOWN
            total = best[end - length] + cost
            if total < best[end]:
                best[end], back[end] = total, length
    tokens = []
    end = n
    while end > 0:
        tokens.append(clause[end - back[end]:end])
        end -= back[end]
    tokens.reverse()
    return tokens


def _count(batch: list[tuple[str, int]]) -> tuple[Counter, np.ndarray, np.ndarray]:
    """Weighted word counts and (first id << 32 | second id) pair counts."""
    words: Counter = Counter()
    pairs: list[int] = []
    weights: list[int] = []
    ids = _IDS
    for clause, weight in batch:
        tokens = segment(clause)
        for token in tokens:
            words[token] += weight
        for first, second in zip(tokens, tokens[1:]):
            a, b = ids.get(first), ids.get(second)
            if a is not None and b is not None:
                pairs.append(a << 32 | b)
                weights.append(weight)
    keys = np.asarray(pairs, dtype=np.uint64)
    values = np.asarray(weights, dtype=np.int64)
    if len(keys):
        order = np.argsort(keys, kind="stable")
        keys, values = keys[order], values[order]
        unique, start = np.unique(keys, return_index=True)
        return words, unique, np.add.reduceat(values, start)
    return words, keys, values


def count_all(clauses, costs, ids, unknown, workers):
    batches = [clauses[index:index + 20000] for index in range(0, len(clauses), 20000)]
    words: Counter = Counter()
    keys, values = [], []
    with ProcessPoolExecutor(workers, initializer=_init, initargs=(costs, ids, unknown)) as pool:
        for batch_words, batch_keys, batch_values in pool.map(_count, batches):
            words.update(batch_words)
            keys.append(batch_keys)
            values.append(batch_values)
    keys = np.concatenate(keys)
    values = np.concatenate(values)
    order = np.argsort(keys, kind="stable")
    keys, values = keys[order], values[order]
    unique, start = np.unique(keys, return_index=True)
    return words, unique, np.add.reduceat(values, start)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, nargs="+", required=True, help="text files; NAME:WEIGHT repeats clauses")
    parser.add_argument("--lexicon", type=Path, nargs="+", required=True, help="lexicon TSV files (pinyin, word, frequency)")
    parser.add_argument("--exclude", type=Path, nargs="*", default=[], help="case TSV files to hold out")
    parser.add_argument("--min-pair-count", type=int, default=3)
    parser.add_argument("--min-score", type=float, default=0.5, help="smallest stored association, nats")
    parser.add_argument("--max-score", type=float, default=15.0, help="associations above this are clipped, nats")
    parser.add_argument("--workers", type=int, default=16)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    frequency: dict[str, int] = {}
    for path in args.lexicon:
        for line in path.read_text(encoding="utf-8").splitlines():
            parts = line.split("\t")
            if line.startswith("#") or len(parts) < 3 or not parts[2].strip().isdigit():
                continue
            word = parts[1]
            if len(word) <= MAX_WORD and HAN_RUN.fullmatch(word):
                frequency[word] = max(frequency.get(word, 0), int(parts[2]))
    ids = {word: index for index, word in enumerate(sorted(frequency))}
    words_by_id = sorted(frequency)

    excluded = excluded_texts(args.exclude)
    by_first: dict[str, list[str]] = {}
    for item in excluded:
        by_first.setdefault(item[0], []).append(item)
    clauses: list[tuple[str, int]] = []
    manifest, skipped = [], 0
    for spec in args.input:
        name, _, weight = str(spec).rpartition(":")
        path, weight_value = (Path(name), int(weight)) if name and weight.isdigit() else (spec, 1)
        count = 0
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.startswith("#"):
                continue
            if path.suffix == ".tsv":
                line = line.split("\t", 1)[-1]
            for clause in HAN_RUN.findall(line):
                if len(clause) < 2:
                    continue
                candidates = {item for ch in set(clause) for item in by_first.get(ch, ())}
                if any(item in clause for item in candidates):
                    skipped += 1
                    continue
                clauses.append((clause, weight_value))
                count += 1
        manifest.append({"file": path.name, "weight": weight_value, "clauses": count,
                         "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})

    # First segmentation: lexicon frequency bands as costs. Every token also
    # pays a constant, so a known longer word beats its pieces.
    costs = {word: 12.0 - min(value, 12000) / 1000.0 for word, value in frequency.items()}
    words, _, _ = count_all(clauses, costs, ids, 14.0, args.workers)
    # Re-estimate: unigram costs from the first segmentation.
    total = sum(words.values())
    costs = {word: -math.log((words.get(word, 0) + 0.1) / total) for word in frequency}
    unknown = -math.log(0.05 / total)
    words, keys, values = count_all(clauses, costs, ids, unknown, args.workers)
    total = sum(words.values())

    first_ids = (keys >> np.uint64(32)).astype(np.int64)
    second_ids = (keys & np.uint64(0xFFFFFFFF)).astype(np.int64)
    unigram = np.array([words.get(word, 0) for word in words_by_id], dtype=np.float64)
    keep = values >= args.min_pair_count
    first_ids, second_ids, values = first_ids[keep], second_ids[keep], values[keep].astype(np.float64)
    scores = np.log((values - DISCOUNT) * total / (unigram[first_ids] * unigram[second_ids]))
    keep = scores >= args.min_score
    first_ids, second_ids, scores = first_ids[keep], second_ids[keep], np.minimum(scores[keep], args.max_score)

    pair_scores: dict[int, int] = {}
    starts = set()
    for a, b, score in zip(first_ids.tolist(), second_ids.tolist(), scores.tolist()):
        key = pair_key(words_by_id[a], words_by_id[b])
        value = int(round(score * SCALE))
        if value > pair_scores.get(key, -1):
            pair_scores[key] = value
        starts.add(word_key(words_by_id[a]))
    pair_keys = np.array(sorted(pair_scores), dtype="<u4")
    pair_values = np.array([pair_scores[int(key)] for key in pair_keys], dtype="<i2")
    word_keys = np.array(sorted(starts), dtype="<u4")
    with args.output.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(struct.pack("<IIII", VERSION, SCALE, len(word_keys), len(pair_keys)))
        handle.write(word_keys.tobytes())
        handle.write(pair_keys.tobytes())
        handle.write(pair_values.tobytes())
    metadata = {
        "format": "LPWM", "format_version": VERSION, "score_scale": SCALE, "discount": DISCOUNT,
        "minimum_pair_count": args.min_pair_count, "minimum_score": args.min_score, "maximum_score": args.max_score,
        "words": int(len(word_keys)), "pairs": int(len(pair_keys)), "tokens": int(total),
        "excluded_clauses": skipped, "inputs": manifest,
        "lexicons": [{"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in args.lexicon],
        "excluded_case_files": [{"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in args.exclude],
        "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
    }
    args.output.with_name(args.output.name + ".metadata.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("words", "pairs", "tokens", "excluded_clauses")}))


if __name__ == "__main__":
    main()
