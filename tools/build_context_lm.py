"""Build the binary character 4-gram language model used for contextual ranking.

The model is an interpolated modified Kneser-Ney character model stored in
backoff form. Only n-gram statistics are packaged; no sentence is stored.

Inputs are plain UTF-8 text files. Lines are split into clauses of Han
characters; every clause is wrapped in sentence boundary markers. Clauses that
contain an evaluation sentence are skipped so published measurements remain
held out. Requires numpy at authoring time only; the runtime has no dependency.

Binary layout (little endian):
  magic "LPLM", u32 version, u32 max_order, u32 scale,
  i32 unknown_logprob, u32 count[4], 4 zero bytes, then for every order n = 1..4:
  u32 keys[count[n]] sorted ascending, i16 logprob[count[n]],
  i16 backoff[count[n]] (zero for the top order).
An n-gram packs its 16-bit code units oldest first. Unigram and bigram keys
are exact; trigram and 4gram keys are the high 32 bits of the packed value
multiplied by 0x9E3779B97F4A7C15 (a fingerprint; colliding entries keep the
likelier n-gram, and a false match for an unseen n-gram is about 1 in 3000).
Log probabilities are natural logarithms multiplied by ``scale``.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

import numpy as np

HAN_RUN = re.compile(r"[㐀-鿿]+")
SEP, BOS, EOS = 1, 2, 3
ORDER = 4
SCALE = 1000
MAGIC = b"LPLM"
VERSION = 2
FINGERPRINT = np.uint64(0x9E3779B97F4A7C15)


def read_clauses(path: Path, excluded: list[str]) -> list[str]:
    text = path.read_text(encoding="utf-8")
    if path.suffix == ".tsv":
        text = "\n".join(line.split("\t", 1)[-1] for line in text.splitlines())
    clauses = []
    for line in text.splitlines():
        if line.startswith("#"):
            continue
        for clause in HAN_RUN.findall(line):
            if len(clause) >= 2 and not any(item in clause for item in excluded if len(item) <= len(clause)):
                clauses.append(clause)
    return clauses


def token_array(clauses: list[tuple[str, int]], fragments: list[tuple[str, int]]) -> np.ndarray:
    """Clauses are wrapped in boundary markers. Fragments (lexicon words) only
    contribute their internal n-grams, so they never claim a clause boundary."""
    pieces = []
    for clause, weight in clauses:
        codes = [BOS, *map(ord, clause), EOS]
        pieces.extend(codes * weight)
    for fragment, weight in fragments:
        codes = [SEP, *map(ord, fragment)]
        pieces.extend(codes * weight)
    pieces.append(SEP)
    return np.asarray(pieces, dtype=np.uint64)


def ngram_counts(tokens: np.ndarray, order: int) -> tuple[np.ndarray, np.ndarray]:
    length = len(tokens) - order + 1
    keys = np.zeros(length, dtype=np.uint64)
    valid = np.ones(length, dtype=bool)
    for offset in range(order):
        part = tokens[offset:offset + length]
        keys = (keys << np.uint64(16)) | part
        valid &= part != SEP
        if offset < order - 1:
            valid &= part != EOS  # no n-gram crosses a clause boundary
        if offset > 0:
            valid &= part != BOS
    keys = keys[valid]
    unique, counts = np.unique(keys, return_counts=True)
    return unique, counts.astype(np.int64)


def discounts(counts: np.ndarray) -> np.ndarray:
    n = [int(np.sum(counts == value)) for value in (1, 2, 3, 4)]
    if min(n) == 0:
        return np.array([0.0, 0.5, 1.0, 1.5])
    y = n[0] / (n[0] + 2 * n[1])
    values = [1 - 2 * y * n[1] / n[0], 2 - 3 * y * n[2] / n[1], 3 - 4 * y * n[3] / n[2]]
    # Weighted (repeated) text distorts count-of-counts; keep each discount in
    # its valid range so every probability stays positive.
    return np.array([0.0] + [float(np.clip(value, 0.1, limit)) for value, limit in zip(values, (0.95, 1.9, 2.85))])


def build(token_stream: np.ndarray, minimum_counts: dict[int, int]):
    raw = {order: ngram_counts(token_stream, order) for order in range(1, ORDER + 1)}
    mask16 = np.uint64(0xFFFF)
    # Kneser-Ney continuation counts for lower orders: the number of distinct
    # left contexts. An n-gram starting with BOS has no left context, so it
    # keeps its raw count.
    counts: dict[int, tuple[np.ndarray, np.ndarray]] = {ORDER: raw[ORDER]}
    for order in range(ORDER - 1, 0, -1):
        higher_keys = raw[order + 1][0]
        suffix = higher_keys & np.uint64((1 << (16 * order)) - 1)
        suffix_keys, continuation = np.unique(suffix, return_counts=True)
        keys, raw_counts = raw[order]
        position = np.searchsorted(suffix_keys, keys)
        position = np.clip(position, 0, len(suffix_keys) - 1)
        found = suffix_keys[position] == keys
        modified = np.where(found, continuation[position], 0).astype(np.int64)
        first = keys >> np.uint64(16 * (order - 1))
        modified = np.where(first == BOS, raw_counts, np.maximum(modified, 1))
        counts[order] = (keys, modified)

    probabilities: dict[int, dict[int, float]] = {}
    logprob: dict[int, np.ndarray] = {}
    keys_by_order: dict[int, np.ndarray] = {}
    gamma_by_order: dict[int, tuple[np.ndarray, np.ndarray]] = {}
    # Unigrams: continuation distribution interpolated with a uniform floor.
    # BOS is never predicted; it is kept only as a history with a backoff.
    all_keys, all_values = counts[1]
    is_bos = all_keys == BOS
    keys, values = all_keys[~is_bos], all_values[~is_bos]
    vocabulary = len(keys) + 1
    d = discounts(values)
    adjusted = np.maximum(values - d[np.minimum(values, 3)], 0)
    total = values.sum()
    leftover = (d[1] * np.sum(values == 1) + d[2] * np.sum(values == 2) + d[3] * np.sum(values >= 3)) / total
    p1 = adjusted / total + leftover / vocabulary
    keys_by_order[1] = all_keys
    logprob[1] = np.full(len(all_keys), -30.0)
    logprob[1][~is_bos] = np.log(p1)
    unknown = float(np.log(leftover / vocabulary))
    for order in range(2, ORDER + 1):
        keys, values = counts[order]
        d = discounts(values)
        history = keys >> np.uint64(16)
        histories, inverse = np.unique(history, return_inverse=True)
        history_total = np.bincount(inverse, weights=values)
        bucket = np.minimum(values, 3)
        n_bucket = [np.bincount(inverse, weights=(bucket == b).astype(float), minlength=len(histories)) for b in (1, 2, 3)]
        gamma = (d[1] * n_bucket[0] + d[2] * n_bucket[1] + d[3] * n_bucket[2]) / history_total
        lower_keys = keys & np.uint64((1 << (16 * (order - 1))) - 1)
        lower_logprob = lookup(order - 1, lower_keys, keys_by_order, logprob, gamma_by_order, unknown)
        p = np.maximum(values - d[bucket], 0) / history_total[inverse] + gamma[inverse] * np.exp(lower_logprob)
        keys_by_order[order] = keys
        logprob[order] = np.log(p)
        gamma_by_order[order - 1] = (histories, gamma)

    # Count cutoffs for the packaged model. Pruned probabilities fall back to
    # the lower order; histories are renormalized so each row still sums to 1.
    kept: dict[int, np.ndarray] = {}
    for order in range(1, ORDER + 1):
        raw_keys, raw_values = raw[order]
        position = np.searchsorted(raw_keys, keys_by_order[order])
        keep = raw_values[position] >= minimum_counts.get(order, 1)
        kept[order] = keep
    backoff: dict[int, np.ndarray] = {}
    for order in range(1, ORDER):
        histories = keys_by_order[order][kept[order]]
        child_keys = keys_by_order[order + 1][kept[order + 1]]
        child_logprob = logprob[order + 1][kept[order + 1]]
        child_history = child_keys >> np.uint64(16)
        child_lower = child_keys & np.uint64((1 << (16 * order)) - 1)
        lower = lookup_pruned(order, child_lower, keys_by_order, logprob, kept, backoff, unknown)
        groups, inverse = np.unique(child_history, return_inverse=True)
        seen_high = np.bincount(inverse, weights=np.exp(child_logprob))
        seen_low = np.bincount(inverse, weights=np.exp(lower))
        weight = np.zeros(len(histories))
        position = np.searchsorted(histories, groups)
        valid = (position < len(histories))
        valid[valid] = histories[position[valid]] == groups[valid]
        numerator = np.clip(1 - seen_high, 1e-9, None)
        denominator = np.clip(1 - seen_low, 1e-9, None)
        weight[position[valid]] = np.log(numerator[valid] / denominator[valid])
        backoff[order] = weight
    result = []
    for order in range(1, ORDER + 1):
        keys = keys_by_order[order][kept[order]]
        lp = np.clip(np.round(logprob[order][kept[order]] * SCALE), -32000, 0).astype(np.int16)
        bo = np.clip(np.round(backoff.get(order, np.zeros(len(keys))) * SCALE), -32000, 32000).astype(np.int16)
        result.append((keys, lp, bo))
    return result, int(round(unknown * SCALE))


def lookup(order, keys, keys_by_order, logprob, gamma_by_order, unknown):
    """Interpolated probability of every key at ``order`` (unpruned model)."""
    table = keys_by_order[order]
    position = np.clip(np.searchsorted(table, keys), 0, len(table) - 1)
    found = table[position] == keys
    if order == 1:
        return np.where(found, logprob[1][position], unknown)
    result = np.where(found, logprob[order][position], 0.0)
    missing = ~found
    if missing.any():
        lower = keys[missing] & np.uint64((1 << (16 * (order - 1))) - 1)
        history = keys[missing] >> np.uint64(16)
        histories, gamma = gamma_by_order[order - 1]
        hp = np.clip(np.searchsorted(histories, history), 0, len(histories) - 1)
        g = np.where(histories[hp] == history, gamma[hp], 1.0)
        result[missing] = np.log(g) + lookup(order - 1, lower, keys_by_order, logprob, gamma_by_order, unknown)
    return result


def lookup_pruned(order, keys, keys_by_order, logprob, kept, backoff, unknown):
    """Backoff probability of every key at ``order`` in the pruned model."""
    table = keys_by_order[order][kept[order]]
    values = logprob[order][kept[order]]
    if len(table) == 0:
        return np.full(len(keys), unknown)
    position = np.clip(np.searchsorted(table, keys), 0, len(table) - 1)
    found = table[position] == keys
    if order == 1:
        return np.where(found, values[position], unknown)
    result = np.where(found, values[position], 0.0)
    missing = ~found
    if missing.any():
        history = keys[missing] >> np.uint64(16)
        histories = keys_by_order[order - 1][kept[order - 1]]
        hp = np.clip(np.searchsorted(histories, history), 0, len(histories) - 1)
        weights = backoff.get(order - 1)
        bow = np.where(histories[hp] == history, weights[hp] if weights is not None else 0.0, 0.0)
        lower = keys[missing] & np.uint64((1 << (16 * (order - 1))) - 1)
        result[missing] = bow + lookup_pruned(order - 1, lower, keys_by_order, logprob, kept, backoff, unknown)
    return result


def compact_keys(order: int, keys: np.ndarray, lp: np.ndarray, bo: np.ndarray):
    if order <= 2:
        return keys.astype(np.uint32), lp, bo
    with np.errstate(over="ignore"):
        hashed = ((keys * FINGERPRINT) >> np.uint64(32)).astype(np.uint32)
    order_index = np.lexsort((-lp.astype(np.int32), hashed))
    hashed, lp, bo = hashed[order_index], lp[order_index], bo[order_index]
    first = np.ones(len(hashed), dtype=bool)
    first[1:] = hashed[1:] != hashed[:-1]
    return hashed[first], lp[first], bo[first]


def write_model(path: Path, tables, unknown: int) -> None:
    tables = [compact_keys(order, *table) for order, table in enumerate(tables, 1)]
    with path.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(struct.pack("<IIIi", VERSION, ORDER, SCALE, unknown))
        handle.write(struct.pack("<4I", *(len(keys) for keys, _, _ in tables)))
        handle.write(b"\0" * 4)  # 8-byte alignment for the key arrays
        for keys, lp, bo in tables:
            handle.write(keys.astype("<u4").tobytes())
            handle.write(lp.astype("<i2").tobytes())
            handle.write(bo.astype("<i2").tobytes())


def excluded_texts(paths: list[Path]) -> list[str]:
    result = set()
    for path in paths:
        for line in path.read_text(encoding="utf-8").splitlines():
            if not line or line.startswith("#"):
                continue
            parts = line.split("\t")
            text = parts[1] if len(parts) >= 2 else parts[0]
            for clause in HAN_RUN.findall(text):
                if len(clause) >= 4:
                    result.add(clause)
    return sorted(result, key=len)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, nargs="+", required=True,
                        help="text files; a NAME:WEIGHT suffix repeats clauses WEIGHT times")
    parser.add_argument("--fragments", type=Path, nargs="*", default=[],
                        help="lexicon TSV files (pinyin, word, frequency); NAME:WEIGHT repeats each word")
    parser.add_argument("--exclude", type=Path, nargs="*", default=[],
                        help="case TSV files (column 2) or plain line lists to hold out")
    parser.add_argument("--min-count", type=int, nargs=4, default=[1, 1, 2, 2], metavar=("N1", "N2", "N3", "N4"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    excluded = excluded_texts(args.exclude)
    # Index exclusions by their first character to keep filtering fast.
    by_first: dict[str, list[str]] = {}
    for item in excluded:
        by_first.setdefault(item[0], []).append(item)
    weighted = []
    manifest = []
    skipped = 0
    for spec in args.input:
        name, _, weight = str(spec).rpartition(":")
        path, weight_value = (Path(name), int(weight)) if name and weight.isdigit() else (spec, 1)
        clauses = read_clauses(path, [])
        for clause in clauses:
            candidates = {item for ch in set(clause) for item in by_first.get(ch, ())}
            if any(item in clause for item in candidates):
                skipped += 1
                continue
            weighted.append((clause, weight_value))
        manifest.append({"file": path.name, "weight": weight_value, "clauses": len(clauses),
                         "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    fragments = []
    for spec in args.fragments:
        name, _, weight = str(spec).rpartition(":")
        path, weight_value = (Path(name), int(weight)) if name and weight.isdigit() else (spec, 1)
        words = 0
        for line in path.read_text(encoding="utf-8").splitlines():
            parts = line.split("	")
            if line.startswith("#") or len(parts) < 2:
                continue
            word = parts[1]
            if 2 <= len(word) <= 12 and HAN_RUN.fullmatch(word):
                fragments.append((word, weight_value))
                words += 1
        manifest.append({"file": path.name, "weight": weight_value, "fragments": words,
                         "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    tokens = token_array(weighted, fragments)
    tables, unknown = build(tokens, dict(enumerate(args.min_count, 1)))
    write_model(args.output, tables, unknown)
    metadata = {
        "format": "LPLM", "format_version": VERSION, "maximum_order": ORDER, "smoothing": "interpolated modified Kneser-Ney",
        "logprob_scale": SCALE, "minimum_counts": args.min_count,
        "entries": {str(order + 1): int(len(keys)) for order, (keys, _, _) in enumerate(tables)},
        "han_characters": int(sum(len(clause) * weight for clause, weight in weighted)),
        "excluded_clauses": skipped, "inputs": manifest,
        "excluded_case_files": [{"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for path in args.exclude],
        "sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
    }
    args.output.with_name(args.output.name + ".metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n",
                                                        encoding="utf-8", newline="\n")
    print(json.dumps({key: metadata[key] for key in ("entries", "han_characters", "excluded_clauses")}))


if __name__ == "__main__":
    main()
