# 词库续加第61轮（批次 586–593）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `4b2272e876ff6bc899b6eecb802dee0de83e5559`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：266765 → 268297，新增 **1532 条**。
- 权重 6000 / 5500 / 5000：0 / 1532 / 0 条。
- 最终 SHA-256：`02C826355CBF9AEF0BDC610E9FB5A14BC0073BD8430EBEF6DDC86340EF0DA87D`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 586 | 语音视频通话中的接听、听清与回拨聊天 | curate | 365 | 0 | 365 | 0 | 79.73% | 100.00% | `d1e280a735a6431e431c2ecf32f8d34b1df96868` |
| 587 | 分享图片、链接与文件后确认收到和打开聊天 | curate | 355 | 0 | 355 | 0 | 77.18% | 100.00% | `207bbbf4e831e47988a68c0f4435419b8b768228` |
| 588 | 群聊公告、消息提醒与补充信息沟通 | curate | 347 | 0 | 347 | 0 | 74.93% | 100.00% | `8b198031c5cd7b6eb31c62eb740e940e50712e2e` |
| 589 | 线下见面碰头、找人和确认地点聊天 | curate | 369 | 0 | 369 | 0 | 78.59% | 100.00% | `fa054db573bd030ea32b217414e6ca27dc4723ef` |
| 590 | 语音视频通话中听清、回拨与临时结束长句 | merge | 24 | 0 | 24 | 0 | 87.50% | 87.50% | `70d76dac399fdb43588c2f39d10c830ef1ff7715` |
| 591 | 分享照片、链接和文件后确认查看情况长句 | merge | 24 | 0 | 24 | 0 | 75.00% | 87.50% | `bd1af9d363e373f299d76adc78a04693578e09c9` |
| 592 | 群公告提醒、消息补充与确认收到长句 | merge | 24 | 0 | 24 | 0 | 79.17% | 87.50% | `ee9b3d8589bc48e08d4ba022e6f3fa098f20029d` |
| 593 | 线下碰头、确认出口和互相寻找长句 | merge | 24 | 0 | 24 | 0 | 91.67% | 95.83% | `ef8ee7f391bd0e451f33d22e0bcf60db2db7d26f` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round61_exclusions.tsv`，共 424 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：123 条。
- `outside_batch_length_scope`：276 条。
- `duplicate:batch586`：2 条。
- `语序不自然`：2 条。
- `搭配不自然，日常通常说原图`：1 条。
- `不自然的口语搭配`：1 条。
- `duplicate:batch587`：3 条。
- `non_BMP_Han_or_non_Han`：2 条。
- `缺少结果补语的不完整表达`：2 条。
- `语义不完整`：2 条。
- `缺少谓语的不完整表达`：1 条。
- `问法不自然`：1 条。
- `缺少比较项的不完整表达`：1 条。
- `duplicate:batch589`：5 条。
- `语义不清的省略表达`：1 条。
- `缺少比较对象的不完整表达`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1353 个含多音字的词在各批复核表逐字记录；6 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

本轮逐字语境决定和默认键差异均见各批 `pronunciation_review.tsv` 与批次 JSON。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 9/9 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，268297 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round61_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.25% → 88.25% | +0.00 | 98.70% → 98.70% |
| 逐词输入·无前文 | 20626 | 79.65% → 79.65% | +0.00 | 97.56% → 97.56% |
| 留出整句 | 4126 | 64.49% → 64.49% | +0.00 | 77.61% → 77.61% |
| 口语对话·整句 | 3368 | 85.36% → 85.36% | +0.00 | 94.69% → 94.69% |
| 口语对话·逐词 | 19504 | 95.58% → 95.58% | +0.00 | 99.72% → 99.72% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.67% → 96.67% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round61.md`
- `resources/dictionary/expansion/batch586_authored_draft.txt`
- `resources/dictionary/expansion/batch586_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch587_authored_draft.txt`
- `resources/dictionary/expansion/batch587_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch588_authored_draft.txt`
- `resources/dictionary/expansion/batch588_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch589_authored_draft.txt`
- `resources/dictionary/expansion/batch589_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch590_draft.txt`
- `resources/dictionary/expansion/batch590_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch591_draft.txt`
- `resources/dictionary/expansion/batch591_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch592_draft.txt`
- `resources/dictionary/expansion/batch592_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch593_draft.txt`
- `resources/dictionary/expansion/batch593_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_586.md`
- `resources/dictionary/expansion/batch_587.md`
- `resources/dictionary/expansion/batch_588.md`
- `resources/dictionary/expansion/batch_589.md`
- `resources/dictionary/expansion/batch_590.md`
- `resources/dictionary/expansion/batch_591.md`
- `resources/dictionary/expansion/batch_592.md`
- `resources/dictionary/expansion/batch_593.md`
- `resources/dictionary/expansion/lexicon_batch_586.applied.json`
- `resources/dictionary/expansion/lexicon_batch_586.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_586.json`
- `resources/dictionary/expansion/lexicon_batch_586.tsv`
- `resources/dictionary/expansion/lexicon_batch_586.validation.json`
- `resources/dictionary/expansion/lexicon_batch_587.applied.json`
- `resources/dictionary/expansion/lexicon_batch_587.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_587.json`
- `resources/dictionary/expansion/lexicon_batch_587.tsv`
- `resources/dictionary/expansion/lexicon_batch_587.validation.json`
- `resources/dictionary/expansion/lexicon_batch_588.applied.json`
- `resources/dictionary/expansion/lexicon_batch_588.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_588.json`
- `resources/dictionary/expansion/lexicon_batch_588.tsv`
- `resources/dictionary/expansion/lexicon_batch_588.validation.json`
- `resources/dictionary/expansion/lexicon_batch_589.applied.json`
- `resources/dictionary/expansion/lexicon_batch_589.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_589.json`
- `resources/dictionary/expansion/lexicon_batch_589.tsv`
- `resources/dictionary/expansion/lexicon_batch_589.validation.json`
- `resources/dictionary/expansion/lexicon_batch_590.json`
- `resources/dictionary/expansion/lexicon_batch_590.tsv`
- `resources/dictionary/expansion/lexicon_batch_591.json`
- `resources/dictionary/expansion/lexicon_batch_591.tsv`
- `resources/dictionary/expansion/lexicon_batch_592.json`
- `resources/dictionary/expansion/lexicon_batch_592.tsv`
- `resources/dictionary/expansion/lexicon_batch_593.json`
- `resources/dictionary/expansion/lexicon_batch_593.tsv`
- `resources/dictionary/expansion/probes/batch586_probe.tsv`
- `resources/dictionary/expansion/probes/batch587_probe.tsv`
- `resources/dictionary/expansion/probes/batch588_probe.tsv`
- `resources/dictionary/expansion/probes/batch589_probe.tsv`
- `resources/dictionary/expansion/probes/batch590_probe.tsv`
- `resources/dictionary/expansion/probes/batch591_probe.tsv`
- `resources/dictionary/expansion/probes/batch592_probe.tsv`
- `resources/dictionary/expansion/probes/batch593_probe.tsv`
- `resources/dictionary/expansion/round61_exclusions.tsv`
- `resources/dictionary/expansion/round61_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
