"""Replay visible common-word requirements, with isolated personal data.

Compare old data, the finite priority changes alone, and final priorities plus
new vocabulary using the same final evaluator. These cases are public author
checks, not a blind or representative test of everyday typing accuracy.
"""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from common_word_priorities_079 import reconstruct_local, MANIFEST
from evaluate_typing import summarize

ROOT = Path(__file__).absolute().parents[1]
NAMES = ('core_zh_pinyin.tsv', 'local_core_zh_pinyin.tsv', 'standard_zh_pinyin.tsv',
         'context_zh.tsv', 'continuations_zh.tsv')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def main():
    base, current = ROOT / '.build/baseline-078', ROOT / '.build/bin'
    engine = current / 'LocalPinyinImeEval.exe'
    cases = ROOT / 'tests/data/common_word_order_079.tsv'
    folder = ROOT / '.build/common-priority079-final'
    priority = folder / 'priority-only/dictionary'
    priority.mkdir(parents=True, exist_ok=True)
    manifest = read(MANIFEST)
    for name in NAMES:
        data = (base / 'dictionary' / name).read_bytes()
        assert hashlib.sha256(data).hexdigest() == manifest['baseline_runtime_sha256'][name]
        if name == 'local_core_zh_pinyin.tsv':
            data = reconstruct_local(data, manifest)
        (priority / name).write_bytes(data)
    initial = {'cases': sha(cases), 'engine': sha(engine), 'manifest': sha(MANIFEST),
               'runtime': {n: sha(current / 'dictionary' / n) for n in NAMES}}
    assert initial['runtime'] == {n: sha(ROOT / 'resources/dictionary' / n) for n in NAMES}
    results = {}
    for label, resources in (('baseline', base), ('priority_only', priority.parent), ('current', current)):
        path = folder / (label + '.json')
        subprocess.run([sys.executable, str(ROOT / 'tools/evaluate_typing.py'),
                        '--engine', str(engine), '--resources', str(resources),
                        '--cases', str(cases), '--output', str(path)],
                       check=True, stdout=subprocess.DEVNULL)
        data = read(path)
        rows = data['rows']
        standalone = [r for r in rows if r['source'].startswith('priority:visible:')]
        context = [r for r in rows if r['source'].startswith('priority:context:')]
        assert len(standalone) == 36 and len(context) == 9
        results[label] = {'summary': data['summary'], 'standalone36': summarize(standalone),
                          'context9': summarize(context), 'full_output_sha256': sha(path), 'rows': rows}
    for label in ('priority_only', 'current'):
        assert results[label]['standalone36']['top1_correct'] == 36
        rows = {r['pinyin']: r for r in results[label]['rows'] if r['source'].startswith('priority:visible:')}
        assert rows['he']['candidates'][0] == '和' and '喝' in rows['he']['candidates'][1:]
        assert rows['tianjia']['candidates'][0] == '添加'
    assert sha(cases) == initial['cases'] and sha(engine) == initial['engine'] and sha(MANIFEST) == initial['manifest']
    assert initial['runtime'] == {n: sha(current / 'dictionary' / n) for n in NAMES}
    report = {'version': '0.7.9', 'measured_at_utc': datetime.now(timezone.utc).isoformat(),
              'scope': __doc__.strip(), 'engine_sha256': initial['engine'],
              'cases_sha256': initial['cases'], 'priority_manifest_sha256': initial['manifest'],
              'runtime_sha256': initial['runtime'], 'personal_data_isolated': True,
              'expected_answers_learned': False, 'not_blind_or_representative': True,
              'priority_entries': len(manifest['entries']), **results}
    (ROOT / 'docs/common-word-priorities-079.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(json.dumps({k: {x: v[x] for x in ('summary', 'standalone36', 'context9')}
                      for k, v in results.items()}, ensure_ascii=False))


if __name__ == '__main__':
    main()
