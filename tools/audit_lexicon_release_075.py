#!/usr/bin/env python3
"""Independently audit 0.7.5 authoring against immutable 0.7.4 Git blobs.

No dictionary, weight, builder, review, source registry, or native binary is
modified. This is a structural/provenance audit, not a substitute for the
contextual linguistic review recorded separately by the authors and QA.
"""
from __future__ import annotations

import argparse
import ast
from collections import Counter
from datetime import datetime, timezone
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import re
import subprocess

import pypinyin
from pypinyin import Style, lazy_pinyin, pinyin

ROOT = Path(__file__).resolve().parents[1]
BASE_COMMIT = "d62c22c94d25b40e7da7c966f7fcfd78e4530d68"
RESOURCE = "resources/dictionary/"
OUT = ROOT / RESOURCE / "expansion"
LAYERS = ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv")
RUNTIME = (*LAYERS, "context_zh.tsv", "continuations_zh.tsv")
HIGH_RISK = "卡调行重薄为着了血得的"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha(path: Path) -> str:
    return digest(path.read_bytes())


def git_blob(name: str) -> bytes:
    return subprocess.check_output(["git", "-c", f"safe.directory={ROOT.as_posix()}",
                                    "show", f"{BASE_COMMIT}:{name}"], cwd=ROOT)


def rows(data: bytes):
    category, result = "", []
    for number, line in enumerate(data.decode("utf-8-sig").splitlines(), 1):
        if line.startswith("# category:"):
            category = line.partition(":")[2].strip()
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        assert len(fields) == 3, (number, "field count")
        key, word, frequency = fields
        assert re.fullmatch("[a-z]+", key) and word and frequency.isdigit(), (number, fields)
        result.append((category, key, word, int(frequency)))
    return result


def pair(row):
    return row[1], row[2]


def constants(path: Path):
    result = {}
    for node in ast.parse(path.read_text(encoding="utf-8")).body:
        if isinstance(node, ast.Assign) and len(node.targets) == 1 and isinstance(node.targets[0], ast.Name):
            name, value = node.targets[0].id, node.value
        elif isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            name, value = node.target.id, node.value
        else:
            continue
        if name in ("WEIGHTS", "QUALITY_EXCLUSIONS", "READING_VARIANT_WORDS"):
            result[name] = ast.literal_eval(value)
    assert "WEIGHTS" in result and set(result["WEIGHTS"].values()) <= {5000, 5500, 6000}
    return result


@lru_cache(maxsize=None)
def readings(char: str):
    return tuple(pinyin(char, style=Style.NORMAL, heteronym=True, strict=True)[0])


def default_key(word: str):
    return "".join(lazy_pinyin(word, style=Style.NORMAL, strict=True))


def needs_review(word: str):
    return any(len(readings(char)) > 1 for char in word)


def compatible(word: str, key: str):
    offsets = {0}
    for char in word:
        offsets = {i + len(syllable) for i in offsets for syllable in readings(char)
                   if key.startswith(syllable, i)}
    return len(key) in offsets


def note_key_compatible(word: str, key: str, note: str):
    """Check the recorded character choices, without asserting their semantics."""
    choices = {}
    for char in set(word):
        if len(readings(char)) > 1:
            match = re.search(re.escape(char) + r"=([a-z]+)", note)
            assert match is not None, ("Context note omitted detected polyphone", word, char)
            assert match[1] in readings(char), ("Impossible noted character reading", word, char, match[1])
            choices[char] = (match[1],)
    offsets = {0}
    for char in word:
        offsets = {i + len(syllable) for i in offsets for syllable in choices.get(char, readings(char))
                   if key.startswith(syllable, i)}
    return len(key) in offsets


def review_rows(path: Path):
    result = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        assert len(fields) == 4, (path.name, number)
        key, word, reviewer, note = fields
        assert word not in result and re.fullmatch("[a-z]+", key) and reviewer.strip() and note.strip(), fields
        result[word] = (key, reviewer, note)
    return result


def hash_mapping(mapping: dict, directory: Path):
    for name, expected in mapping.items():
        assert sha(directory / name) == expected.lower(), ("Frozen file changed", name)
    # Draft order is frozen too: it decides which category owns duplicate words.
    return dict(mapping)


def audit_batch(number: int, baseline: dict, answers: set[str], fixtures: dict):
    stem = f"lexicon_batch_{number:03d}"
    batch, builder, apply = (OUT / f"{stem}.tsv", OUT / f"build_{stem}.py", OUT / f"apply_{stem}.py")
    meta = json.loads(batch.with_suffix(".json").read_text(encoding="utf-8"))
    policy = constants(builder)
    assert meta["batch"] == stem and sha(batch) == meta["sha256"]
    assert b"\r" not in batch.read_bytes() and not batch.read_bytes().startswith(b"\xef\xbb\xbf")
    drafts = hash_mapping(meta["authoring_files_sha256"], OUT)
    worklists = hash_mapping(meta["reviewer_worklists_sha256"], OUT)
    code = hash_mapping(meta["authoring_tools_sha256"], OUT)
    assert builder.name in code and apply.name in code, "Builder and apply code must be frozen"
    assert meta["dedup_baseline_sha256"] == {name: digest(baseline[name]) for name in LAYERS}
    assert meta["benchmark_sha256"] == fixtures, "All frozen typing files must be bound"
    review_path = OUT / meta["pronunciation_review_file"]
    assert sha(review_path) == meta["pronunciation_review_sha256"]
    reviews = review_rows(review_path)
    author_reviews = {}
    for name in worklists:
        for word, entry in review_rows(OUT / name).items():
            assert word not in author_reviews or author_reviews[word][0] == entry[0], ("Conflicting author reviews", word)
            author_reviews.setdefault(word, entry)
    known = [row for name in LAYERS for row in rows(baseline[name])]
    known_pairs, known_words = {pair(row) for row in known}, {row[2] for row in known}
    selected, seen, duplicates, existing, alternate, excluded, rejected = [], set(), [], [], [], [], []
    authored = 0
    for name in drafts:
        category = ""
        for line in (OUT / name).read_text(encoding="utf-8").splitlines():
            if line.startswith("# category:"):
                category = line.partition(":")[2].strip()
            elif line and not line.startswith("#"):
                assert category in policy["WEIGHTS"], (name, category)
                for word in line.split():
                    authored += 1
                    assert re.fullmatch(r"[\u4e00-\u9fff]{2,6}", word), word
                    if word in seen:
                        duplicates.append(word)
                        continue
                    seen.add(word)
                    if word in answers:
                        excluded.append(word)
                        continue
                    if word in policy.get("QUALITY_EXCLUSIONS", {}):
                        rejected.append({"word": word, "reason": policy["QUALITY_EXCLUSIONS"][word]})
                        continue
                    key = default_key(word)
                    if (key, word) in known_pairs:
                        existing.append(word)
                        continue
                    if word in known_words:
                        alternate.append({"word": word, "proposed_pinyin": key})
                        continue
                    selected.append((category, key, word, policy["WEIGHTS"][category]))
    required = {row[2] for row in selected if needs_review(row[2])}
    assert set(reviews) == required, ("Review membership", sorted(required - set(reviews)), sorted(set(reviews) - required))
    assert required <= set(author_reviews)
    assert all(author_reviews[word] == reviews[word] for word in required), "Final review must preserve the selected author decision"
    assert all(note_key_compatible(word, entry[0], entry[2]) for word, entry in reviews.items()), "Noted character choices differ from reviewed compound key"
    final = sorted((category, reviews[word][0] if word in reviews else key, word, weight)
                   for category, key, word, weight in selected)
    actual = rows(batch.read_bytes())
    assert actual == final, "Frozen batch differs from independently reconstructed draft/review selection"
    assert len(final) == len({pair(row) for row in final}) == len({row[2] for row in final}) == meta["new_entries"]
    assert all(row[2] not in known_words and pair(row) not in known_pairs and row[2] not in answers for row in final)
    assert all(compatible(row[2], row[1]) for row in final)
    assert authored == meta["authored_candidates"]
    assert Counter(duplicates) == Counter(meta["duplicate_author_candidates_excluded"])
    assert Counter(existing) == Counter(meta["existing_display_words_excluded"])
    assert Counter(excluded) == Counter(meta["benchmark_answers_excluded"])
    assert sorted(alternate, key=lambda item: item["word"]) == sorted(meta["alternate_pronunciation_candidates_excluded"], key=lambda item: item["word"])
    assert sorted(rejected, key=lambda item: item["word"]) == sorted(meta["quality_candidates_excluded"], key=lambda item: item["word"])
    overrides = {word: entry[0] for word, entry in reviews.items() if entry[0] != default_key(word)}
    variants = {word: key for word, key in overrides.items() if word in policy.get("READING_VARIANT_WORDS", set())}
    assert overrides == meta["manual_pronunciation_overrides"] and variants == meta["reading_variant_choices"]
    validation = meta["validation"]
    assert validation["polyphone_reviewed_display_words"] == len(required)
    assert validation["automatic_default_readings_changed"] == len(overrides)
    assert validation["semantic_pronunciation_corrections"] == len(overrides) - len(variants)
    assert validation["reading_variant_choices"] == len(variants)
    assert meta["category_counts"] == dict(Counter(row[0] for row in final))
    assert meta["frequency_distribution"] == dict(Counter(str(row[3]) for row in final))
    assert meta["frequency_range"] == sorted({row[3] for row in final})
    assert meta["length_distribution"] == dict(Counter(str(len(row[2])) for row in final))
    applied_path = batch.with_suffix(".applied.json")
    merged = (baseline["local_core_zh_pinyin.tsv"].decode("utf-8").rstrip() + f"\n\n# category: {stem}\n" +
              "".join(f"{key}\t{word}\t{weight}\n" for _, key, word, weight in final)).encode("utf-8")
    if applied_path.exists():
        applied = json.loads(applied_path.read_text(encoding="utf-8"))
        assert applied["frozen_batch_sha256"] == sha(batch)
        assert applied["canonical_before_sha256"] == digest(baseline["local_core_zh_pinyin.tsv"])
        assert applied["canonical_after_sha256"] == digest(merged)
        assert applied["canonical_before_entries"] == len(rows(baseline["local_core_zh_pinyin.tsv"]))
        assert applied["canonical_after_entries"] == len(rows(merged))
        assert applied["frozen_case_count"] == 267 and applied["merged_entries"] == len(final)
        baseline["local_core_zh_pinyin.tsv"] = merged
    return {
        "batch": stem, "applied": applied_path.exists(), "entries": len(final), "sha256": sha(batch),
        "authoring_tools_sha256": code, "authoring_files_sha256": drafts, "reviewer_worklists_sha256": worklists,
        "pronunciation_review_sha256": sha(review_path), "three_layer_display_collisions": 0,
        "full_typing_answer_matches": 0, "independent_draft_selection_matches_frozen_batch": True,
        "preset_category_weights": policy["WEIGHTS"], "polyphone_review_required": len(required),
        "polyphone_review_membership_exact": True, "per_character_readings_compatible": True,
        "context_notes_cover_every_detected_polyphone": True, "note_readings_match_frozen_key": True,
        "automatic_default_disagreements": len(overrides), "semantic_pronunciation_corrections": len(overrides) - len(variants),
        "reading_variant_choices": variants,
        "high_risk_display_counts": {char: sum(char in row[2] for row in final) for char in HIGH_RISK},
        "linguistic_scope": "Complete contextual author review membership and key structure verified; notes are evidence of recorded decisions, not independent proof of naturalness or preferred pronunciation.",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--batches", nargs="+", type=int, help="Defaults to all frozen 018+ batches")
    parser.add_argument("--linguistic-review", type=Path, help="Independent manual QA record bound to frozen batch/review hashes")
    args = parser.parse_args()
    assert pypinyin.__version__ == "0.55.0", "Use the pinned authoring-only pronunciation dependency"
    baseline = {name: git_blob(RESOURCE + name) for name in RUNTIME}
    original = dict(baseline)
    fixtures, answers, case_count = {}, set(), 0
    for path in sorted((ROOT / "tests/data").glob("typing_*.tsv")):
        data = path.read_bytes()
        assert data == git_blob(path.relative_to(ROOT).as_posix()), ("Typing fixture changed from 0.7.4", path.name)
        fixtures[path.name] = digest(data)
        for line in data.decode("utf-8-sig").splitlines():
            if line and not line.startswith("#"):
                fields = line.split("\t")
                assert len(fields) >= 3
                answers.add(fields[1])
                case_count += 1
    assert case_count == len(answers) == 267
    numbers = args.batches or sorted(int(path.stem.rsplit("_", 1)[1]) for path in OUT.glob("lexicon_batch_*.json")
                                     if re.fullmatch(r"lexicon_batch_\d{3}", path.stem) and int(path.stem[-3:]) >= 18)
    batches = [audit_batch(number, baseline, answers, fixtures) for number in numbers]
    current = {name: (ROOT / RESOURCE / name).read_bytes() for name in RUNTIME}
    assert current == baseline, "Runtime bytes differ from exact baseline plus applied frozen batches"
    old_rows, current_rows = rows(original["local_core_zh_pinyin.tsv"]), rows(current["local_core_zh_pinyin.tsv"])
    old_map, new_map = {pair(row): row[3] for row in old_rows}, {pair(row): row[3] for row in current_rows}
    assert len(old_rows) == 95324 and all(new_map.get(key) == weight for key, weight in old_map.items())
    baseline_registry = json.loads(git_blob(RESOURCE + "sources/lexicon_sources.json"))
    registry = json.loads((ROOT / RESOURCE / "sources/lexicon_sources.json").read_text(encoding="utf-8"))
    original_sources = {item["source_id"]: item for item in baseline_registry["sources"]}
    current_sources = {item["source_id"]: item for item in registry["sources"]}
    assert all(current_sources.get(key) == value for key, value in original_sources.items()), "Prior source terms changed"
    metadata = json.loads((ROOT / RESOURCE / "local_core_zh_pinyin.metadata.json").read_text(encoding="utf-8"))
    old_metadata = json.loads(git_blob(RESOURCE + "local_core_zh_pinyin.metadata.json"))
    assert metadata["source_policy"] == old_metadata["source_policy"], "Reviewed reference-derived policy changed"
    assert set(old_metadata["source_ids"]) <= set(metadata["source_ids"])
    assert set(metadata["source_ids"]) - set(old_metadata["source_ids"]) <= set(current_sources)
    inherited_unregistered_ids = sorted(set(old_metadata["source_ids"]) - set(original_sources))
    assert sorted(set(metadata["source_ids"]) - set(current_sources)) == inherited_unregistered_ids
    assert metadata["entry_count"] == len(current_rows) and metadata["sha256"].lower() == digest(current["local_core_zh_pinyin.tsv"])
    assert metadata["category_counts"] == dict(Counter(row[0] for row in current_rows))
    lengths = Counter({"single": 0, "two": 0, "three": 0, "fourplus": 0})
    for row in current_rows:
        lengths[{1: "single", 2: "two", 3: "three"}.get(len(row[2]), "fourplus")] += 1
    assert metadata["length_distribution"] == dict(lengths)
    report = {
        "format_version": 1, "version": "0.7.5", "audited_at_utc": datetime.now(timezone.utc).isoformat(),
        "status": "passed" if batches else "waiting_for_frozen_new_batch", "baseline_version": "0.7.4", "baseline_commit": BASE_COMMIT,
        "method": "Independent immutable Git-blob baseline, AST-read preset policy, draft/review reconstruction, and exact canonical merge bytes; no native compilation, dictionary writes or public-answer weight adjustments.",
        "audit_tool_sha256": sha(Path(__file__)), "pronunciation_dependency": "pypinyin 0.55.0, authoring only",
        "baseline_runtime_sha256": {name: digest(data) for name, data in original.items()},
        "runtime_sha256": {name: digest(data) for name, data in current.items()},
        "typing_fixture_sha256": fixtures, "complete_typing_answers_excluded": case_count,
        "baseline_local_core_entries": len(old_rows), "current_local_core_entries": len(current_rows),
        "baseline_three_layer_unique_pairs": len({pair(row) for name in LAYERS for row in rows(original[name])}),
        "current_three_layer_unique_pairs": len({pair(row) for name in LAYERS for row in rows(current[name])}),
        "baseline_rows_and_weights_preserved": True, "runtime_exact_frozen_merge_verified": True,
        "unchanged_other_runtime_layers": [name for name in RUNTIME if name != "local_core_zh_pinyin.tsv" and original[name] == current[name]],
        "all_prior_source_records_preserved_exactly": True, "reviewed_source_policy_preserved": True,
        "inherited_unregistered_metadata_source_ids": inherited_unregistered_ids,
        "current_source_registry_sha256": sha(ROOT / RESOURCE / "sources/lexicon_sources.json"),
        "reference_derived_batches": [f"lexicon_batch_{n:03d}" for n in range(12, 16)],
        "provenance_scope": "New individually authored batches are project first-party material. Existing 012–015 remain reference-derived name/title indexes with their original recorded limitations; the entire local dictionary is not described as first-party MIT material.",
        "batches": batches,
        "independent_linguistic_review": {"status": "pending", "scope": "Structural checks do not establish semantic pronunciation, preferred variant choice, naturalness, or general typing accuracy. Manual QA decisions will be recorded after batch freeze."},
    }
    if args.linguistic_review:
        manual = json.loads(args.linguistic_review.read_text(encoding="utf-8"))
        assert manual["reviewer"] and manual["status"] == "passed"
        assert set(manual["batches"]) == {item["batch"] for item in batches}
        for item in batches:
            decision = manual["batches"][item["batch"]]
            assert decision["frozen_batch_sha256"] == item["sha256"]
            assert decision["pronunciation_review_sha256"] == item["pronunciation_review_sha256"]
            assert decision["display_words_inspected"] == item["entries"]
            assert decision["high_risk_display_counts"] == item["high_risk_display_counts"]
            assert decision["automatic_default_disagreements_inspected"] == item["automatic_default_disagreements"]
            assert decision["findings"] and decision["unresolved_findings"] == []
        report["independent_linguistic_review"] = {
            "status": "passed", "reviewer": manual["reviewer"], "manual_record_sha256": sha(args.linguistic_review),
            "batches": manual["batches"],
            "scope": "Independent visual reading of every new display word, all default disagreements, and common polyphones in context; this is editorial QA, not a blind general-language accuracy result.",
        }
    target = ROOT / "docs/lexicon-075-authoring-audit.json"
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    lines = ["# 0.7.5 vocabulary authoring audit", "", f"Status: **{report['status']}**. Immutable 0.7.4 baseline: `{BASE_COMMIT}`.", "",
             f"Baseline local rows: {len(old_rows):,}; current: {len(current_rows):,}. All 267 complete typing answers are excluded from new batches.", "",
             "The audit independently reconstructs each batch from frozen drafts, preset category weights, and contextual pronunciation decisions. It checks every polyphone review membership, display/key deduplication across all three runtime layers, authoring-code/draft/review SHA, and exact canonical merge bytes. Existing rows and weights are unchanged.", "",
             "Prior source records and reviewed source policy are preserved exactly. Batches 012–015 retain reference-derived provenance and its recorded limitations; this audit does not relabel the entire local dictionary as first-party MIT material.", "",
             "| Batch | Added entries | Polyphone reviews | Semantic default corrections | Reading variant choices |", "| --- | ---: | ---: | ---: | ---: |"]
    lines.extend(f"| {item['batch']} | {item['entries']} | {item['polyphone_review_required']} | {item['semantic_pronunciation_corrections']} | {len(item['reading_variant_choices'])} |" for item in batches)
    linguistic_status = report["independent_linguistic_review"]["status"]
    lines.extend(["", f"Per-character compatibility and recorded review membership are structural evidence. Independent manual naturalness/pronunciation QA: **{linguistic_status}**. Native coverage and public sentence replay are reported separately by the release coordinator.", "", "Machine-readable evidence and all bound SHA values: [lexicon-075-authoring-audit.json](lexicon-075-authoring-audit.json).", ""])
    target.with_suffix(".md").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(json.dumps({"status": report["status"], "batches": len(batches), "local_entries": len(current_rows), "three_layer_unique_pairs": report["current_three_layer_unique_pairs"]}, ensure_ascii=False))


if __name__ == "__main__":
    main()
