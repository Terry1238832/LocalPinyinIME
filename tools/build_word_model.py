"""Build the word association model used beside the character language model.

The character 4-gram model sees at most three characters of history, so it
cannot tell 这家店是专门卖 from 这家电视专门买 once the deciding word is
further back. This model scores pairs of adjacent lexicon words instead.

Training text is segmented with the bundled lexicon (unigram dynamic
programming, re-estimated once from the first segmentation). For every
adjacent pair seen often enough, the model stores how much more often the pair
occurs than its words would by chance:

    score(a, b) = ln((count(a b) - D) * N / (count(a) * count(b)))

A run of three words also keeps how much more often its third word follows
the first two than it follows the second alone:

    score(a, b, c) = ln((count(a b c) - D) * count(b) / (count(a b) * count(b c)))

Only positive associations are kept, so an unseen pair scores zero and a
stored pair is a bonus. Clauses containing an evaluation sentence are skipped
exactly as for the character model. Only pair and triple statistics are
packaged.

Binary layout (little endian):
  magic "LPWM", u32 version (2), u32 scale, u32 word_count, u32 pair_count,
  u32 triple_count,
  u32 word_keys[word_count] sorted ascending (words that begin a stored pair),
  u32 pair_keys[pair_count] sorted ascending, u32 triple_keys[triple_count]
  sorted ascending, i16 pair_scores[pair_count], i16 triple_scores[triple_count].
Keys are the high 32 bits of 64-bit FNV-1a over UTF-16 code units; a pair
hashes its first word, U+0001, then its second word, and a triple its three
words with U+0001 between them.

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
VERSION = 2
IDBITS = 21
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


def triple_key(first: str, second: str, third: str) -> int:
    return fnv(third, fnv("\x01", fnv(second, fnv("\x01", fnv(first))))) >> 32


def _aggregate(keys: list[int] | np.ndarray, weights: list[int] | np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    keys = np.asarray(keys, dtype=np.uint64)
    values = np.asarray(weights, dtype=np.int64)
    if not len(keys):
        return keys, values
    order = np.argsort(keys, kind="stable")
    keys, values = keys[order], values[order]
    unique, start = np.unique(keys, return_index=True)
    return unique, np.add.reduceat(values, start)


# Worker state: word -> (id, cost); set once per process.
_COSTS: dict[str, float] = {}
_IDS: dict[str, int] = {}
_UNKNOWN = 0.0
_TRIPLES = False


def _init(costs: dict[str, float], ids: dict[str, int], unknown: float, triples: bool) -> None:
    global _COSTS, _IDS, _UNKNOWN, _TRIPLES
    _COSTS, _IDS, _UNKNOWN, _TRIPLES = costs, ids, unknown, triples


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


def _count(batch: list[tuple[str, int]]):
    """Weighted word counts, pair counts keyed (a << 32 | b) and, when
    enabled, triple counts keyed by three IDBITS-bit word ids."""
    words: Counter = Counter()
    pairs: list[int] = []
    pair_weights: list[int] = []
    triples: list[int] = []
    triple_weights: list[int] = []
    ids = _IDS
    for clause, weight in batch:
        tokens = segment(clause)
        for token in tokens:
            words[token] += weight
        token_ids = [ids.get(token) for token in tokens]
        for index in range(len(token_ids) - 1):
            a, b = token_ids[index], token_ids[index + 1]
            if a is None or b is None:
                continue
            pairs.append(a << 32 | b)
            pair_weights.append(weight)
            if _TRIPLES and index + 2 < len(token_ids) and token_ids[index + 2] is not None:
                triples.append((a << IDBITS | b) << IDBITS | token_ids[index + 2])
                triple_weights.append(weight)
    return words, _aggregate(pairs, pair_weights), _aggregate(triples, triple_weights)


def count_all(clauses, costs, ids, unknown, workers, triples=False):
    batches = [clauses[index:index + 20000] for index in range(0, len(clauses), 20000)]
    words: Counter = Counter()
    pair_parts, triple_parts = [], []
    with ProcessPoolExecutor(workers, initializer=_init, initargs=(costs, ids, unknown, triples)) as pool:
        for batch_words, batch_pairs, batch_triples in pool.map(_count, batches):
            words.update(batch_words)
            pair_parts.append(batch_pairs)
            # Merge as they arrive, so memory holds one copy of the triples.
            triple_parts.append(batch_triples)
            if len(triple_parts) >= 16:
                triple_parts = [_aggregate(np.concatenate([k for k, _ in triple_parts]),
                                           np.concatenate([v for _, v in triple_parts]))]
    pairs = _aggregate(np.concatenate([k for k, _ in pair_parts]), np.concatenate([v for _, v in pair_parts]))
    triples_out = _aggregate(np.concatenate([k for k, _ in triple_parts]), np.concatenate([v for _, v in triple_parts]))
    return words, pairs, triples_out


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, nargs="+", required=True, help="text files; NAME:WEIGHT repeats clauses")
    parser.add_argument("--lexicon", type=Path, nargs="+", required=True, help="lexicon TSV files (pinyin, word, frequency)")
    parser.add_argument("--exclude", type=Path, nargs="*", default=[], help="case TSV files to hold out")
    parser.add_argument("--min-pair-count", type=int, default=3)
    parser.add_argument("--min-score", type=float, default=0.5, help="smallest stored association, nats")
    parser.add_argument("--max-score", type=float, default=15.0, help="associations above this are clipped, nats")
    parser.add_argument("--min-triple-count", type=int, default=3, help="0 stores no triples")
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
    assert len(words_by_id) < 1 << IDBITS

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
    words, (keys, values), (triple_keys_raw, triple_values) = count_all(
        clauses, costs, ids, unknown, args.workers, args.min_triple_count > 0)
    total = sum(words.values())
    all_pair_keys, all_pair_values = keys, values

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

    triple_scores: dict[int, int] = {}
    if args.min_triple_count > 0 and len(triple_keys_raw):
        keep = triple_values >= args.min_triple_count
        packed, counts = triple_keys_raw[keep], triple_values[keep].astype(np.float64)
        mask = np.uint64((1 << IDBITS) - 1)
        a_ids = (packed >> np.uint64(2 * IDBITS)).astype(np.int64)
        b_ids = ((packed >> np.uint64(IDBITS)) & mask).astype(np.int64)
        c_ids = (packed & mask).astype(np.int64)

        def pair_count(first, second):
            wanted = (first.astype(np.uint64) << np.uint64(32)) | second.astype(np.uint64)
            position = np.clip(np.searchsorted(all_pair_keys, wanted), 0, len(all_pair_keys) - 1)
            return np.where(all_pair_keys[position] == wanted, all_pair_values[position], 0).astype(np.float64)

        ab, bc = pair_count(a_ids, b_ids), pair_count(b_ids, c_ids)
        with np.errstate(divide="ignore", invalid="ignore"):
            scores = np.log((counts - DISCOUNT) * unigram[b_ids] / (ab * bc))
        keep = np.isfinite(scores) & (scores >= args.min_score)
        for a, b, c, score in zip(a_ids[keep].tolist(), b_ids[keep].tolist(), c_ids[keep].tolist(),
                                  np.minimum(scores[keep], args.max_score).tolist()):
            key = triple_key(words_by_id[a], words_by_id[b], words_by_id[c])
            value = int(round(score * SCALE))
            if value > triple_scores.get(key, -1):
                triple_scores[key] = value
            starts.add(word_key(words_by_id[a]))
    triple_keys = np.array(sorted(triple_scores), dtype="<u4")
    triple_out = np.array([triple_scores[int(key)] for key in triple_keys], dtype="<i2")
    word_keys = np.array(sorted(starts), dtype="<u4")
    with args.output.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(struct.pack("<IIIII", VERSION, SCALE, len(word_keys), len(pair_keys), len(triple_keys)))
        handle.write(word_keys.tobytes())
        handle.write(pair_keys.tobytes())
        handle.write(triple_keys.tobytes())
        handle.write(pair_values.tobytes())
        handle.write(triple_out.tobytes())
    metadata = {
        "format": "LPWM", "format_version": VERSION, "score_scale": SCALE, "discount": DISCOUNT,
        "minimum_pair_count": args.min_pair_count, "minimum_triple_count": args.min_triple_count,
        "minimum_score": args.min_score, "maximum_score": args.max_score,
        "words": int(len(word_keys)), "pairs": int(len(pair_keys)), "triples": int(len(triple_keys)), "tokens": int(total),
        "excluded_clauses": skipped, "inputs": manifest,
        "lexicons": [{"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in args.lexicon],
        "excluded_case_files": [{"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in args.exclude],
        "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
    }
    args.output.with_name(args.output.name + ".metadata.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("words", "pairs", "triples", "tokens", "excluded_clauses")}))


if __name__ == "__main__":
    main()
