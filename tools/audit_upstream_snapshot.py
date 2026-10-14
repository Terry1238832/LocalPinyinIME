#!/usr/bin/env python3
"""Read immutable upstream Git dictionaries and measure their stored keys.

No dictionary, ranking, personal data, or source registry is modified. The
pronunciation scan is structural; agreement with pypinyin is not human review.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import tempfile

from audit_remote_lexicon_074 import git_blob, parse, pair, sha, stats, pronunciation_flags, evaluate, NAMES

ROOT = Path(__file__).absolute().parents[1]
RESOURCE = 'resources/dictionary/'


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')


def coverage(engine, resources, entries):
    report, output = evaluate(engine, resources, entries)
    report['by_batch'] = {}
    for category in sorted({row['category'] for row in entries}):
        subset = [row for row in output if row['source'] == category]
        report['by_batch'][category] = {
            'cases': len(subset), 'top1_present': sum(row['expected_rank'] == 1 for row in subset),
            'top8_present': sum(1 <= row['expected_rank'] <= 8 for row in subset),
            'top9_present': sum(1 <= row['expected_rank'] <= 9 for row in subset)}
    return report, output


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base', required=True)
    p.add_argument('--remote', required=True)
    p.add_argument('--version', required=True)
    p.add_argument('--batches', nargs='+', type=int, required=True)
    p.add_argument('--engine', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--current-resources', type=Path)
    a = p.parse_args()
    baselines = {name: git_blob(a.base, RESOURCE+name) for name in NAMES}
    remote = {name: git_blob(a.remote, RESOURCE+name) for name in NAMES}
    old, old_errors = parse(baselines['local_core_zh_pinyin.tsv'])
    new, new_errors = parse(remote['local_core_zh_pinyin.tsv'])
    assert not old_errors and not new_errors
    old_map, new_map = {pair(row): row for row in old}, {pair(row): row for row in new}
    assert len(new_map) == len(new), 'Duplicate canonical key/display pair'
    added = [row for row in new if pair(row) not in old_map]
    categories = {f'lexicon_batch_{number:03d}' for number in a.batches}
    assert {row['category'] for row in added} == categories
    deleted = [row for row in old if pair(row) not in new_map]
    changed = [{'pinyin': row['pinyin'], 'word': row['word'], 'before': old_map[pair(row)]['weight'], 'after': row['weight']}
               for row in new if pair(row) in old_map and row['weight'] != old_map[pair(row)]['weight']]
    other = [row for name in ('core_zh_pinyin.tsv','standard_zh_pinyin.tsv') for row in parse(remote[name])[0]]
    prior_pairs = set(old_map) | {pair(row) for row in other}
    prior_displays = {row['word'] for row in old+other}
    fixtures = {path.name: sha(path.read_bytes()) for path in sorted((ROOT/'tests/data').glob('typing_*.tsv'))}
    answers = {line.split('\t')[1] for filename in fixtures for line in (ROOT/'tests/data'/filename).read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')}
    metadata = json.loads(git_blob(a.remote, RESOURCE+'local_core_zh_pinyin.metadata.json'))
    registry = json.loads(git_blob(a.remote, RESOURCE+'sources/lexicon_sources.json'))
    length_counts = Counter({name: 0 for name in ('single','two','three','fourplus')})
    for row in new:
        length_counts[('single','two','three','fourplus')[min(len(row['word']),4)-1]] += 1
    screen = pronunciation_flags(added)
    screen_path = ROOT/f'.build/remote-{a.version.replace(".", "")}-pronunciation-screen.json'
    write(screen_path, screen)
    report = {
        'version': a.version, 'audited_at_utc': datetime.now(timezone.utc).isoformat(),
        'base_commit': a.base, 'remote_commit': a.remote, 'audit_tool_sha256': sha(Path(__file__).read_bytes()),
        'method': 'Exact immutable Git blobs and actual native lookup with isolated empty personal dictionaries. No learning or expected-answer training.',
        'resource_sha256': {name: sha(data) for name,data in remote.items()},
        'unchanged_runtime_files': [name for name in NAMES if baselines[name] == remote[name]],
        'static': {'base_rows': len(old), 'remote_rows': len(new), 'added_rows': len(added),
                   'removed_rows': deleted, 'existing_weight_changes': changed,
                   'canonical_utf8_lf': b'\r' not in remote['local_core_zh_pinyin.tsv'] and not remote['local_core_zh_pinyin.tsv'].startswith(b'\xef\xbb\xbf'),
                   'metadata_count_matches': metadata['entry_count'] == len(new),
                   'metadata_sha_matches': metadata['sha256'].lower() == sha(remote['local_core_zh_pinyin.tsv']),
                   'metadata_categories_match': metadata['category_counts'] == dict(Counter(row['category'] for row in new)),
                   'metadata_lengths_match': metadata['length_distribution'] == dict(length_counts),
                   'unregistered_source_ids': sorted(set(metadata['source_ids'])-{row['source_id'] for row in registry['sources']}),
                   'added_pairs_already_in_other_layers': sum(pair(row) in prior_pairs for row in added),
                   'added_displays_already_in_other_layers_or_base': sum(row['word'] in prior_displays for row in added),
                   'added_full_fixture_answers': [row for row in added if row['word'] in answers],
                   'fixture_sha256': fixtures, 'fixture_answer_count': len(answers),
                   'all_three_layer_unique_pairs': len(set(new_map)|{pair(row) for row in other}),
                   'by_batch': {category: stats([row for row in added if row['category'] == category]) for category in sorted(categories)}},
        'pronunciation_screen': {'checker': screen['checker'], 'method': screen['method'],
                                'full_screen_sha256': sha(screen_path.read_bytes()),
                                'counts': {key: len(value) for key,value in screen.items() if isinstance(value,list)},
                                'linguistic_scope': 'Structural scan only. A stored-key candidate does not establish naturalness, preferred compound reading, or an official personal-name pronunciation.'},
        'provenance': {'scope': 'These are user-requested upstream contributions. Commit descriptions mention referenced names/headwords; no external source-page or revision manifests were supplied. No blanket first-party authorship or unrestricted external license is asserted.',
                       'reference_commit_urls': [f'https://github.com/Terry1238832/LocalPinyinIME/commit/{a.remote}'],
                       'registry_review_required': True}}
    assert not deleted and not changed
    assert all(report['static'][name] for name in ('canonical_utf8_lf','metadata_count_matches','metadata_sha_matches','metadata_categories_match','metadata_lengths_match'))
    engine = a.engine.absolute()
    engine_hash = sha(engine.read_bytes())
    with tempfile.TemporaryDirectory(prefix='upstream-audit-', dir=ROOT/'.build') as directory:
        runtime = Path(directory)
        (runtime/'dictionary').mkdir()
        for name,data in remote.items(): (runtime/'dictionary'/name).write_bytes(data)
        measured, rows = coverage(engine, runtime, added)
        raw = ROOT/f'.build/remote-{a.version.replace(".", "")}-native-rows.json'
        write(raw, rows)
        measured.update(engine_sha256=engine_hash, full_output_sha256=sha(raw.read_bytes()), personal_data_isolated=True, expected_answers_learned=False)
        report['raw_upstream_native_coverage'] = measured
        if a.current_resources:
            current, rows = coverage(engine, a.current_resources.absolute(), added)
            output = ROOT/f'.build/remote-{a.version.replace(".", "")}-current-native-rows.json'
            write(output, rows)
            current.update(engine_sha256=engine_hash, full_output_sha256=sha(output.read_bytes()), runtime_sha256={name:sha((a.current_resources/'dictionary'/name).read_bytes()) for name in NAMES}, personal_data_isolated=True, expected_answers_learned=False)
            report['final_runtime_upstream_key_coverage'] = current
    assert sha(engine.read_bytes()) == engine_hash
    write(a.output, report)
    print(json.dumps({'static':{key:value for key,value in report['static'].items() if key not in ('by_batch','fixture_sha256')},'pronunciation':report['pronunciation_screen']['counts'],'native':report['raw_upstream_native_coverage']['summary']},ensure_ascii=False))


if __name__ == '__main__': main()
