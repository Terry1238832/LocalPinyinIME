#!/usr/bin/env python3
"""Compare byte-frozen runtime data with the same native evaluator, without learning.

This is a public regression comparison, not blind testing. Reports preserve changed
candidate arrays and disclose newly added full fixture answers and weight changes.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from evaluate_typing import summarize

ROOT = Path(__file__).absolute().parents[1]
NAMES = ('core_zh_pinyin.tsv', 'local_core_zh_pinyin.tsv', 'standard_zh_pinyin.tsv', 'context_zh.tsv', 'continuations_zh.tsv')
FIXTURES = {'fixed': 'typing_holdout.tsv', 'fresh': 'typing_fresh.tsv', 'independent': 'typing_independent.tsv'}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def hashes(directory: Path) -> dict[str, str]:
    return {name: sha(directory / 'dictionary' / name) for name in NAMES}


def vocabulary(directory: Path) -> dict[tuple[str, str], int]:
    result = {}
    for name in NAMES[:3]:
        for line in (directory / 'dictionary' / name).read_text(encoding='utf-8-sig').splitlines():
            if line and not line.startswith('#'):
                key, word, weight = line.split('\t')
                pair = key, word
                result[pair] = max(result.get(pair, 0), int(weight))
    return result


def local_core(directory: Path) -> dict[tuple[str, str], int]:
    return {(key, word): int(weight) for line in (directory / 'dictionary/local_core_zh_pinyin.tsv').read_text(encoding='utf-8-sig').splitlines() if line and not line.startswith('#') for key, word, weight in [line.split('\t')]}


def changed_weights(before: dict, current: dict) -> list[dict]:
    return [{'pinyin': key, 'word': word, 'before': before[key, word], 'current': current[key, word]} for key, word in sorted(set(before) & set(current)) if before[key, word] != current[key, word]]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    parser.add_argument('--baseline-version', required=True)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--resources', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    before_hashes, current_hashes = hashes(args.baseline), hashes(args.resources)
    engine_hash = sha(args.engine)
    before_vocab, current_vocab = vocabulary(args.baseline), vocabulary(args.resources)
    before_local, current_local = local_core(args.baseline), local_core(args.resources)
    additions = set(current_vocab) - set(before_vocab)
    all_before, all_current, fixtures = [], [], {}
    added_answers = []
    for label, name in FIXTURES.items():
        fixture = ROOT / 'tests/data' / name
        fixture_hash = sha(fixture)
        reports = []
        for mode, resources in (('baseline', args.baseline), ('current', args.resources)):
            path = args.work_dir / f'{mode}-{label}.json'
            subprocess.run([sys.executable, str(ROOT / 'tools/evaluate_typing.py'), '--engine', str(args.engine.absolute()), '--resources', str(resources.absolute()), '--cases', str(fixture), '--output', str(path)], check=True, stdout=subprocess.DEVNULL)
            data = json.loads(path.read_text(encoding='utf-8'))
            assert data['evaluation']['engine_sha256'] == engine_hash
            assert data['evaluation']['cases_sha256'] == fixture_hash
            reports.append(data)
        before, current = reports
        assert len(before['rows']) == len(current['rows'])
        comparison = {'top1_changes': 0, 'top1_improvements': 0, 'top1_regressions': 0, 'top9_improvements': 0, 'top9_regressions': 0}
        changes = []
        for old, new in zip(before['rows'], current['rows']):
            assert (old['pinyin'], old['expected'], old['source']) == (new['pinyin'], new['expected'], new['source'])
            old_first, new_first = old['candidates'][:1], new['candidates'][:1]
            comparison['top1_changes'] += old_first != new_first
            comparison['top1_improvements'] += old['expected_rank'] != 1 and new['expected_rank'] == 1
            comparison['top1_regressions'] += old['expected_rank'] == 1 and new['expected_rank'] != 1
            comparison['top9_improvements'] += old['expected_rank'] == 0 and new['expected_rank'] > 0
            comparison['top9_regressions'] += old['expected_rank'] > 0 and new['expected_rank'] == 0
            if old['candidates'] != new['candidates']:
                changes.append({'source': new['source'], 'pinyin': new['pinyin'], 'expected': new['expected'], 'before': old['candidates'], 'current': new['candidates'], 'before_rank': old['expected_rank'], 'current_rank': new['expected_rank']})
            if (new['pinyin'], new['expected']) in additions:
                added_answers.append({'fixture': name, 'source': new['source'], 'pinyin': new['pinyin'], 'word': new['expected']})
        fixtures[label] = {'fixture': fixture.relative_to(ROOT).as_posix(), 'fixture_sha256': fixture_hash, 'baseline_summary': before['summary'], 'current_summary': current['summary'], 'comparison': comparison, 'candidate_array_changes': changes}
        all_before.extend(before['rows'])
        all_current.extend(current['rows'])
        assert sha(fixture) == fixture_hash
    report = {
        'version': args.version, 'baseline_version': args.baseline_version,
        'evaluated_at_utc': datetime.now(timezone.utc).isoformat(),
        'method': {'personal_data_isolated': True, 'expected_answers_learned': False, 'blind_evaluation': False, 'weights_not_adjusted_from_public_sentence_answers': True, 'runtime_frozen_during_evaluation': True, 'keystroke_benchmark_run': False, 'baseline_method': 'Same final evaluator replaying byte-exact prior-release data, not an unavailable old binary.', 'raw_reports_retained_after_delivery': False},
        'engine_sha256': engine_hash, 'runtime_sha256': current_hashes, 'baseline_runtime_sha256': before_hashes,
        'data_changes': {'baseline_unique_key_display_pairs': len(before_vocab), 'current_unique_key_display_pairs': len(current_vocab), 'added_key_display_pairs': len(additions), 'removed_key_display_pairs': len(set(before_vocab) - set(current_vocab)), 'baseline_local_core_entries': len(before_local), 'current_local_core_entries': len(current_local), 'added_local_core_rows': len(set(current_local) - set(before_local)), 'existing_local_core_weight_changes': changed_weights(before_local, current_local), 'effective_max_layer_weight_changes': changed_weights(before_vocab, current_vocab), 'new_full_fixture_answers': added_answers, 'new_full_fixture_answer_count': len(added_answers)},
        'fixtures': fixtures,
        'overall': {'baseline_summary': summarize(all_before), 'current_summary': summarize(all_current)},
        'scope': 'Public sentence regressions evaluated through the actual native engine. No OS keyboard dispatch, installed-profile compatibility or blind general-language accuracy claim.'
    }
    assert sha(args.engine) == engine_hash and hashes(args.baseline) == before_hashes and hashes(args.resources) == current_hashes, 'Frozen evaluator or runtime data changed during evaluation'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({'overall': report['overall'], 'data_changes': report['data_changes'], 'report_sha256': sha(args.output)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
