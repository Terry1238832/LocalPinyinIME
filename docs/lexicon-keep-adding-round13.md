# 词库续加第13轮（批次 262–269）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `5de728db85dafb32211f33b85ee475bb18228588`，引擎版本 1.0.0。分支通过合并吸收 main，不 rebase、不 force push。持续 PR 的总表从第十轮起累计。

- `local_core`：208652 → 210614，新增 **1962 条**。
- 权重 6000 / 5500 / 5000：0 / 1960 / 2 条。
- 最终 SHA-256：`3EE63F67941B9BBA445ABE7DCE13A89F1302C8E3BEF5B5C0302A041194B79EB9`。
- CMake 上限从 209495 调整到 231676，约为新总数的 110%；独立提交 `741ff868a00531cee444259ff8af0226c121f18a`。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 262 | 电竞 | curate | 438 | 0 | 437 | 1 | 96.58% | 100.00% | `8733a63e7b5ebebd3c27bd67e362650c180637c4` |
| 263 | 动漫与二次元 | curate | 444 | 0 | 444 | 0 | 96.62% | 100.00% | `af2b0d404ca6222fe5e96bca7998669f068efb6c` |
| 264 | 网络小说用语 | curate | 500 | 0 | 499 | 1 | 97.40% | 100.00% | `7be1ffdf808642225d283ed972b482eb10ca7a77` |
| 265 | 直播带货与电商运营 | curate | 500 | 0 | 500 | 0 | 91.40% | 100.00% | `3b954a17f9b91c0e30161d4cf0c0d37b7f9844d7` |
| 266 | 电竞较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `fac994644e85b5abdb05a2abb619ea89e0b56c19` |
| 267 | 动漫与二次元较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `f67a31622d6d2e61c7b64301744aec349efcc6d5` |
| 268 | 网络小说用语较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `379f4fd5c177f169b27b750829af2e2eb43594de` |
| 269 | 直播带货与电商运营较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `77ca35f7443d9793bd47a2139a50f0950582a9ed` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round13_exclusions.tsv`，共 457 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:incomplete_or_uncommon_label`：61 条。
- `quality:specific_tournament_name`：1 条。
- `duplicate:standard_zh_pinyin.tsv`：23 条。
- `duplicate:local_core_zh_pinyin.tsv`：289 条。
- `quality:over_specific_situation`：2 条。
- `quality:prefer_clearer_fandom_label`：1 条。
- `quality:subjective_derogatory_label_omit`：1 条。
- `quality:outdated_format_omit`：1 条。
- `quality:niche_imported_label_omit`：2 条。
- `quality:prefer_generic_unbranded_action_figure_term`：1 条。
- `quality:uncommon_contrast_label`：1 条。
- `quality:ambiguous_scale_label`：1 条。
- `quality:possible_brand_line_association`：1 条。
- `quality:uncertain_colloquial_accessory_label`：1 条。
- `quality:brand_derived_label_omit`：1 条。
- `quality:specific_platform_feature_label`：1 条。
- `duplicate:batch263`：4 条。
- `quality:derogatory_label_omit`：1 条。
- `quality:prefer_unambiguous_structural_label`：2 条。
- `quality:character_name_derived_niche_label`：1 条。
- `batch_size_limit`：57 条。
- `quality:incomplete_label`：1 条。
- `quality:prefer_full_term`：2 条。
- `quality:misleading_preorder_label`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1347 个含多音字的词在各批复核表逐字记录；14 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 电竞：重开、重赛、重做用重 `chong`；囤兵线的囤 `tun`。对线换血、换血优势和劣势的血按复合词选 `xue`；弹药类投掷物的弹 `dan`。长枪局的长 `chang`；禁选策略用 `celve`。
- 动漫：角色、主角、配角用角 `jue`，视角的角 `jiao`；动画长片的长 `chang`，外传和单行本分别为 `waizhuan`、`danxingben`。音乐的乐 `yue`，重温的重 `chong`，还原的还 `huan`。
- 网文：前传、后传、外传、小传的传 `zhuan`；重写、重发、重生、重复的重 `chong`；成长的长 `zhang`，长篇的长 `chang`；连载与转载的载按 `zai`。虐文用 `nvewen`。
- 电商：行业的行 `hang`；时长的长 `chang`；重启的重 `chong`，权重、毛重、净重的重 `zhong`；各类率用 `lv`。订单拆分的拆 `chai`，打折的折 `zhe`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1703/1703 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，210614 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round13_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.95% → 85.95% | +0.00 | 98.10% → 98.10% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.41% → 73.41% |
| 口语对话·整句 | 3368 | 80.43% → 80.43% | +0.00 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `CMakeLists.txt`
- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round13.md`
- `resources/dictionary/expansion/batch262_authored_draft.txt`
- `resources/dictionary/expansion/batch262_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch263_authored_draft.txt`
- `resources/dictionary/expansion/batch263_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch264_authored_draft.txt`
- `resources/dictionary/expansion/batch264_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch265_authored_draft.txt`
- `resources/dictionary/expansion/batch265_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch266_draft.txt`
- `resources/dictionary/expansion/batch266_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch267_draft.txt`
- `resources/dictionary/expansion/batch267_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch268_draft.txt`
- `resources/dictionary/expansion/batch268_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch269_draft.txt`
- `resources/dictionary/expansion/batch269_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_262.md`
- `resources/dictionary/expansion/batch_263.md`
- `resources/dictionary/expansion/batch_264.md`
- `resources/dictionary/expansion/batch_265.md`
- `resources/dictionary/expansion/batch_266.md`
- `resources/dictionary/expansion/batch_267.md`
- `resources/dictionary/expansion/batch_268.md`
- `resources/dictionary/expansion/batch_269.md`
- `resources/dictionary/expansion/lexicon_batch_262.applied.json`
- `resources/dictionary/expansion/lexicon_batch_262.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_262.json`
- `resources/dictionary/expansion/lexicon_batch_262.tsv`
- `resources/dictionary/expansion/lexicon_batch_262.validation.json`
- `resources/dictionary/expansion/lexicon_batch_263.applied.json`
- `resources/dictionary/expansion/lexicon_batch_263.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_263.json`
- `resources/dictionary/expansion/lexicon_batch_263.tsv`
- `resources/dictionary/expansion/lexicon_batch_263.validation.json`
- `resources/dictionary/expansion/lexicon_batch_264.applied.json`
- `resources/dictionary/expansion/lexicon_batch_264.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_264.json`
- `resources/dictionary/expansion/lexicon_batch_264.tsv`
- `resources/dictionary/expansion/lexicon_batch_264.validation.json`
- `resources/dictionary/expansion/lexicon_batch_265.applied.json`
- `resources/dictionary/expansion/lexicon_batch_265.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_265.json`
- `resources/dictionary/expansion/lexicon_batch_265.tsv`
- `resources/dictionary/expansion/lexicon_batch_265.validation.json`
- `resources/dictionary/expansion/lexicon_batch_266.json`
- `resources/dictionary/expansion/lexicon_batch_266.tsv`
- `resources/dictionary/expansion/lexicon_batch_267.json`
- `resources/dictionary/expansion/lexicon_batch_267.tsv`
- `resources/dictionary/expansion/lexicon_batch_268.json`
- `resources/dictionary/expansion/lexicon_batch_268.tsv`
- `resources/dictionary/expansion/lexicon_batch_269.json`
- `resources/dictionary/expansion/lexicon_batch_269.tsv`
- `resources/dictionary/expansion/probes/batch262_probe.tsv`
- `resources/dictionary/expansion/probes/batch263_probe.tsv`
- `resources/dictionary/expansion/probes/batch264_probe.tsv`
- `resources/dictionary/expansion/probes/batch265_probe.tsv`
- `resources/dictionary/expansion/probes/batch266_probe.tsv`
- `resources/dictionary/expansion/probes/batch267_probe.tsv`
- `resources/dictionary/expansion/probes/batch268_probe.tsv`
- `resources/dictionary/expansion/probes/batch269_probe.tsv`
- `resources/dictionary/expansion/round13_exclusions.tsv`
- `resources/dictionary/expansion/round13_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
