# 词库续加第52轮（批次 532–537）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `89642069c61d5c814991e53c82b3e193fd974966`，引擎版本 1.8.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：256888 → 257922，新增 **1034 条**。
- 权重 6000 / 5500 / 5000：0 / 1034 / 0 条。
- 最终 SHA-256：`9517EFE50381E44E0101E3E23C1127C270570EAC3C8FFBFB1F63CEDDECF5259A`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮按聊天说法类型补充自然夸赞与认可、温和提醒与进度跟进、犹豫保留与婉转改口；短词为可独立输入的固定表达，长批为常见完整沟通句。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 532 | 夸赞与表达认可聊天 | curate | 374 | 0 | 374 | 0 | 72.46% | 100.00% | `759935f5e560f8a8c59e35da572ab8ba5f2882fa` |
| 533 | 温和提醒与进度跟进聊天 | curate | 321 | 0 | 321 | 0 | 81.00% | 100.00% | `c404b1e7a50fa2ddb44dd9aeadab9434736b1fd3` |
| 534 | 犹豫保留与婉转改口聊天 | curate | 309 | 0 | 309 | 0 | 82.52% | 100.00% | `941f1e5561f7ae2113fae95c3446cd7f156262db` |
| 535 | 夸赞与表达认可较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `b0cb997bf3ba937f47ba2a3c45bd6f46610e4538` |
| 536 | 温和提醒与进度跟进较长说法 | merge | 10 | 0 | 10 | 0 | 70.00% | 90.00% | `f257e29749ec66bffcdc8f278f2365a2c567c0be` |
| 537 | 犹豫保留与婉转改口较长说法 | merge | 10 | 0 | 10 | 0 | 80.00% | 80.00% | `11fd6a1b551a75a4a24ca7711bfc93dcb951d2ab` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round52_exclusions.tsv`，共 267 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：79 条。
- `quality: ungrammatical; the expression needs a subject before 用心`：1 条。
- `quality: unnatural collocation; 耐心 does not fit this complement`：1 条。
- `quality: context-dependent and not a usual standalone chat phrase`：1 条。
- `quality: awkward wording; common form would require an object or different syntax`：1 条。
- `outside_batch_length_scope`：148 条。
- `quality: unclear without context and not a common standalone compliment`：1 条。
- `quality: incomplete without the praise or responsibility being borne`：1 条。
- `quality: ungrammatical phrase`：1 条。
- `quality: generic clause rather than an established compliment`：1 条。
- `quality: unnatural wording; not a common way to express appreciation`：1 条。
- `quality: unclear object; not a natural standalone chat phrase`：1 条。
- `quality: unnatural collocation; 提得有价值 is not usual chat phrasing`：1 条。
- `quality: unclear without a specific editing context`：1 条。
- `quality: awkward word order`：1 条。
- `quality: clipped and unnatural standalone compliment`：1 条。
- `quality: awkward word order; incomplete as a standalone phrase`：1 条。
- `quality: generic event description, not a compliment or recognition phrase`：1 条。
- `quality: unnatural wording; common phrasing is 让你操心了`：1 条。
- `quality: awkward redundant wording`：1 条。
- `quality: unnatural collocation`：2 条。
- `quality: incomplete and unnatural collocation`：1 条。
- `quality: incomplete expression; the object of 值得 is missing`：1 条。
- `quality: clipped and context-dependent`：1 条。
- `quality: dated or regional wording; replaced by common message wording elsewhere`：1 条。
- `quality: dialectal question form`：1 条。
- `quality: unnatural wording for meeting at an entrance`：1 条。
- `quality: regional or nonstandard wording`：1 条。
- `quality: unclear wording`：1 条。
- `quality: awkward wording and not a natural standalone reminder`：1 条。
- `quality: ungrammatical word order`：1 条。
- `quality: stiff formal wording, not daily chat`：1 条。
- `quality: unclear reference and not a natural standalone phrase`：1 条。
- `quality: clipped and unnatural wording`：1 条。
- `quality: unnatural wording`：1 条。
- `duplicate:batch534`：1 条。
- `quality: stiff wording for daily chat`：1 条。
- `quality: awkward duration wording`：1 条。
- `quality: vague and incomplete without context`：1 条。
- `quality: outside the tentative-decision direction`：1 条。
- `quality: overly formal for this daily chat batch`：1 条。



同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

945 个含多音字的词在各批复核表逐字记录；3 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

夸赞、提醒和婉转表态中的多音字逐字按完整短语复核；长、还、着、得、重、便、乐、行、调、和等按语境判断并在复核表记录。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 40/40 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，257922 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round52_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round52.md`
- `resources/dictionary/expansion/batch532_authored_draft.txt`
- `resources/dictionary/expansion/batch532_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch533_authored_draft.txt`
- `resources/dictionary/expansion/batch533_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch534_authored_draft.txt`
- `resources/dictionary/expansion/batch534_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch535_draft.txt`
- `resources/dictionary/expansion/batch535_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch536_draft.txt`
- `resources/dictionary/expansion/batch536_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch537_draft.txt`
- `resources/dictionary/expansion/batch537_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_532.md`
- `resources/dictionary/expansion/batch_533.md`
- `resources/dictionary/expansion/batch_534.md`
- `resources/dictionary/expansion/batch_535.md`
- `resources/dictionary/expansion/batch_536.md`
- `resources/dictionary/expansion/batch_537.md`
- `resources/dictionary/expansion/lexicon_batch_532.applied.json`
- `resources/dictionary/expansion/lexicon_batch_532.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_532.json`
- `resources/dictionary/expansion/lexicon_batch_532.tsv`
- `resources/dictionary/expansion/lexicon_batch_532.validation.json`
- `resources/dictionary/expansion/lexicon_batch_533.applied.json`
- `resources/dictionary/expansion/lexicon_batch_533.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_533.json`
- `resources/dictionary/expansion/lexicon_batch_533.tsv`
- `resources/dictionary/expansion/lexicon_batch_533.validation.json`
- `resources/dictionary/expansion/lexicon_batch_534.applied.json`
- `resources/dictionary/expansion/lexicon_batch_534.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_534.json`
- `resources/dictionary/expansion/lexicon_batch_534.tsv`
- `resources/dictionary/expansion/lexicon_batch_534.validation.json`
- `resources/dictionary/expansion/lexicon_batch_535.json`
- `resources/dictionary/expansion/lexicon_batch_535.tsv`
- `resources/dictionary/expansion/lexicon_batch_536.json`
- `resources/dictionary/expansion/lexicon_batch_536.tsv`
- `resources/dictionary/expansion/lexicon_batch_537.json`
- `resources/dictionary/expansion/lexicon_batch_537.tsv`
- `resources/dictionary/expansion/probes/batch532_probe.tsv`
- `resources/dictionary/expansion/probes/batch533_probe.tsv`
- `resources/dictionary/expansion/probes/batch534_probe.tsv`
- `resources/dictionary/expansion/probes/batch535_probe.tsv`
- `resources/dictionary/expansion/probes/batch536_probe.tsv`
- `resources/dictionary/expansion/probes/batch537_probe.tsv`
- `resources/dictionary/expansion/round52_exclusions.tsv`
- `resources/dictionary/expansion/round52_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
