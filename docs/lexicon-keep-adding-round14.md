# 词库续加第14轮（批次 270–277）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `c93841675c5991996f0dfd8e45a7144c026b745b`，引擎版本 1.0.0。分支通过合并吸收 main，不 rebase、不 force push。持续 PR 的总表从第十轮起累计。

- `local_core`：210614 → 212502，新增 **1888 条**。
- 权重 6000 / 5500 / 5000：0 / 1888 / 0 条。
- 最终 SHA-256：`08F027F806461EAE65B37CB324D3E5504C1E22A671C7846AA0B4BD19F7061DF1`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 270 | 自媒体运营 | curate | 483 | 0 | 483 | 0 | 97.93% | 100.00% | `558a9ead3b026dcbe7717c17e33ef757f6ffa4cf` |
| 271 | 表格与办公软件操作 | curate | 325 | 0 | 325 | 0 | 89.23% | 100.00% | `ef671acec3b1fe8ce299724f95d801b8765ae64b` |
| 272 | 财务会计日常 | curate | 500 | 0 | 500 | 0 | 90.00% | 100.00% | `2fdab76961d3e561abc8377a67fa0ccd8fe8f1be` |
| 273 | 物流仓储 | curate | 500 | 0 | 500 | 0 | 95.60% | 100.00% | `9c89286e89a6f4ea4e3d30b95d4a8b921ad215d7` |
| 274 | 自媒体运营较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `bcd03547e5665ea13deee8a9df2df971fe704306` |
| 275 | 表格与办公软件操作较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `f4b37b5cfda49b5305e11748b311ec270ab3c4a3` |
| 276 | 财务会计日常较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `a5d82a26761e4fb2ff78bbb5a682ecde38df9922` |
| 277 | 物流仓储较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `1356d0333c7f633e17ab70327a53fdcbfa0f4da9` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round14_exclusions.tsv`，共 735 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：533 条。
- `quality:incomplete_or_uncommon_label`：33 条。
- `duplicate:standard_zh_pinyin.tsv`：21 条。
- `quality:uncertain_or_uncommon_label`：18 条。
- `duplicate:batch270`：1 条。
- `outside_batch_length_scope`：4 条。
- `batch_size_limit`：125 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1340 个含多音字的词在各批复核表逐字记录；30 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 自媒体：内容、图片、视频查重的重 `chong`；长期、长度、长图文的长 `chang`，粉丝增长的长 `zhang`。站外传播的传 `chuan`，转载的载 `zai`。品牌调性的调 `diao`，选题调研的调 `diao`。涨粉策略用 `celve`。
- 表格：行高、行标签、重复行、空白行的行均 `hang`；转文本、转图片和旋转文字的转 `zhuan`。去重、查重、重命名、重排和重置用重 `chong`。会计格式的会 `kuai`，模板的模 `mu`，粘贴的粘 `zhan`，奇偶页的奇 `ji`。
- 会计：会计的会以及税会差异中作为会计简称的会均 `kuai`；重置成本、科目重分类、报表重分类的重 `chong`。现金长款、长期待摊的长 `chang`；银行的行 `hang`；勾稽的稽 `ji`，折旧的折 `zhe`。
- 仓储：货物绑扎的扎 `za`；重新盘点的重盘用重 `chong`，重型、重货、承重、称重的重 `zhong`。调拨的调 `diao`、调整的调 `tiao`；冷藏的藏 `cang`；拣货策略用 `celve`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1524/1524 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，212502 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round14_validation.json`；短词批次另存工具生成的 `.validation.json`。
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

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round14.md`
- `resources/dictionary/expansion/batch270_authored_draft.txt`
- `resources/dictionary/expansion/batch270_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch271_authored_draft.txt`
- `resources/dictionary/expansion/batch271_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch272_authored_draft.txt`
- `resources/dictionary/expansion/batch272_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch273_authored_draft.txt`
- `resources/dictionary/expansion/batch273_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch274_draft.txt`
- `resources/dictionary/expansion/batch274_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch275_draft.txt`
- `resources/dictionary/expansion/batch275_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch276_draft.txt`
- `resources/dictionary/expansion/batch276_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch277_draft.txt`
- `resources/dictionary/expansion/batch277_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_270.md`
- `resources/dictionary/expansion/batch_271.md`
- `resources/dictionary/expansion/batch_272.md`
- `resources/dictionary/expansion/batch_273.md`
- `resources/dictionary/expansion/batch_274.md`
- `resources/dictionary/expansion/batch_275.md`
- `resources/dictionary/expansion/batch_276.md`
- `resources/dictionary/expansion/batch_277.md`
- `resources/dictionary/expansion/lexicon_batch_270.applied.json`
- `resources/dictionary/expansion/lexicon_batch_270.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_270.json`
- `resources/dictionary/expansion/lexicon_batch_270.tsv`
- `resources/dictionary/expansion/lexicon_batch_270.validation.json`
- `resources/dictionary/expansion/lexicon_batch_271.applied.json`
- `resources/dictionary/expansion/lexicon_batch_271.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_271.json`
- `resources/dictionary/expansion/lexicon_batch_271.tsv`
- `resources/dictionary/expansion/lexicon_batch_271.validation.json`
- `resources/dictionary/expansion/lexicon_batch_272.applied.json`
- `resources/dictionary/expansion/lexicon_batch_272.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_272.json`
- `resources/dictionary/expansion/lexicon_batch_272.tsv`
- `resources/dictionary/expansion/lexicon_batch_272.validation.json`
- `resources/dictionary/expansion/lexicon_batch_273.applied.json`
- `resources/dictionary/expansion/lexicon_batch_273.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_273.json`
- `resources/dictionary/expansion/lexicon_batch_273.tsv`
- `resources/dictionary/expansion/lexicon_batch_273.validation.json`
- `resources/dictionary/expansion/lexicon_batch_274.json`
- `resources/dictionary/expansion/lexicon_batch_274.tsv`
- `resources/dictionary/expansion/lexicon_batch_275.json`
- `resources/dictionary/expansion/lexicon_batch_275.tsv`
- `resources/dictionary/expansion/lexicon_batch_276.json`
- `resources/dictionary/expansion/lexicon_batch_276.tsv`
- `resources/dictionary/expansion/lexicon_batch_277.json`
- `resources/dictionary/expansion/lexicon_batch_277.tsv`
- `resources/dictionary/expansion/probes/batch270_probe.tsv`
- `resources/dictionary/expansion/probes/batch271_probe.tsv`
- `resources/dictionary/expansion/probes/batch272_probe.tsv`
- `resources/dictionary/expansion/probes/batch273_probe.tsv`
- `resources/dictionary/expansion/probes/batch274_probe.tsv`
- `resources/dictionary/expansion/probes/batch275_probe.tsv`
- `resources/dictionary/expansion/probes/batch276_probe.tsv`
- `resources/dictionary/expansion/probes/batch277_probe.tsv`
- `resources/dictionary/expansion/round14_exclusions.tsv`
- `resources/dictionary/expansion/round14_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
