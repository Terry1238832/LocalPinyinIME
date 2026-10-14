#!/usr/bin/env python3
"""Apply finite editorial review to the immutable user-requested 020–023 import.

Existing 0.7.5 local entries and weights are retained byte-for-byte. The review
does not pretend every reference-derived name received an official phonetic
verification; regional aliases and ordinary reading corrections are separated.
"""
from collections import Counter
from datetime import date
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).absolute().parents[1]
BASE = ROOT/'resources/dictionary'
OLD = 'c6a1f769d8e6a1d37d7f86711df03ef2158fcc5e'
REMOTE = '435c8d880837cc12cee7228265cf14604515705f'
RAW_SHA = 'e2ddeee8765a5c73dd478a3a037e89cffbff4de939a2858744bb34f4b58adc39'
IDIOM = 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=120822&la=0&powerMode=0'
HEAVY = 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=8701&la=0&powerMode=0'
MULTIPLE = 'https://dict.revised.moe.edu.tw/dictView.jsp?ID=47823&la=0&powerMode=0'
BENG = 'https://ysj.bengbu.gov.cn/wzgl/wzbz/index.html'
BAISE = 'https://ipr.mofcom.gov.cn/hwwq_2/chn_eu_gi/index.html'
CORRECTIONS = {
    '北马其顿': ('beimqidun','beimaqidun','马ma音节漏字母；只修复中文词面对应的拼音。'),
    '埃斯瓦蒂尼': ('aisivatini','aisiwadini','瓦wa、蒂di；原键混入外文罗马拼写。'),
    '养尊处优': ('yangzunchuyu','yangzunchuyou','优you音节漏o。'),
    '削足适履': ('xuezushili','xuezushilv','履lv音节不等于li；保持辞语削xue。'),
    '妄自菲薄': ('wangzifibo','wangzifeibo','菲fei音节漏e；保持薄bo。'),
    '强人所难': ('qiangrenweinan','qiangrensuonan','所suo不能对应为wei；强迫语义qiang无声调键不变。'),
    '破镜重圆': ('pojinchongyuan','pojingchongyuan','镜jing漏g；再次团圆重chong保留。'),
    '吃一堑长一智': ('chiyiqianchangyizhi','chiyiqianzhangyizhi','增长才智长zhang，不是长短chang。'),
    '如释重负': ('rushichongfu','rushizhongfu','沉重负担重zhong；如与本地已有正确键重复，仅移除重复导入行。'),
    '安土重迁': ('antuchongqian','antuzhongqian','不轻易迁徙的慎重重zhong。'),
    '浓墨重彩': ('nongmochongcai','nongmozhongcai','浓厚、浓重色彩重zhong。'),
    '百色': ('bose','baise','大陆城市标准拉丁音译Baise对应百bai；不对历史所有读音作断言。'),
    '百色市': ('boseshi','baiseshi','大陆城市标准拉丁音译Baise对应百bai。'),
    '蚌埠': ('bangbu','bengbu','当地政府明确bengbu为汉语拼音全称；采用当地普通话城市主键。'),
    '蚌埠市': ('bangbushi','bengbushi','当地城市普通话主键bengbu；不是用台湾历史辞典的bangfu替换。'),
    '天长市': ('tianzhangshi','tianchangshi','城市天长Tianchang；长chang，非生长zhang。'),
    '子长市': ('zizhangshi','zichangshi','纪念谢子长的城市主键Zichang，长chang。'),
    '司马台长城': ('simataizhangcheng','simataichangcheng','长城固定复合词changcheng。'),
    '明长陵': ('mingzhangling','mingchangling','明代长陵的长采用chang。'),
    '海西蒙藏哈萨克族自治州': ('haiximengcanghasakezuzizhizhou','haiximengzanghasakezuzizhizhou','藏族名称中的藏zang，不是收藏cang。'),
    '雅鲁藏布大峡谷': ('yalucangbudaxiagu','yaluzangbudaxiagu','雅鲁藏布专名藏zang；不改其它藏cang词。'),
    '多重共线性': ('duozhonggongxianxing','duochonggongxianxing','多个层面多重duochong，非重量。'),
    '多重共线性诊断': ('duozhonggongxianxingzhenduan','duochonggongxianxingzhenduan','多重duochong统计术语。'),
    '多重插补': ('duozhongchabu','duochongchabu','多个插补多重duochong。'),
    '多重箭矢': ('duozhongjianshi','duochongjianshi','复数、多层箭矢的多重duochong；不是重型箭矢。'),
}
ALIASES = {
    '焢肉饭': ('hongroufan','普通话单字焢hong键新增；保留原kong方言约定键。','https://hakkadict.moe.edu.tw/search_result/?accent=5&id=307'),
    '哲蚌寺': ('zhebangsi','增加通常寺名bang键；单字蚌辞典也有beng又音，保留旧键，不宣称全地域非法。','https://dict.concised.moe.edu.tw/dictView.jsp?ID=1150&la=0&powerMode=0'),
    '鸡汤汆海蚌': ('jitangcuanhaibang','海蚌动物义普通话bang主键补充；保留原beng又音键。','https://dict.revised.moe.edu.tw/dictView.jsp?ID=308&la=0&powerMode=0'),
    '乐陵': ('laoling','地方志记录本地地名古音lao；保留官方拉丁标名等可能使用的le键。','https://www.lucheng.gov.cn/module/download/downfile.jsp?classid=0&filename=e9117dc0c2d84cadbc9ff9994bd2187f.pdf'),
    '乐陵市': ('laolingshi','地名本地lao键增加；le旧键保留，地区音与默认音不混作全球错误。','https://www.lucheng.gov.cn/module/download/downfile.jsp?classid=0&filename=e9117dc0c2d84cadbc9ff9994bd2187f.pdf'),
}
COMMITS = {
    20: ('f47cb189298b5a6b57635e383399a1516c842d5d','HoYoLAB leftover names and missing place/country/idiom/product headwords'),
    21: ('424c098f4ea65cac70ef12b72686e2e7f2f378dc','County-level and historic cities, scenic places and brands'),
    22: ('b3150ddfa58a61175a5511eaf9934f6992636158','Chinese dish names'),
    23: ('435c8d880837cc12cee7228265cf14604515705f','Honor of Kings, music, mathematics and Steam terms'),
}


def sha(data): return hashlib.sha256(data).hexdigest()
def write(path, value): path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
def blob(commit,name): return subprocess.check_output(['git','-c',f'safe.directory={ROOT.as_posix()}','show',f'{commit}:resources/dictionary/{name}'],cwd=ROOT)


def main():
    path = BASE/'local_core_zh_pinyin.tsv'
    before = path.read_bytes()
    assert sha(before) == RAW_SHA, 'Review applies only to the immutable fresh import'
    old = blob(OLD,'local_core_zh_pinyin.tsv')
    assert before.startswith(old.rstrip()+b'\n'), 'Old rows were not retained as a prefix'
    old_rows = {tuple(line.split('\t')[:2]): line for line in old.decode('utf-8').splitlines() if line and not line.startswith('#')}
    original_pairs = {tuple(line.split('\t')[:2]) for line in before.decode('utf-8').splitlines() if line and not line.startswith('#')}
    output, seen, changed, dropped, found, categories = [], set(), [], [], set(), Counter()
    originals, category = {}, ''
    for line in before.decode('utf-8').splitlines():
        if line.startswith('# category:'): category=line.partition(':')[2].strip()
        if not line or line.startswith('#'):
            output.append(line); continue
        key,word,weight=line.split('\t')
        if category in {f'lexicon_batch_{n:03d}' for n in COMMITS}:
            originals[word] = (key,weight,category)
            if word in CORRECTIONS:
                previous,new,reason=CORRECTIONS[word]
                assert key in (previous,new), (word,key,previous,new)
                if key == previous:
                    assert word not in found
                    found.add(word); key=new
                    changed.append({'word':word,'before':previous,'after':new,'reason':reason,'weight_unchanged':int(weight)})
                    if (key,word) in original_pairs:
                        dropped.append({'word':word,'key':key,'reason':'Corrected key already exists in canonical; remove only the incorrect imported alias and retain the existing correct row and weight.'})
                        continue
        if (key,word) in seen:
            assert (key,word) in old_rows, 'Unexpected within-import collision'
            dropped.append({'word':word,'key':key,'reason':'Corrected import duplicates preserved pre-0.7.6 local row; original weight retained.'})
            continue
        seen.add((key,word)); output.append(f'{key}\t{word}\t{weight}'); categories[category]+=1
    assert found == set(CORRECTIONS)
    aliases=[]
    for word,(key,reason,url) in ALIASES.items():
        assert word in originals and (key,word) not in seen
        _,weight,category=originals[word]
        # Keep provenance in its upstream batch, including the original lookup.
        insertion=max(index for index,line in enumerate(output) if line.startswith('# category: '+category))+1
        output.insert(insertion,f'{key}\t{word}\t{weight}'); seen.add((key,word)); categories[category]+=1
        aliases.append({'word':word,'added_key':key,'original_key_retained':originals[word][0],'reason':reason,'reference':url})
    after=('\n'.join(output)+'\n').encode('utf-8')
    assert set(old_rows.values()) <= set(output)
    path.write_bytes(after)
    metadata_path=BASE/'local_core_zh_pinyin.metadata.json'
    metadata=json.loads(metadata_path.read_text(encoding='utf-8'))
    lengths=Counter()
    for line in output:
        if line and not line.startswith('#'):
            word=line.split('\t')[1]; lengths[{1:'single',2:'two',3:'three'}.get(len(word),'fourplus')]+=1
    metadata.update(entry_count=sum(categories.values()),sha256=sha(after).upper(),generated_at=date.today().isoformat(),category_counts=dict(sorted(categories.items())),length_distribution=dict(sorted(lengths.items())))
    metadata['source_ids']=[f'upstream_contributed_lexicon_batch_{n:03d}' if identifier==f'first_party_lexicon_batch_{n:03d}' else identifier for identifier in metadata['source_ids'] for n in [next((n for n in COMMITS if identifier==f'first_party_lexicon_batch_{n:03d}'),0)]]
    metadata['source_policy'] += ' Additional user-requested upstream batches 020–023 are separately registered as contributed name/headword indexes with commit provenance and absent exact external source-page manifests. Their inclusion is technical review, not proof of first-party authorship or a blanket grant for website content.'
    write(metadata_path,metadata)
    registry_path=BASE/'sources/lexicon_sources.json'
    registry=json.loads(registry_path.read_text(encoding='utf-8'))
    prior_registry=json.loads(blob(OLD,'sources/lexicon_sources.json'))
    assert registry == prior_registry
    for number,(commit,description) in COMMITS.items():
        registry['sources'].append({'source_id':f'upstream_contributed_lexicon_batch_{number:03d}','source_name':description,'source_url':f'https://github.com/Terry1238832/LocalPinyinIME/commit/{commit}',
            'license':'LicenseRef-UpstreamContributedNameIndex','license_url':'','redistribution_allowed':None,'attribution_required':None,'share_alike_required':None,'commercial_use_allowed':None,'derivative_work_allowed':None,
            'review_status':'technical_verified_provenance_incomplete','reviewed_at':date.today().isoformat(),'included_in_release':True,
            'notes':'Included on the user explicit request to pull and merge their repository contributions. Immutable Git provenance, native stored-key coverage and finite editorial changes are recorded in docs/remote-lexicon-audit-076.json and docs/upstream-review-076.json. No precise external page/revision or harvesting manifest was provided; no protected page body, images, lyrics or definitions are imported by this review. Unknown external permissions remain unknown; this is not a blanket MIT or commercial-content license.'})
    write(registry_path,registry)
    report={'version':'0.7.6','base_commit':OLD,'remote_commit':REMOTE,'canonical_before_sha256':sha(before),'canonical_after_sha256':sha(after),'canonical_after_entries':metadata['entry_count'],
            'all_075_local_rows_and_weights_preserved':True,'source_registry_prior_records_preserved':registry['sources'][:len(prior_registry['sources'])]==prior_registry['sources'],
            'semantic_or_spelling_key_corrections':changed,'corrected_import_duplicates_removed':dropped,'legitimate_or_regional_aliases':aliases,
            'default_disagreement_flags_read':105,'selected_risk_words_read':557,'risk_words_scope':'All 557 selected upstream terms were read in three ordered segments, plus all 105 default disagreements and 8 structural flags; this is non-exhaustive editorial review of the entire 7708-row import, not formal linguistic approval of every name.',
            'reference_links':{'growth_idiom':IDIOM,'heavy_or_cautious_zhong':HEAVY,'multiple_chong':MULTIPLE,'bengbu_local_pinyin':BENG,'baise_official_romanization':BAISE},
            'unresolved_pronunciation_scope':['Official game-title or personal-name pronunciations without primary phonetic statements were not changed by tool default alone.','焢肉饭 keeps a documented regional kong lookup and adds Mandarin hong.','MOE historical 蚌埠市 bangfu differs from local Mainland bengbu; local city spelling is used.'],
            'ranking_weights_not_tuned_to_public_answers':True,'review_tool_sha256':sha(Path(__file__).read_bytes())}
    write(ROOT/'docs/upstream-review-076.json',report)
    snapshot=ROOT/'.build/upstream-reviewed-076/dictionary'; snapshot.mkdir(parents=True,exist_ok=False)
    for name in ('core_zh_pinyin.tsv','local_core_zh_pinyin.tsv','standard_zh_pinyin.tsv','context_zh.tsv','continuations_zh.tsv'): (snapshot/name).write_bytes((BASE/name).read_bytes())
    print(json.dumps({'corrected_keys':len(changed),'duplicate_corrected_rows_removed':len(dropped),'regional_alias_keys_added':len(aliases),'local_rows':metadata['entry_count'],'sha256':sha(after)},ensure_ascii=False))


if __name__ == '__main__': main()
