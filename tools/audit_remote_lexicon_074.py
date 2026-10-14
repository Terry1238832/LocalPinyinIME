#!/usr/bin/env python3
"""Audit a fixed upstream lexicon snapshot, independent of later authoring edits.

Reads Git objects and an optional native evaluator; never edits dictionary rows,
personal data, source metadata, or ranking weights. Pypinyin is authoring-only.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from datetime import datetime, timezone
import functools
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).absolute().parents[1]
BASE = "4bb1a63a6d3475301f0e1900a9e90ace40d96a51"
REMOTE = "cc2eeff4ab73240e276d913bcda93c4a1ca254cd"
RESOURCE = "resources/dictionary/"
NAMES = ("core_zh_pinyin.tsv", "local_core_zh_pinyin.tsv", "standard_zh_pinyin.tsv", "context_zh.tsv", "continuations_zh.tsv")
BATCHES = tuple(f"lexicon_batch_{n:03d}" for n in range(12, 16))
SAMPLES = (("wangyi", "网易"), ("yizhi", "一直"), ("yizhi", "一致"), ("yiqi", "一起"),
           ("yishi", "意识"), ("tuan", "图案"), ("wangyi", "王一"), ("wangyi", "王逸"),
           ("yizhi", "一智"), ("xingshangyangtuo", "行商羊驼"), ("hangshangyangtuo", "行商羊驼"),
           ("zhongcunyoucai", "种村有菜"), ("chongcunyoucai", "种村有菜"),
           ("zheyuanyi", "折原一"), ("sheyuanyi", "折原一"), ("letong", "乐瞳"),
           ("yuetong", "乐瞳"), ("mawei", "马尾"), ("mayi", "马尾"))


def git_blob(commit: str, name: str) -> bytes:
    return subprocess.check_output(["git", "-c", f"safe.directory={ROOT.as_posix()}", "show", f"{commit}:{name}"], cwd=ROOT)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse(data: bytes):
    records, errors, category = [], [], ""
    for number, line in enumerate(data.decode("utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            if line.startswith("# category:"):
                category = line.partition(":")[2].strip()
            continue
        fields = line.split("\t")
        if len(fields) != 3:
            errors.append({"line": number, "reason": "field_count"})
            continue
        key, word, weight = fields
        if not re.fullmatch("[a-z]+", key) or not word or not weight.isdigit():
            errors.append({"line": number, "reason": "invalid_key_word_or_weight"})
            continue
        records.append({"pinyin": key, "word": word, "weight": int(weight), "category": category, "line": number})
    return records, errors


def pair(record):
    return record["pinyin"], record["word"]


def han(character):
    return any(lo <= ord(character) <= hi for lo, hi in ((0x3400, 0x4dbf), (0x4e00, 0x9fff), (0x20000, 0x323af)))


def pronunciation_flags(rows):
    import pypinyin
    from pypinyin import Style, pinyin, lazy_pinyin

    @functools.lru_cache(maxsize=None)
    def readings(character):
        return tuple(pinyin(character, style=Style.NORMAL, heteronym=True, strict=False)[0])

    @functools.lru_cache(maxsize=None)
    def compatible(text, key):
        possible = {0}
        for ch in text:
            possible = {offset + len(reading) for offset in possible for reading in readings(ch) if key.startswith(reading, offset)}
            if not possible:
                return False
        return len(key) in possible

    incompatible, compound, unknown, mixed = [], [], [], []
    for row in rows:
        text = "".join(character for character in row["word"] if han(character))
        others = "".join(character for character in row["word"] if not han(character) and character not in "·・‧•．. ")
        if others:
            mixed.append({**row, "other_characters": others})
            continue
        unread = [ch for ch in text if ch in readings(ch)]
        if unread:
            unknown.append({**row, "unknown_characters": unread})
            continue
        common = "".join(lazy_pinyin(text, style=Style.NORMAL, strict=False))
        if not compatible(text, row["pinyin"]):
            incompatible.append({**row, "default_compound_key": common})
        elif row["pinyin"] != common:
            compound.append({**row, "default_compound_key": common})
    return {"checker": f"pypinyin {pypinyin.__version__} authoring-only", "method": "Remove middle-dot punctuation; dynamic matching against all toneless per-character readings. Default compound disagreement is a review flag, never proof of an error. Mixed-script titles and unknown characters are separate.", "incompatible_per_character_readings": incompatible, "default_compound_disagreements": compound, "unknown_characters": unknown, "mixed_script_rows": mixed}


def stats(rows):
    display = Counter(row["word"] for row in rows)
    keys = Counter(row["pinyin"] for row in rows)
    return {"rows": len(rows), "unique_key_display_pairs": len({pair(row) for row in rows}), "distinct_displays": len(display), "distinct_keys": len(keys), "weights": dict(sorted(Counter(row["weight"] for row in rows).items())), "display_lengths": dict(sorted(Counter(len(row["word"]) for row in rows).items())), "max_display_characters": max(len(row["word"]) for row in rows), "max_key_characters": max(len(row["pinyin"]) for row in rows), "same_key_multiple_displays": sum(count > 1 for count in keys.values()), "same_display_multiple_keys": sum(count > 1 for count in display.values())}


def evaluate(engine: Path, resources: Path, rows):
    cases = resources / "audit-cases.tsv"
    cases.write_text("".join(f"{r['pinyin']}\t{r['word']}\t{r['category']}\n" for r in rows), encoding="utf-8", newline="\n")
    env = dict(os.environ, LOCALPINYINIME_LOCALAPPDATA=str(resources / "private"), LOCALPINYINIME_USER_LEXICON_PATH=str(resources / "private/user_lexicon.tsv"))
    result = subprocess.run([str(engine.absolute()), str(resources), str(cases)], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    output = [json.loads(line) for line in result.stdout.decode("utf-8").splitlines() if line]
    return summarize_native(output, rows), output


def summarize_native(output, rows):
    assert len(output) == len(rows) and {pair(row) for row in rows} == {(row["pinyin"], row["expected"]) for row in output}
    duration = sorted(row["lookup_us"] for row in output)
    summary = {"cases": len(output), "top1_present": sum(row["expected_rank"] == 1 for row in output), "top8_present": sum(1 <= row["expected_rank"] <= 8 for row in output), "top9_present": sum(1 <= row["expected_rank"] <= 9 for row in output), "max_returned_candidates": max(len(row["candidates"]) for row in output), "p50_lookup_ms": round(duration[len(duration) // 2] / 1000, 3), "p95_lookup_ms": round(duration[min(len(duration) - 1, math.floor(.95 * len(duration)))] / 1000, 3), "max_lookup_ms": round(duration[-1] / 1000, 3)}
    missing = [{"pinyin": row["pinyin"], "word": row["expected"], "rank": row["expected_rank"], "candidates": row["candidates"]} for row in output if not 1 <= row["expected_rank"] <= 9]
    groups = {}
    for category in BATCHES:
        subset = [row for row in output if row["source"] == category]
        groups[category] = {"cases": len(subset), "top1_present": sum(row["expected_rank"] == 1 for row in subset), "top8_present": sum(1 <= row["expected_rank"] <= 8 for row in subset), "top9_present": sum(1 <= row["expected_rank"] <= 9 for row in subset)}
    return {"summary": summary, "by_batch": groups, "missing_top9": missing, "rows_sha256": sha(json.dumps(output, ensure_ascii=False, separators=(",", ":")).encode("utf-8"))}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", type=Path)
    parser.add_argument("--skip-pronunciation", action="store_true")
    parser.add_argument("--reuse-native-output", type=Path, help="Use previously audited full results, checking their immutable snapshot and engine SHA against the existing public report.")
    args = parser.parse_args()
    previous_path = ROOT / "docs/remote-lexicon-audit-074.json"
    previous = json.loads(previous_path.read_text(encoding="utf-8")) if args.reuse_native_output else None
    base, remote = {}, {}
    for name in NAMES:
        base[name] = git_blob(BASE, RESOURCE + name)
        remote[name] = git_blob(REMOTE, RESOURCE + name)
    old, old_errors = parse(base["local_core_zh_pinyin.tsv"])
    current, errors = parse(remote["local_core_zh_pinyin.tsv"])
    old_map = {pair(row): row for row in old}
    current_map = {pair(row): row for row in current}
    added = [row for row in current if pair(row) not in old_map]
    deleted = [row for row in old if pair(row) not in current_map]
    changed = [{"pinyin": row["pinyin"], "word": row["word"], "before_weight": old_map[pair(row)]["weight"], "after_weight": row["weight"]} for row in current if pair(row) in old_map and row["weight"] != old_map[pair(row)]["weight"]]
    metadata = json.loads(git_blob(REMOTE, RESOURCE + "local_core_zh_pinyin.metadata.json"))
    sources = json.loads(git_blob(REMOTE, RESOURCE + "sources/lexicon_sources.json"))
    known_sources = {source["source_id"] for source in sources["sources"]}
    layer = [row for name in ("core_zh_pinyin.tsv", "standard_zh_pinyin.tsv") for row in parse(remote[name])[0]]
    existing = {pair(row) for row in layer} | set(old_map)
    displays = {row["word"] for row in layer + old}
    fixture_answers = {line.split("\t")[1] for file in (ROOT / "tests/data").glob("typing_*.tsv") for line in file.read_text(encoding="utf-8").splitlines() if line and not line.startswith("#") and len(line.split("\t")) >= 3}
    counts = Counter(pair(row) for row in current)
    category_counts = Counter(row["category"] for row in current)
    buckets = {"single": 0, "two": 0, "three": 0, "fourplus": 0}
    for row in current:
        length = len(row["word"])
        buckets[("single", "two", "three", "fourplus")[min(length, 4) - 1]] += 1
    report = {"format_version": 1, "audit": "fixed_remote_lexicon_snapshot_074", "audited_at_utc": datetime.now(timezone.utc).isoformat(), "base_commit": BASE, "remote_commit": REMOTE, "method": "Immutable Git blobs; no source dictionary or rank edits. Native coverage, when present, uses temporary empty personal dictionaries and exact immutable remote resources.", "resource_sha256": {name: sha(data) for name, data in remote.items()}, "unchanged_runtime_files": [name for name in NAMES if base[name] == remote[name]], "static": {"base_rows": len(old), "remote_rows": len(current), "added_rows": len(added), "removed_rows": len(deleted), "removed": deleted, "existing_weights_changed": changed, "invalid_format_rows": old_errors + errors, "duplicate_pairs": [{"pinyin": key, "word": word, "count": count} for (key, word), count in counts.items() if count > 1], "metadata_count_matches": metadata["entry_count"] == len(current), "metadata_sha_matches": metadata["sha256"].lower() == sha(remote["local_core_zh_pinyin.tsv"]), "metadata_categories_match": metadata["category_counts"] == dict(category_counts), "metadata_lengths_match": metadata["length_distribution"] == buckets, "missing_source_registry_ids": sorted(set(metadata["source_ids"]) - known_sources), "added_pairs_already_in_other_layers": sum(pair(row) in existing for row in added), "added_displays_already_in_other_layers_or_base": sum(row["word"] in displays for row in added), "added_full_fixture_answer_matches": [row for row in added if row["word"] in fixture_answers], "frozen_fixture_answers_checked": len(fixture_answers), "all_three_layer_unique_key_display_pairs": len(set(current_map) | {pair(row) for row in layer}), "canonical_utf8_lf": b"\r" not in remote["local_core_zh_pinyin.tsv"] and not remote["local_core_zh_pinyin.tsv"].startswith(b"\xef\xbb\xbf"), "by_batch": {category: stats([row for row in added if row["category"] == category]) for category in BATCHES}}, "provenance": {"status": "external-reference-derived title/name lists need accurately separated provenance; not a determination of the copyrightability or license of individual factual names", "batch012": {"commit": "7abb248bf2a62255aa5ddb3163fe9a3b393549c9", "declared_source": "Three public texts: news, forum thread, fiction chapter", "exact_urls_or_revision_ids_in_repository": False}, "batch013": {"commit": "97e2e9e7876d53dfc3756964fe58ac81e9ab74b2", "declared_source": "zh.minecraft.wiki items, blocks, mobs, effects, aliases", "exact_page_urls_or_revision_ids_in_repository": False}, "batch014": {"commit": "0228b69d96cb4ec3c63abe19cfac49727f25c0ae", "declared_source": "HoYoLAB home-page games/channels/tools and Genshin, Star Rail, Zenless Zone Zero wiki names", "exact_page_urls_or_revision_ids_in_repository": False}, "batch015": {"commit": "025b5893ce6fcfedb05edf5106303404181e36df", "declared_source": "Chinese Wikipedia category member article titles, not article bodies; documented in docs/famous-people-lexicon.md", "exact_category_urls_or_revision_ids_in_repository": False}, "metadata_policy_needs_revision": "Unqualified first-party authored and first_party_lexicon_batch_012..015 imply original authoring even though upstream commits and document describe externally gathered names/titles. Record source-derived facts separately; do not fabricate a blanket MIT grant or unsupported third-party license."}}
    latin = [row for row in added if re.search("[A-Za-z]", row["word"])]
    pure_latin = [row for row in latin if not any(han(ch) for ch in row["word"])]
    mixed_latin = [row for row in latin if any(han(ch) for ch in row["word"])]
    report["latin_title_audit"] = {"contains_ascii_letters": len(latin), "non_han_latin_titles": len(pure_latin), "mixed_han_latin_titles": len(mixed_latin), "non_han_title_key_equals_lowercased_ascii_letters": sum(row["pinyin"] == re.sub("[^A-Za-z]", "", row["word"]).lower() for row in pure_latin), "by_batch": {category: {"non_han_latin_titles": sum(row["category"] == category for row in pure_latin), "mixed_han_latin_titles": sum(row["category"] == category for row in mixed_latin)} for category in BATCHES}, "representative_syllable_collisions": [row for row in pure_latin if row["pinyin"] in ("ban", "gui", "kai", "lan", "lang", "long", "jin", "keng", "ran", "zun")], "recommendation": "Keep factual Latin titles for exact key lookup. Do not treat their letters as Han syllable words during default Chinese sentence composition. Apply a generic script eligibility rule rather than changing weights to fit public typing fixtures."}
    if not args.skip_pronunciation:
        full_screen = pronunciation_flags(added)
        private_screen = ROOT / ".build/remote-074-pronunciation-screen.json"
        private_screen.write_text(json.dumps(full_screen, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
        report["pronunciation_screen"] = {"checker": full_screen["checker"], "method": full_screen["method"], "full_screen_sha256": sha(private_screen.read_bytes()), "full_screen_retention": "Temporary authoring details only; this tool can regenerate the complete flags from fixed Git commits.", "counts": {key: len(value) for key, value in full_screen.items() if isinstance(value, list)}, "default_disagreements_by_batch": dict(Counter(row["category"] for row in full_screen["default_compound_disagreements"])), "representative_review_flags": [row for row in full_screen["default_compound_disagreements"] if row["word"] in ("行商羊驼", "胜利只在朝夕间", "乐瞳", "折原一", "种村有菜", "重冈大毅")], "linguistic_review_scope": "Per-character compatibility is a structural screen only. Default compound disagreements and mixed scripts have not all received formal linguistic review. A successful stored-key lookup does not prove the spelling is the preferred input reading."}
    if args.engine:
        assert args.engine.is_file()
        before = sha(args.engine.read_bytes())
        if args.reuse_native_output:
            assert previous["remote_commit"] == REMOTE and previous["base_commit"] == BASE
            assert previous["resource_sha256"] == report["resource_sha256"]
            assert previous["native_coverage"]["engine_sha256"] == before
            native_rows = json.loads(args.reuse_native_output.read_text(encoding="utf-8"))
            report["native_coverage"] = summarize_native(native_rows, added)
            assert report["native_coverage"]["rows_sha256"] == previous["native_coverage"]["rows_sha256"]
            report["native_coverage"]["full_output_file_sha256"] = sha(args.reuse_native_output.read_bytes())
            report["native_coverage"]["evaluated_at_utc"] = previous.get("native_coverage", {}).get("evaluated_at_utc", previous["audited_at_utc"])
            report["native_coverage"]["pass_reused_without_requery"] = True
        else:
          with tempfile.TemporaryDirectory(prefix="remote074-native-", dir=ROOT / ".build") as directory:
            resource_root = Path(directory)
            (resource_root / "dictionary").mkdir()
            for name, data in remote.items():
                (resource_root / "dictionary" / name).write_bytes(data)
            report["native_coverage"], native_rows = evaluate(args.engine, resource_root, added)
            output = ROOT / ".build/remote-074-native-rows.json"
            output.write_text(json.dumps(native_rows, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
            report["native_coverage"]["full_output_file_sha256"] = sha(output.read_bytes())
          report["native_coverage"]["evaluated_at_utc"] = datetime.now(timezone.utc).isoformat()
        assert sha(args.engine.read_bytes()) == before
        report["native_coverage"]["engine_sha256"] = before
        report["native_coverage"]["personal_data_isolated"] = True
        report["native_coverage"]["expected_answers_learned"] = False
        sample_cases = [{"pinyin": key, "word": word, "category": "independent-homophone-and-pronunciation-sample"} for key, word in SAMPLES]
        samples = {}
        with tempfile.TemporaryDirectory(prefix="remote074-samples-", dir=ROOT / ".build") as directory:
            for label, runtime in (("base", base), ("remote", remote)):
                sample_root = Path(directory) / label
                (sample_root / "dictionary").mkdir(parents=True)
                for name, data in runtime.items():
                    (sample_root / "dictionary" / name).write_bytes(data)
                _, samples[label] = evaluate(args.engine, sample_root, sample_cases)
        assert sha(args.engine.read_bytes()) == before
        report["independent_samples"] = {"method": "Independent homophone and pronunciation probes selected by inspection of remote entries; not a blind benchmark or training input. Same native executable, exact base/remote Git resources, isolated empty personal data, no answer learning.", "engine_sha256": before, "samples": samples}
    path = ROOT / "docs/remote-lexicon-audit-074.json"
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"static": {k: v for k, v in report["static"].items() if k != "by_batch"}, "pronunciation_counts": report.get("pronunciation_screen", {}).get("counts"), "native_coverage": report.get("native_coverage", {}).get("summary")}, ensure_ascii=False))


if __name__ == "__main__":
    main()
