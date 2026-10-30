# 词库续加第62轮（批次 594–599）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `fbb3aa18b412683405076aa8e477cafa1284fe88`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：268297 → 269741，新增 **1444 条**。
- 权重 6000 / 5500 / 5000：0 / 1444 / 0 条。
- 最终 SHA-256：`DC4C4AEF83A8132C24D912F35566A50434ACB39F4EBD42B3DAA6B33289F0B6C1`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 594 | 语音留言的录制、发送与收听 | curate | 378 | 0 | 378 | 0 | 81.48% | 100.00% | `cbab6e9ebe59bad577eb0f6e2c25b9ee3a6d85f2` |
| 595 | 群成员邀请、入群申请与退群沟通 | curate | 570 | 0 | 570 | 0 | 81.93% | 100.00% | `b78f2d2919055f404cabfba5b271e43881c8de88` |
| 596 | 聊天引用回复、接话与澄清上下文 | curate | 422 | 0 | 422 | 0 | 70.38% | 100.00% | `9de48a5e12bcc3a539767388248c45e1929f4c39` |
| 597 | 语音留言分段发送、收听与补充长句 | merge | 25 | 0 | 25 | 0 | 84.00% | 88.00% | `2a71fc08d5480159a0b79062cf94add7796e049f` |
| 598 | 邀请新成员、申请入群与整理群成员长句 | merge | 24 | 0 | 24 | 0 | 79.17% | 87.50% | `8bba94c5d99e07fcc23b577d15837c5b5bdec942` |
| 599 | 引用回复、补足上下文与澄清误解长句 | merge | 25 | 0 | 25 | 0 | 84.00% | 84.00% | `59fb8063f550e5faa5cfaaa0bd39912a80168431` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round62_exclusions.tsv`，共 631 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：67 条。
- `表达不完整，缺少量词‘一遍’`：1 条。
- `语序不自然，不能作为常见聊天短语`：1 条。
- `outside_batch_length_scope`：553 条。
- `表达不完整，应说找个安静的地方`：1 条。
- `重复赘词，表达不自然`：1 条。
- `duplicate:batch595`：2 条。
- `non_BMP_Han_or_non_Han`：1 条。
- `duplicate:batch596`：4 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1293 个含多音字的词在各批复核表逐字记录；19 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

本轮逐字语境决定和默认键差异均见各批 `pronunciation_review.tsv` 与批次 JSON。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 59/59 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，269741 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round62_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round62.md`
- `resources/dictionary/expansion/batch594_authored_draft.txt`
- `resources/dictionary/expansion/batch594_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch595_authored_draft.txt`
- `resources/dictionary/expansion/batch595_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch596_authored_draft.txt`
- `resources/dictionary/expansion/batch596_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch597_draft.txt`
- `resources/dictionary/expansion/batch597_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch598_draft.txt`
- `resources/dictionary/expansion/batch598_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch599_draft.txt`
- `resources/dictionary/expansion/batch599_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_594.md`
- `resources/dictionary/expansion/batch_595.md`
- `resources/dictionary/expansion/batch_596.md`
- `resources/dictionary/expansion/batch_597.md`
- `resources/dictionary/expansion/batch_598.md`
- `resources/dictionary/expansion/batch_599.md`
- `resources/dictionary/expansion/lexicon_batch_594.applied.json`
- `resources/dictionary/expansion/lexicon_batch_594.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_594.json`
- `resources/dictionary/expansion/lexicon_batch_594.tsv`
- `resources/dictionary/expansion/lexicon_batch_594.validation.json`
- `resources/dictionary/expansion/lexicon_batch_595.applied.json`
- `resources/dictionary/expansion/lexicon_batch_595.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_595.json`
- `resources/dictionary/expansion/lexicon_batch_595.tsv`
- `resources/dictionary/expansion/lexicon_batch_595.validation.json`
- `resources/dictionary/expansion/lexicon_batch_596.applied.json`
- `resources/dictionary/expansion/lexicon_batch_596.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_596.json`
- `resources/dictionary/expansion/lexicon_batch_596.tsv`
- `resources/dictionary/expansion/lexicon_batch_596.validation.json`
- `resources/dictionary/expansion/lexicon_batch_597.json`
- `resources/dictionary/expansion/lexicon_batch_597.tsv`
- `resources/dictionary/expansion/lexicon_batch_598.json`
- `resources/dictionary/expansion/lexicon_batch_598.tsv`
- `resources/dictionary/expansion/lexicon_batch_599.json`
- `resources/dictionary/expansion/lexicon_batch_599.tsv`
- `resources/dictionary/expansion/probes/batch594_probe.tsv`
- `resources/dictionary/expansion/probes/batch595_probe.tsv`
- `resources/dictionary/expansion/probes/batch596_probe.tsv`
- `resources/dictionary/expansion/probes/batch597_probe.tsv`
- `resources/dictionary/expansion/probes/batch598_probe.tsv`
- `resources/dictionary/expansion/probes/batch599_probe.tsv`
- `resources/dictionary/expansion/round62_exclusions.tsv`
- `resources/dictionary/expansion/round62_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
