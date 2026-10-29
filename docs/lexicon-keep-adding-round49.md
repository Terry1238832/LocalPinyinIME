# 词库续加第49轮（批次 514–519）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `40b5cb24756d766fe38711f42aa8862e7c76fb78`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：253593 → 254823，新增 **1230 条**。
- 权重 6000 / 5500 / 5000：0 / 1230 / 0 条。
- 最终 SHA-256：`3EA62971C6518A543309D0FE42D8E60579E50CE77F89234222655EE01BBC5E25`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

音乐与乐器排练、跟拍学舞和舞台排练、画画练字与笔墨交流均是日常兴趣活动；对应的聊天请求、排练安排和创作交流是常见整词输入。每条逐项自拟并按语境复核。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 514 | 练琴合奏与排练交流聊天 | curate | 331 | 0 | 331 | 0 | 55.89% | 100.00% | `01248fc91ed54115e269ebc33bca1d07a5cd56d4` |
| 515 | 跟拍学舞与排练沟通聊天 | curate | 488 | 0 | 488 | 0 | 65.78% | 100.00% | `fc3675bf5f6e96db45f2856eb1e3ff2da7f0a47d` |
| 516 | 画画练字与笔墨交流聊天 | curate | 381 | 0 | 381 | 0 | 57.74% | 100.00% | `aa677ad71cb0bbc56bad1fcc35cd1563d42ddb2f` |
| 517 | 练琴合奏与排练较长说法 | merge | 10 | 0 | 10 | 0 | 60.00% | 70.00% | `03cf59dfe10a7ab2b54b7ba80e7dd2b3a2171944` |
| 518 | 跟拍学舞与排练较长说法 | merge | 10 | 0 | 10 | 0 | 80.00% | 90.00% | `67fb6626247046aeb5c5ba84c05517629b9703f0` |
| 519 | 画画练字与笔墨交流较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `2f9b19b73b053b2f3d9bf706c8b8e18d37cab545` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round49_exclusions.tsv`，共 336 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality: clipped, unidiomatic or unclear chat wording; individually authored replacement added where suitable`：30 条。
- `duplicate:local_core_zh_pinyin.tsv`：37 条。
- `outside_batch_length_scope`：257 条。
- `quality: clipped or unidiomatic chat fragment; replaced with a natural conversational form`：7 条。
- `duplicate:batch514`：3 条。
- `duplicate:batch515`：2 条。



同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1081 个含多音字的词在各批复核表逐字记录；64 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐字复核弹（演奏 tan）、调（调整 tiao；曲调 diao）、行（行列 hang；行吗 xing）、重（重复 chong；分量 zhong；重拍 zhong）、长（长度 chang）、还（归还 huan；仍然 hai）、落（落下 la；落灰 luo）、得（必须 dei；补语 de）、着（着急 zhao；持续 zhe）、乐（音乐 yue）。其他同字词逐字按当前短语语境记录；轻声按本字。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 26/26 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，254823 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round49_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.23% → 88.23% | +0.00 | 98.65% → 98.65% |
| 逐词输入·无前文 | 20626 | 79.64% → 79.64% | +0.00 | 97.53% → 97.53% |
| 留出整句 | 4126 | 64.44% → 64.44% | +0.00 | 77.70% → 77.70% |
| 口语对话·整句 | 3368 | 84.80% → 84.80% | +0.00 | 94.24% → 94.24% |
| 口语对话·逐词 | 19504 | 95.50% → 95.50% | +0.00 | 99.63% → 99.63% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.00% → 96.00% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round49.md`
- `resources/dictionary/expansion/batch514_authored_draft.txt`
- `resources/dictionary/expansion/batch514_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch515_authored_draft.txt`
- `resources/dictionary/expansion/batch515_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch516_authored_draft.txt`
- `resources/dictionary/expansion/batch516_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch517_draft.txt`
- `resources/dictionary/expansion/batch517_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch518_draft.txt`
- `resources/dictionary/expansion/batch518_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch519_draft.txt`
- `resources/dictionary/expansion/batch519_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_514.md`
- `resources/dictionary/expansion/batch_515.md`
- `resources/dictionary/expansion/batch_516.md`
- `resources/dictionary/expansion/batch_517.md`
- `resources/dictionary/expansion/batch_518.md`
- `resources/dictionary/expansion/batch_519.md`
- `resources/dictionary/expansion/lexicon_batch_514.applied.json`
- `resources/dictionary/expansion/lexicon_batch_514.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_514.json`
- `resources/dictionary/expansion/lexicon_batch_514.tsv`
- `resources/dictionary/expansion/lexicon_batch_514.validation.json`
- `resources/dictionary/expansion/lexicon_batch_515.applied.json`
- `resources/dictionary/expansion/lexicon_batch_515.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_515.json`
- `resources/dictionary/expansion/lexicon_batch_515.tsv`
- `resources/dictionary/expansion/lexicon_batch_515.validation.json`
- `resources/dictionary/expansion/lexicon_batch_516.applied.json`
- `resources/dictionary/expansion/lexicon_batch_516.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_516.json`
- `resources/dictionary/expansion/lexicon_batch_516.tsv`
- `resources/dictionary/expansion/lexicon_batch_516.validation.json`
- `resources/dictionary/expansion/lexicon_batch_517.json`
- `resources/dictionary/expansion/lexicon_batch_517.tsv`
- `resources/dictionary/expansion/lexicon_batch_518.json`
- `resources/dictionary/expansion/lexicon_batch_518.tsv`
- `resources/dictionary/expansion/lexicon_batch_519.json`
- `resources/dictionary/expansion/lexicon_batch_519.tsv`
- `resources/dictionary/expansion/probes/batch514_probe.tsv`
- `resources/dictionary/expansion/probes/batch515_probe.tsv`
- `resources/dictionary/expansion/probes/batch516_probe.tsv`
- `resources/dictionary/expansion/probes/batch517_probe.tsv`
- `resources/dictionary/expansion/probes/batch518_probe.tsv`
- `resources/dictionary/expansion/probes/batch519_probe.tsv`
- `resources/dictionary/expansion/round49_exclusions.tsv`
- `resources/dictionary/expansion/round49_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
