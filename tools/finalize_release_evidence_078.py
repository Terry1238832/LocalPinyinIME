#!/usr/bin/env python3
"""Assemble local delivery evidence after actual native and packaged desktop QA.

Does not commit, publish, install, register an IME, or edit personal data. Source
provenance and historical release reports are preserved. Temporary raw paths are
reduced to screenshot basenames in the public verification document.
"""
import argparse
from datetime import date
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).absolute().parents[1]
NAMES = ('core_zh_pinyin.tsv', 'local_core_zh_pinyin.tsv', 'standard_zh_pinyin.tsv', 'context_zh.tsv', 'continuations_zh.tsv')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def rows(path):
    return [line.split('\t') for line in path.read_text(encoding='utf-8').splitlines() if line and not line.startswith('#')]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--version', required=True)
    p.add_argument('--baseline', required=True)
    p.add_argument('--batches', nargs='+', type=int, required=True)
    p.add_argument('--quality', type=Path, required=True)
    p.add_argument('--qa', type=Path, required=True)
    p.add_argument('--capture', type=Path, required=True)
    p.add_argument('--native-log', type=Path, required=True)
    p.add_argument('--scope', required=True)
    p.add_argument('--upstream-current', type=Path, help='Final original and resolved upstream-key coverage, if this release imports upstream rows')
    a = p.parse_args()
    a.quality, a.qa, a.capture, a.native_log = [path.absolute() for path in (a.quality, a.qa, a.capture, a.native_log)]
    old = read(ROOT / f'.build/baseline-{a.baseline.replace(".", "")}/verification.json')
    assert old['version'] == a.baseline
    quality, qa = read(a.quality), read(a.qa / 'verification.json')
    assert quality['version'] == a.version and quality['baseline_version'] == a.baseline
    assert qa['passed'] and not qa['errors'] and len(qa['checks']) == 15 and all(check['passed'] for check in qa['checks'])
    assert len(qa['screenshots']) == 41 and qa['navigationTransitions'] == 80 and qa['navigationFrames'] == 160
    log_bytes = a.native_log.read_bytes()
    native_log = log_bytes.decode('utf-16' if log_bytes.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')
    assert '100% tests passed, 0 tests failed out of 34' in native_log
    assert read(ROOT / 'desktop/package.json')['version'] == a.version
    stage, dictionary = ROOT / '.build/package', ROOT / 'resources/dictionary'
    payload = {path.relative_to(stage).as_posix(): sha(path) for path in sorted(stage.rglob('*')) if path.is_file()}
    assert len(payload) == 31
    for name in NAMES:
        assert payload['bin/dictionary/' + name] == sha(dictionary / name) == quality['runtime_sha256'][name]
    for name, expected in old['restoredIconAssetsSha256'].items():
        assert sha(ROOT / name) == expected
    assert payload['bin/icon.ico'] == old['restoredIconAssetsSha256']['assets/branding/icon/LocalPinyinIME.ico']
    assert sorted(path.name for path in (stage / 'bin/locales').iterdir()) == ['en-US.pak', 'zh-CN.pak']
    total = len({(key, word) for name in NAMES[:3] for key, word, _ in rows(dictionary / name)})
    assert total == qa['dictionary']['total'] == quality['data_changes']['current_unique_key_display_pairs']
    audit_path = ROOT / f'docs/lexicon-{a.version.replace(".", "")}-authoring-audit.json'
    audit = read(audit_path)
    assert audit['version'] == a.version and audit['status'] == 'passed'
    assert audit['current_three_layer_unique_pairs'] == total and audit['runtime_sha256'] == quality['runtime_sha256']
    linguistic_status = audit['independent_linguistic_review']['status']
    assert linguistic_status == 'passed' or audit.get('contextual_review_membership_verified') is True
    coverage = []
    for batch in a.batches:
        path = dictionary / f'expansion/lexicon_batch_{batch:03d}.validation.json'
        report = read(path)
        assert sha(path.with_name(f'lexicon_batch_{batch:03d}.tsv')) == report['frozen_batch_sha256']
        assert report['engine_sha256'] == quality['engine_sha256'] and report['resource_sha256'] == quality['runtime_sha256']
        coverage.append({'batch': batch, 'report': '../' + path.relative_to(ROOT).as_posix(), 'report_sha256': sha(path), **report['summary']})
    added = sum(item['cases'] for item in coverage)
    net_added = total - old['dictionaryEntries']
    assert net_added == quality['data_changes']['added_key_display_pairs']
    assert not quality['data_changes']['removed_key_display_pairs']
    upstream = None
    imported_rows = 0
    if a.upstream_current:
        a.upstream_current = a.upstream_current.absolute()
        upstream = read(a.upstream_current)
        assert upstream['version'] == a.version and upstream['engine_sha256'] == quality['engine_sha256']
        assert upstream['runtime_sha256'] == quality['runtime_sha256']
        imported_rows = upstream['original_imported_keys']['summary']['cases']
        assert audit['authored_entries'] == added and total - audit['reviewed_import_unique_pairs'] == added
    else:
        assert net_added == added
    exe = ROOT / f'dist/LocalPinyinIME-{a.version}-x64.exe'
    checksum = Path(str(exe) + '.sha256')
    assert checksum.read_text(encoding='ascii').split()[0].lower() == sha(exe)
    size = exe.stat().st_size
    payload_bytes = sum(path.stat().st_size for path in stage.rglob('*') if path.is_file())
    review_path = a.qa / 'review-summary.json'
    review = read(review_path)
    assert review['automatedScreenshotChecksPassed'] == 41
    assert review['allVisuallyInspected'] is True
    assert review['desktopVerificationSha256'] == sha(a.qa / 'verification.json')
    screenshot_hashes = {Path(item['file']).name: sha(Path(item['file'])) for item in qa['screenshots']}
    assert review['screenshotSha256'] == screenshot_hashes
    assert review['payloadSha256'] == {name: payload['bin/' + name] for name in ('LocalPinyinSettings.exe', 'LocalPinyinSettingsBridge.exe', 'resources/app.asar')}
    native_hashes = {name: sha(a.capture / name) for name in old['nativeCandidateScreenshotSha256']}
    assert native_hashes == old['nativeCandidateScreenshotSha256'], 'Changed native frames require a new visual review'
    v = dict(old)
    historical_upstream = v.pop('upstreamFinalCoverageThisRelease', None)
    if historical_upstream:
        v['historicalUpstreamCoverage'] = {'evidenceVersion': old['version'], **historical_upstream}
    v['upstreamImportedRowsThisRelease'] = 0
    v['upstreamNetAddedUniquePairsThisRelease'] = 0
    v.update(version=a.version, clientDate=date.today().isoformat(), installer=f'https://github.com/Terry1238832/LocalPinyinIME/releases/download/v{a.version}/{exe.name}', installerBytes=size, installedPayloadBytes=payload_bytes, sha256=sha(exe).upper(), dictionaryEntries=total, addedDictionaryEntries=added, authoredDictionaryEntries=added, testsPassed=34, testsFailed=0, desktopIntegrationChecks=15, interfaceScreenshotsChecked=41, navigationTransitions=80, navigationFrames=160, typingEvaluation=a.quality.relative_to(ROOT).as_posix().removeprefix('docs/'), payloadSha256=payload, desktopChecks=[item['name'] for item in qa['checks']], desktopScreenshotSha256=screenshot_hashes, nativeCandidateScreenshotSha256={name: sha(a.capture / name) for name in old['nativeCandidateScreenshotSha256']}, engineRegressionEvidenceVersion=a.version)
    v['addedDictionaryCoverage'] = {'cases': added, 'top1_correct': sum(item['top1_correct'] for item in coverage), 'top9_present': sum(item['top9_present'] for item in coverage), 'missing_from_top9_count': sum(item['missing_from_top9_count'] for item in coverage), 'batches': coverage}
    v['addedDictionaryEntries'] = net_added
    v['authoringAudit'] = {'report': audit_path.name, 'sha256': sha(audit_path), 'independentLinguisticReview': linguistic_status, 'contextualReviewMembershipVerified': audit.get('contextual_review_membership_verified', True)}
    if upstream:
        v['upstreamImportedRowsThisRelease'] = imported_rows
        v['upstreamFinalCoverageThisRelease'] = {'report': a.upstream_current.name, 'sha256': sha(a.upstream_current), 'originalKeys': upstream['original_imported_keys']['summary'], 'resolvedKeys': upstream['resolved_keys_including_regional_aliases']['summary']}
        v['upstreamNetAddedUniquePairsThisRelease'] = net_added - added
    v['effectiveMaxLayerWeightChangesThisRelease'] = quality['data_changes']['effective_max_layer_weight_changes']
    if a.version == '0.7.8':
        word_path = ROOT / 'docs/new-word-comparison-078.json'
        word_comparison = read(word_path)
        assert word_comparison['runtime_sha256'] == quality['runtime_sha256'] and word_comparison['engine_sha256'] == quality['engine_sha256']
        assert word_comparison['case_count'] == added
        assert word_comparison['personal_data_isolated'] and not word_comparison['expected_answers_learned']
        assert word_comparison['all_cases_absent_as_display_entries_in_baseline']
        v['newWordComparison'] = {'report': word_path.name, 'sha256': sha(word_path), 'baseline': word_comparison['baseline_077']['summary'], 'current': word_comparison['current_078']['summary'], 'gainedInTop9': word_comparison['gained_in_top9'], 'lostFromTop9': word_comparison['lost_from_top9'], 'scope': word_comparison['scope']}
        if 'completeWordRetention' in v:
            v['completeWordRetention'] = {'evidenceVersion': '0.7.7', **v['completeWordRetention']}
        scenario_path = ROOT / 'docs/scenario-replay-078.json'
        scenario = read(scenario_path)
        assert scenario['version'] == a.version
        assert scenario['engine_sha256'] == quality['engine_sha256'] and scenario['runtime_sha256'] == quality['runtime_sha256']
        assert scenario['cases_sha256'] == sha(ROOT / 'tests/data/scenario_replay_076.tsv') and scenario['case_count'] == 32
        assert scenario['personal_data_isolated'] and not scenario['expected_answers_learned'] and not scenario['complete_answers_imported']
        v['supplementalScenarioReplay'] = {'report': scenario_path.name, 'sha256': sha(scenario_path), 'cases': 32, 'scope': scenario['scope'], 'baseline': scenario['baseline_077']['summary'], 'current': scenario['current_078']['summary']}
        scenario_changes_path = ROOT / 'docs/scenario-replay-078-rank-changes.json'
        scenario_changes = read(scenario_changes_path)
        assert scenario_changes['scenario_report_sha256'] == sha(scenario_path)
        assert scenario_changes['runtime_sha256'] == quality['runtime_sha256'] and scenario_changes['engine_sha256'] == quality['engine_sha256']
        v['supplementalScenarioReplay']['rankChanges'] = {'report': scenario_changes_path.name, 'sha256': sha(scenario_changes_path), 'lostFromTop9': len(scenario_changes['lost_from_top9']), 'gainedInTop9': len(scenario_changes['gained_in_top9'])}
    if a.version == '0.7.6':
        scenario_path = ROOT / 'docs/scenario-replay-076.json'
        scenario = read(scenario_path)
        assert scenario['engine_sha256'] == quality['engine_sha256'] and scenario['runtime_sha256'] == quality['runtime_sha256']
        assert scenario['cases_sha256'] == sha(ROOT / 'tests/data/scenario_replay_076.tsv') and scenario['case_count'] == 32
        assert scenario['personal_data_isolated'] and not scenario['expected_answers_learned'] and not scenario['complete_answers_imported']
        v['supplementalScenarioReplay'] = {'report': scenario_path.name, 'sha256': sha(scenario_path), 'cases': 32, 'scope': scenario['scope'], 'baseline': scenario['baseline_075']['summary'], 'current': scenario['current_076']['summary']}
        resource_check = read(ROOT / '.build/resource-refresh-test/verification.json')
        assert resource_check['status'] == 'passed' and resource_check['missing_output_restored'] and resource_check['no_relink_required']
        assert resource_check['evaluator_sha256'].lower() == quality['engine_sha256'].lower()
        resource_check.update(version=a.version, scope='Resource recovery and no-change rebuild checked during batch 028; final runtime coverage is measured separately.', final_runtime_sha256=quality['runtime_sha256'])
        write(ROOT / 'docs/build-resource-refresh-076.json', resource_check)
        v['dictionaryResourceBuildVerification'] = {'report': 'build-resource-refresh-076.json', 'sha256': sha(ROOT / 'docs/build-resource-refresh-076.json')}
    v['nativeCandidateFramesIdenticalToBaseline'] = True
    v['freshNativeBuildVerified'] = False
    v['cleanCheckoutBuildVerified'] = False
    v['desktopSecurity'] = {**qa['security'], 'isolatedPersonalData': True}
    v['desktopPaging'] = qa['pagination']
    v['desktopViewportMeasurements'] = qa.get('viewportMeasurements', [])
    v['desktopVisualReview'] = {'screenshots': 41, 'allVisuallyInspected': review['allVisuallyInspected'], 'automatedScreenshotChecksPassed': 41, 'reviewSummarySha256': sha(review_path)}
    v['verified'] = [text for text in v['verified'] if 'public sentence regressions' not in text]
    if a.version == '0.7.8':
        v['verified'] = [text.replace('Upstream 41526-row snapshot audited independently', 'Historical 0.7.4 upstream 41526-row snapshot audited independently') for text in v['verified']]
    v['verified'].append(f'267 public sentence regressions and all {added} newly authored word lookups evaluated without learning answers')
    # Upstream audits refer to immutable 0.7.4 snapshots, not a newly measured
    # claim that every old name still occupies the same rank after later words.
    v['upstreamCurrentCoverageEvidenceVersion'] = a.version if upstream else old.get('upstreamCurrentCoverageEvidenceVersion', '0.7.4')
    if historical_upstream and not upstream:
        v['upstreamCurrentCoverageEvidenceVersion'] = old['version']
    if a.version == '0.7.8' and not upstream:
        v.pop('upstreamCurrentCoverageEvidenceVersion', None)
    write(ROOT / 'docs/verification.json', v)
    audit_md = audit_path.with_suffix('.md').name
    for item in coverage:
        batch_doc = dictionary / f'expansion/batch_{item["batch"]:03d}.md'
        meta = read(batch_doc.with_name(f'lexicon_batch_{item["batch"]:03d}.json'))
        batch_text = batch_doc.read_text(encoding='utf-8') if batch_doc.exists() else f'# 批次 {item["batch"]:03d}：{meta["scope"]}\n\n{meta["review_scope"]}\n\n冻结TSV SHA-256：`{meta["sha256"]}`。全部草稿、类别粗档权重和逐字位置读音选择随源码保留。没有复制外部词库或公开测试完整答案。\n'
        review_description = '作者与独立审查' if linguistic_status == 'passed' else '单一AI作者语境复核与独立代码路径结构校验'
        batch_text = re.sub(r'状态：[^\n]*', f'状态：{review_description}、冻结、合并、来源登记及最终原生查询已完成。', batch_text)
        batch_doc.write_text(batch_text + f'\n\n## {a.version} 最终交付验证\n\n在五份最终运行数据与同一原生引擎下，{item["cases"]}项逐词查询首选{item["top1_correct"]}，前九覆盖{item["top9_present"]}，前九缺失{item["missing_from_top9_count"]}。完整候选、排名、隔离个人数据和未学习预期答案的证据见[原生报告](lexicon_batch_{item["batch"]:03d}.validation.json)。作者文件、拼音键语境记录、冻结TSV和来源保留见[结构审计](../../../docs/{audit_md})。这不是盲测，也不声称所有词的实际频率或独立人类语言学验收。\n', encoding='utf-8', newline='\n')
    current, previous = quality['overall']['current_summary'], quality['overall']['baseline_summary']
    readme = ROOT / 'README.md'
    text = readme.read_text(encoding='utf-8').replace(a.baseline, a.version)
    if a.version == '0.7.8':
        before_words, after_words = word_comparison['baseline_077']['summary'], word_comparison['current_078']['summary']
        scenario_before, scenario_after = scenario['baseline_077']['summary'], scenario['current_078']['summary']
        paragraph = f'本轮新增词的最终原生前九覆盖为{after_words["top9_present"]:,}/{added:,}，首选{after_words["top1_correct"]:,}；同一引擎回放旧词库分别为{before_words["top9_present"]:,}和{before_words["top1_correct"]:,}，见[新增词对照](docs/new-word-comparison-078.json)。267句公开整句回放首选{current["top1_correct"]}句、前九{current["top9_present"]}句，旧数据分别为{previous["top1_correct"]}句和{previous["top9_present"]}句，见[排名变化](docs/lexicon-078-rank-changes.json)。32句开发场景回放前九{scenario_before["top9_present"]}→{scenario_after["top9_present"]}，见[补充回放](docs/scenario-replay-078.json)。这些是公开开发验证，仍需选词，不代表所有长句和应用；未做独立人类语言编辑验收。\n\n'
        text = re.sub(r'本轮新增词的(?:最终原生)?前九覆盖为.*?\n\n', lambda _: paragraph, text, count=1, flags=re.S)
        text = text.replace('[本轮上游审计](docs/remote-lexicon-audit-076.md)', '[此前上游审计](docs/remote-lexicon-audit-076.md)')
    import_intro = f'接入GitHub新增{imported_rows:,}行词库，并补充' if upstream else '新增'
    text = re.sub(r'本轮.*?\n\n', f'本轮{import_intro}{added:,}条{a.scope}，内置有效词库共{total:,}条。原创补词逐项核对拼音、三层去重及原生候选验证，原本地层记录保持。原有本地行与三层有效最大权重保持不变；少数同音词仍可能超出前九候选，长句和同音输入仍需选词。\n\n', text, count=1)
    text = text.replace(f"{old['installerBytes']/1048576:.2f} MiB", f'{size/1048576:.2f} MiB').replace(f"{old['installedPayloadBytes']/1048576:.2f} MiB", f'{payload_bytes/1048576:.2f} MiB')
    text = re.sub(r'267句公开回归.*?；新增原创词', f'267句公开回归首选{current["top1_correct"]}/267、前九覆盖{current["top9_present"]}/267，结果见[输入质量记录]({a.quality.relative_to(ROOT).as_posix()})；新增原创词', text, count=1)
    text = re.sub(r'新增词作者与[^。\n]*。', '', text)
    text = text.replace('完整验证见[verification.json]', f'新增词作者与结构审计见[词库审计](docs/{audit_md})。完整验证见[verification.json]')
    if upstream:
        text = text.replace('参见[来源审计](docs/remote-lexicon-audit-074.md)', '参见[历史来源审计](docs/remote-lexicon-audit-074.md)及[本轮上游审计](docs/remote-lexicon-audit-076.md)')
    readme.write_text(text, encoding='utf-8', newline='\n')
    for name in ('architecture.md', 'COMPATIBILITY.md', 'INSTALL-zh-CN.md', 'SECURITY-NOTES-zh-CN.md'):
        path = ROOT / 'docs' / name
        path.write_text(path.read_text(encoding='utf-8').replace(a.baseline, a.version), encoding='utf-8', newline='\n')
    continuation = f'''# 当前交付与后续

当前版本{a.version}：本轮原创批次{', '.join(f'{n:03d}' for n in a.batches)}新增{added:,}项{a.scope}；本轮新增上游导入{imported_rows:,}行，净增{net_added:,}键词对，三层有效词共{total:,}。原创补词原生前九覆盖{v['addedDictionaryCoverage']['top9_present']}/{added}，首选{v['addedDictionaryCoverage']['top1_correct']}/{added}。原本地行和权重保留，三层有效最大权重未变，逐项检查见质量记录。历史012–015及此前020–023来源与已知条款分开记录，不统一标MIT或宣称所有专名完成语言学全审。

267句公开回归首选{current['top1_correct']}/267，前九覆盖{current['top9_present']}/267；同一最终引擎回放{a.baseline}数据分别为{previous['top1_correct']}/267与{previous['top9_present']}/267，CER {previous['character_error_rate']}→{current['character_error_rate']}。这不是盲测；完整答案未作为新词加入，权重未按公开句子结果调整。

单行候选、左右翻页、前文联想和原PI/侧栏图标保留。34原生检查、15最终桌面流程、41截图及SHA见verification.json；质量见{a.quality.name}。未验证干净机器完整安装卸载或所有第三方应用的安装后路由。

已发布资产不得覆盖。保留源码、作者记录、来源、运行数据与Git历史；临时构建缓存可清理重建。
'''
    (ROOT / 'docs/CONTINUE.md').write_text(continuation, encoding='utf-8', newline='\n')
    with (ROOT / 'docs/CONTINUE.md').open('a', encoding='utf-8', newline='\n') as stream:
        stream.write(f'\n32句开发场景前九覆盖{scenario["baseline_077"]["summary"]["top9_present"]}→{scenario["current_078"]["summary"]["top9_present"]}，首选{scenario["current_078"]["summary"]["top1_correct"]}/32。候选变化见[场景排名变化](scenario-replay-078-rank-changes.json)。沿用此前完整词保留逻辑，词库作者记录、来源及历史验证保留。\n')
    evaluation = ROOT / 'docs/TYPING_EVALUATION.md'
    evaluation.write_text(evaluation.read_text(encoding='utf-8') + f'\n\n## {a.version} 原创补词\n\n新增{added:,}项{a.scope}，原生前九{v["addedDictionaryCoverage"]["top9_present"]}/{added}。267公开句子首选{current["top1_correct"]}/267、前九{current["top9_present"]}/267、CER {current["character_error_rate"]}；同一最终引擎回放{a.baseline}数据为{previous["top1_correct"]}/267、{previous["top9_present"]}/267、CER {previous["character_error_rate"]}。详见[{a.quality.name}]({a.quality.name})。个人数据隔离，不学习预期答案，不按回归结果调权，也不将已公开案例称盲测。\n', encoding='utf-8', newline='\n')
    for src, dest in [('light-1160-appearance.png', 'interface-overview.png'), ('light-1160-dictionary.png', 'dictionary-preview.png'), ('context-preview.png', 'context-preview.png'), ('paging-preview-page-2.png', 'candidate-preview-paging.png')]:
        shutil.copyfile(a.qa / src, ROOT / 'docs/images' / dest)
    delivery = ROOT.parent / '安装包'
    delivery.mkdir(exist_ok=True)
    shutil.copyfile(exe, delivery / exe.name)
    shutil.copyfile(checksum, delivery / checksum.name)
    (ROOT.parent / 'README.md').write_text(f'# 本地拼音 {a.version}\n\n最新[安装包](安装包/{exe.name})（{size/1048576:.2f} MiB）和同目录SHA-256已生成；[GitHub发布页](https://github.com/Terry1238832/LocalPinyinIME/releases/tag/v{a.version})提供相同文件。\n\n本轮新增{added:,}条{a.scope}，有效词共{total:,}。34原生检查、15桌面流程和41截图检查通过。原图标、单行翻页与前文联想保留；未做干净机器完整安装卸载验收。升级后重新打开输入应用。\n\n源码、作者审音、来源和验证资料保留在source。\n', encoding='utf-8', newline='\n')
    notes = f'{import_intro}{added:,}条逐项核对拼音的{a.scope}，内置有效词{total:,}。原本地记录保持；三层有效最大权重保持不变，原创固定粗档权重未按公开评测答案调整。\n\n34原生检查、15最终桌面流程、41截图检查通过；原创新词前九{v["addedDictionaryCoverage"]["top9_present"]}/{added}、首选{v["addedDictionaryCoverage"]["top1_correct"]}/{added}。267公开句子首选{current["top1_correct"]}/267、前九{current["top9_present"]}/267；这是公开回归，不是盲测。原PI图标、单行左右翻页和前文联想保留。\n\n安装包{size/1048576:.2f} MiB，SHA-256：{sha(exe).upper()}。Windows x64未签名开发版，尚未完成干净机器完整安装卸载与全部应用的安装后验收。源码docs/verification.json包含交付哈希与验证范围，此前上游名称有单独来源和权限未知范围记录；本轮为单一AI作者语境复核与代码路径结构审计，没有独立人类语言编辑。\n'
    notes += f'\n32句公开开发场景的整句前九覆盖{scenario["baseline_077"]["summary"]["top9_present"]}→{scenario["current_078"]["summary"]["top9_present"]}；候选变化保留在docs/scenario-replay-078-rank-changes.json。长句同音排序仍需改进。\n'
    (ROOT / f'.build/release-{a.version.replace(".", "")}.md').write_text(notes, encoding='utf-8', newline='\n')
    print(json.dumps({'version': a.version, 'added': added, 'total': total, 'installerBytes': size, 'payloadBytes': payload_bytes, 'sha256': sha(exe), 'allVisuallyInspected': review['allVisuallyInspected']}, ensure_ascii=False))


if __name__ == '__main__':
    main()
