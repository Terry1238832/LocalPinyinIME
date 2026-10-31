# 词库续加第64轮（批次 606–611）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `158faf6afa13235a7b88a233a621ce4045d06093`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：270895 → 271954，新增 **1059 条**。
- 权重 6000 / 5500 / 5000：0 / 1059 / 0 条。
- 最终 SHA-256：`5CF9F13CF900558EB3EE7D80694307389FC4092C8C3C8602AA11F92F0021FB93`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 606 | 借用物品、归还东西与顺手帮带 | curate | 325 | 0 | 325 | 0 | 78.46% | 100.00% | `ea035fe5f63f422d975fe032bf4dbdfa8e6459c6` |
| 607 | 分享好消息、进展与祝贺回应 | curate | 337 | 0 | 337 | 0 | 79.53% | 100.00% | `dabef1779272405451abce1b9c39a3d62fe96f01` |
| 608 | 追剧听歌、观影和演出后的聊天 | curate | 352 | 0 | 352 | 0 | 77.27% | 100.00% | `8fab65def742b11eadfc1572b09bd8c1b10d72bf` |
| 609 | 借用物品、归还东西与顺手帮带的长句 | merge | 15 | 0 | 15 | 0 | 80.00% | 93.33% | `4de4605c34a8081432372024807678c3283c314f` |
| 610 | 分享好消息、进展与祝贺回应的长句 | merge | 15 | 0 | 15 | 0 | 86.67% | 93.33% | `db57f264b3e17d50d42ff685db9c7530b0bf2a41` |
| 611 | 追剧听歌、观影和演出后的长句闲聊 | merge | 15 | 0 | 15 | 0 | 53.33% | 60.00% | `f35efe557c03b5750cde663ac1d6be27b18d9bd5` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round64_exclusions.tsv`，共 723 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：71 条。
- `outside_batch_length_scope`：643 条。
- `duplicate:batch606`：1 条。
- `借用语境搭配不自然`：1 条。
- `残缺表达，不作为词条`：1 条。
- `还清通常搭配债务，不用于归还物品`：1 条。
- `duplicate:batch608`：1 条。
- `non_BMP_Han_or_non_Han`：1 条。
- `duplicate:batch607`：2 条。
- `搭配不自然，不作为常用聊天说法`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

975 个含多音字的词在各批复核表逐字记录；32 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

关键读音按语境复核：归还/还给/还我/还你中的‘还’读huan，表示仍然、还要的‘还’读hai；‘还没过安检’里的‘没’读mei；‘前几首都好听’里的‘都’读dou；‘有了结果’和‘批下来了’里的‘了’读le。兼有两种语境的长句按字位区分（如‘暂时还要……一定还给你’）。‘着急’读zhao，‘借你先用着’读zhe。逐字选择见批次复核表。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，271954 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round64_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round64.md`
- `resources/dictionary/expansion/batch606_authored_draft.txt`
- `resources/dictionary/expansion/batch606_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch607_authored_draft.txt`
- `resources/dictionary/expansion/batch607_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch608_authored_draft.txt`
- `resources/dictionary/expansion/batch608_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch609_draft.txt`
- `resources/dictionary/expansion/batch609_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch610_draft.txt`
- `resources/dictionary/expansion/batch610_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch611_draft.txt`
- `resources/dictionary/expansion/batch611_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_606.md`
- `resources/dictionary/expansion/batch_607.md`
- `resources/dictionary/expansion/batch_608.md`
- `resources/dictionary/expansion/batch_609.md`
- `resources/dictionary/expansion/batch_610.md`
- `resources/dictionary/expansion/batch_611.md`
- `resources/dictionary/expansion/lexicon_batch_606.applied.json`
- `resources/dictionary/expansion/lexicon_batch_606.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_606.json`
- `resources/dictionary/expansion/lexicon_batch_606.tsv`
- `resources/dictionary/expansion/lexicon_batch_606.validation.json`
- `resources/dictionary/expansion/lexicon_batch_607.applied.json`
- `resources/dictionary/expansion/lexicon_batch_607.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_607.json`
- `resources/dictionary/expansion/lexicon_batch_607.tsv`
- `resources/dictionary/expansion/lexicon_batch_607.validation.json`
- `resources/dictionary/expansion/lexicon_batch_608.applied.json`
- `resources/dictionary/expansion/lexicon_batch_608.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_608.json`
- `resources/dictionary/expansion/lexicon_batch_608.tsv`
- `resources/dictionary/expansion/lexicon_batch_608.validation.json`
- `resources/dictionary/expansion/lexicon_batch_609.json`
- `resources/dictionary/expansion/lexicon_batch_609.tsv`
- `resources/dictionary/expansion/lexicon_batch_610.json`
- `resources/dictionary/expansion/lexicon_batch_610.tsv`
- `resources/dictionary/expansion/lexicon_batch_611.json`
- `resources/dictionary/expansion/lexicon_batch_611.tsv`
- `resources/dictionary/expansion/probes/batch606_probe.tsv`
- `resources/dictionary/expansion/probes/batch607_probe.tsv`
- `resources/dictionary/expansion/probes/batch608_probe.tsv`
- `resources/dictionary/expansion/probes/batch609_probe.tsv`
- `resources/dictionary/expansion/probes/batch610_probe.tsv`
- `resources/dictionary/expansion/probes/batch611_probe.tsv`
- `resources/dictionary/expansion/round64_exclusions.tsv`
- `resources/dictionary/expansion/round64_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
