# 词库续加第66轮（批次 618–623）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `ffbc21888fb7fee2cab98368467271092b95c2fd`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：273354 → 274495，新增 **1141 条**。
- 权重 6000 / 5500 / 5000：0 / 1141 / 0 条。
- 最终 SHA-256：`3C1EEF2B5F82477403716B8248AFD7C9B83F02E2231F65CBD8A02606E29D5793`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 618 | 合租与共享居住空间的日常协调 | curate | 427 | 0 | 427 | 0 | 70.26% | 100.00% | `2b18b5ec0d37a45fd9bcc53b038005ef08f5b12c` |
| 619 | 亲密关系中的日常关心与报平安 | curate | 333 | 0 | 333 | 0 | 88.29% | 100.00% | `ff8d7c2d9c31d1728589f38678c3f80c3a91ea02` |
| 620 | 朋友之间共同开销、代付与结算聊天 | curate | 336 | 0 | 336 | 0 | 70.83% | 100.00% | `2abd3d7949564538a5bc93892615673f8a1c8a12` |
| 621 | 合租与共享居住协调的长句 | merge | 15 | 0 | 15 | 0 | 80.00% | 100.00% | `687b7bc3f9804c8dc952212a06f569f0e472577a` |
| 622 | 亲密关系日常关心与报平安的长句 | merge | 15 | 0 | 15 | 0 | 80.00% | 93.33% | `f85d2e501f283fbc3d57b660ec0ebafcfc32a43b` |
| 623 | 朋友之间共同开销结算的长句 | merge | 15 | 0 | 15 | 0 | 86.67% | 93.33% | `9ebfe2c330fde4956d383f405264fbf1b99690fe` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round66_exclusions.tsv`，共 714 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：611 条。
- `duplicate:local_core_zh_pinyin.tsv`：71 条。
- `指代不清，未收录`：1 条。
- `表达略生硬，改用‘这格大家共用’`：1 条。
- `词语搭配不自然，改用‘打扫改到周日’`：1 条。
- `搭配不自然，未收录`：2 条。
- `表达不自然，改用‘你说我会听’`：1 条。
- `表达不完整，未收录`：4 条。
- `表达略生硬，改用‘我懂你的感受’`：1 条。
- `duplicate:batch619`：1 条。
- `表达不自然，未收录`：2 条。
- `表达略生硬，改用‘先记在我这边’`：1 条。
- `表达不自然，改用‘票款我来收’`：1 条。
- `表达残缺，改用‘看看金额那行’`：1 条。
- `表达不完整，改用‘小钱不用还’`：1 条。
- `偏工作报销用语，超出朋友间共同开销主题`：10 条。
- `表达不完整，改用‘票我已经买好’`：1 条。
- `duplicate:batch620`：2 条。
- `朋友日常语境下搭配不自然，改用‘公用钱还够吗’`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1037 个含多音字的词在各批复核表逐字记录；9 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐字复核含多音字词项；归还义的‘还’读 huan，调节音量或温度的‘调’读 tiao，‘金额那行’的‘行’读 hang。着凉和着急按 zhao，其他常见持续状态按 zhe。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1/1 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，274495 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round66_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round66.md`
- `resources/dictionary/expansion/batch618_authored_draft.txt`
- `resources/dictionary/expansion/batch618_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch619_authored_draft.txt`
- `resources/dictionary/expansion/batch619_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch620_authored_draft.txt`
- `resources/dictionary/expansion/batch620_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch621_draft.txt`
- `resources/dictionary/expansion/batch621_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch622_draft.txt`
- `resources/dictionary/expansion/batch622_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch623_draft.txt`
- `resources/dictionary/expansion/batch623_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_618.md`
- `resources/dictionary/expansion/batch_619.md`
- `resources/dictionary/expansion/batch_620.md`
- `resources/dictionary/expansion/batch_621.md`
- `resources/dictionary/expansion/batch_622.md`
- `resources/dictionary/expansion/batch_623.md`
- `resources/dictionary/expansion/lexicon_batch_618.applied.json`
- `resources/dictionary/expansion/lexicon_batch_618.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_618.json`
- `resources/dictionary/expansion/lexicon_batch_618.tsv`
- `resources/dictionary/expansion/lexicon_batch_618.validation.json`
- `resources/dictionary/expansion/lexicon_batch_619.applied.json`
- `resources/dictionary/expansion/lexicon_batch_619.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_619.json`
- `resources/dictionary/expansion/lexicon_batch_619.tsv`
- `resources/dictionary/expansion/lexicon_batch_619.validation.json`
- `resources/dictionary/expansion/lexicon_batch_620.applied.json`
- `resources/dictionary/expansion/lexicon_batch_620.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_620.json`
- `resources/dictionary/expansion/lexicon_batch_620.tsv`
- `resources/dictionary/expansion/lexicon_batch_620.validation.json`
- `resources/dictionary/expansion/lexicon_batch_621.json`
- `resources/dictionary/expansion/lexicon_batch_621.tsv`
- `resources/dictionary/expansion/lexicon_batch_622.json`
- `resources/dictionary/expansion/lexicon_batch_622.tsv`
- `resources/dictionary/expansion/lexicon_batch_623.json`
- `resources/dictionary/expansion/lexicon_batch_623.tsv`
- `resources/dictionary/expansion/probes/batch618_probe.tsv`
- `resources/dictionary/expansion/probes/batch619_probe.tsv`
- `resources/dictionary/expansion/probes/batch620_probe.tsv`
- `resources/dictionary/expansion/probes/batch621_probe.tsv`
- `resources/dictionary/expansion/probes/batch622_probe.tsv`
- `resources/dictionary/expansion/probes/batch623_probe.tsv`
- `resources/dictionary/expansion/round66_exclusions.tsv`
- `resources/dictionary/expansion/round66_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
