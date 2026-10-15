"""Apply the finite, user-requested common-word preference list for 0.7.9.

Historical authoring records and standard reference frequencies stay immutable.
The manifest is editorial ranking data, not measured corpus percentages.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).absolute().parents[1]
DICT = ROOT / 'resources/dictionary'
MANIFEST = DICT / 'sources/common_word_priorities_079.json'
SOURCE_ID = 'first_party_common_word_priority_079'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n',
                    encoding='utf-8', newline='\n')


def reconstruct_local(baseline, manifest):
    assert digest(baseline) == manifest['baseline_runtime_sha256']['local_core_zh_pinyin.tsv']
    entries = {(r['pinyin'], r['word']): r for r in manifest['entries']}
    assert len(entries) == len(manifest['entries'])
    seen, lines = set(), []
    for line in baseline.decode('utf-8').splitlines():
        if line and not line.startswith('#'):
            key, word, frequency = line.split('\t')
            pair = key, word
            if pair in entries:
                item = entries[pair]
                assert int(frequency) == item['before_local']
                assert item['priority'] == manifest['fixed_bands'][item['band']]
                assert int(frequency) < item['priority']
                seen.add(pair)
                line = f'{key}\t{word}\t{item["priority"]}'
        lines.append(line)
    lines += ['', '# category: ' + SOURCE_ID]
    for pair, item in entries.items():
        if pair not in seen:
            assert item['before_local'] is None
            assert item['priority'] == manifest['fixed_bands'][item['band']]
            lines.append(f'{pair[0]}\t{pair[1]}\t{item["priority"]}')
    return ('\n'.join(lines) + '\n').encode('utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', required=True)
    parser.parse_args()
    manifest = read(MANIFEST)
    before = (DICT / 'local_core_zh_pinyin.tsv').read_bytes()
    for name, expected in manifest['baseline_runtime_sha256'].items():
        assert digest((DICT / name).read_bytes()) == expected
    known = {(k, w) for name in ('core_zh_pinyin.tsv', 'local_core_zh_pinyin.tsv', 'standard_zh_pinyin.tsv')
             for line in (DICT / name).read_text(encoding='utf-8').splitlines()
             if line and not line.startswith('#') for k, w, _ in [line.split('\t')]}
    assert all((r['pinyin'], r['word']) in known for r in manifest['entries'])
    after = reconstruct_local(before, manifest)
    count = sum(bool(line) and not line.startswith(b'#') for line in after.splitlines())
    metadata = read(DICT / 'local_core_zh_pinyin.metadata.json')
    registry = read(DICT / 'sources/lexicon_sources.json')
    assert SOURCE_ID not in metadata['source_ids']
    assert all(r['source_id'] != SOURCE_ID for r in registry['sources'])
    metadata.update(entry_count=count, sha256=digest(after).upper())
    metadata['source_ids'].append(SOURCE_ID)
    metadata['source_policy'] += ' Common-word scores in 0.7.9 follow the separate finite editorial preference manifest requested by the user; historical frozen author records are unchanged.'
    registry['sources'].append({'source_id': SOURCE_ID, 'license': 'MIT',
                               'scope': 'Project-authored ranking preferences only; underlying dictionary provenance and licenses are retained.',
                               'manifest': MANIFEST.relative_to(ROOT).as_posix(),
                               'manifest_sha256': digest(MANIFEST.read_bytes()),
                               'review_status': 'single_ai_contextual_review',
                               'priority_entries': len(manifest['entries']),
                               'new_unique_key_display_pairs': 0})
    receipt = {'version': '0.7.9', 'applied_at_utc': datetime.now(timezone.utc).isoformat(),
               'manifest_sha256': digest(MANIFEST.read_bytes()),
               'canonical_before_sha256': digest(before), 'canonical_after_sha256': digest(after),
               'priority_entries': len(manifest['entries']),
               'changed_existing_local_scores': sum(r['before_local'] is not None for r in manifest['entries']),
               'new_local_overrides_for_existing_words': sum(r['before_local'] is None for r in manifest['entries']),
               'new_unique_key_display_pairs': 0, 'all_other_runtime_files_unchanged': True,
               'historical_frozen_author_files_unchanged': True}
    (DICT / 'local_core_zh_pinyin.tsv').write_bytes(after)
    write(DICT / 'local_core_zh_pinyin.metadata.json', metadata)
    write(DICT / 'sources/lexicon_sources.json', registry)
    write(ROOT / 'docs/common-word-priorities-079-application.json', receipt)
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
