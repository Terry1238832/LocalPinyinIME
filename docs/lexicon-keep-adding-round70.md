# 词库续加第70轮（批次 646–653）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `dabc3c14994f56d496cce71856f2a11c204331a9`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：278596 → 280356，新增 **1760 条**。
- 权重 6000 / 5500 / 5000：0 / 1760 / 0 条。
- 最终 SHA-256：`4F7D098C0E831B5706229761CC85B256E9CD1A2F3FEB094187A6A3FD81DA3E08`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

四个方向覆盖日常聊天中常见的宠物照看、约球运动、阳台花草和天气穿衣安排；这些情境常需整段发送，词面逐条自拟并按历史词库去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 646 | 日常聊天：宠物近况、照看托付与日常互动 | curate | 376 | 0 | 376 | 0 | 48.14% | 100.00% | `e2199892479b1bdca04cf7759e1995feb5ae0b3d` |
| 647 | 日常聊天：打球约场、运动训练与赛后交流 | curate | 409 | 0 | 409 | 0 | 68.46% | 100.00% | `793e192d313ff0a06f0b4cf3bfe3fdb3c7a7d77a` |
| 648 | 日常聊天：阳台花草、日常养护与交换枝苗 | curate | 396 | 0 | 396 | 0 | 59.60% | 100.00% | `5676897d7a252879f090f14d27b9d65e04bbb92f` |
| 649 | 日常聊天：天气变化、穿衣提醒与出门安排 | curate | 539 | 0 | 539 | 0 | 69.20% | 100.00% | `9201dab85006c6433a6fe1f51e64236e8f9ea160` |
| 650 | 日常聊天：宠物近况、临时照看与交接安排的长句 | merge | 10 | 0 | 10 | 0 | 40.00% | 50.00% | `0e068399244c8131fef27170f5c7f9026c28d9cc` |
| 651 | 日常聊天：约球、分队与赛后交流的长句 | merge | 10 | 0 | 10 | 0 | 80.00% | 80.00% | `f0a23105e56fcc388ec0fda9e157b5a481170aa8` |
| 652 | 日常聊天：阳台花草、养护经验与交换枝苗的长句 | merge | 10 | 0 | 10 | 0 | 60.00% | 60.00% | `250234b0f8abf1c3f64598e748e8d41f9da8cd53` |
| 653 | 日常聊天：天气变化、穿衣出门与到家提醒的长句 | merge | 10 | 0 | 10 | 0 | 90.00% | 90.00% | `83759ed204d8d49cc9311c7f487d94c58b9ee4b3` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round70_exclusions.tsv`，共 168 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：105 条。
- `表达不完整，改为猫咪爱躲起来`：1 条。
- `语序不自然，改为它掉毛不少`：1 条。
- `主语不清，改为它今天挺温顺`：1 条。
- `表达不完整，改为外驱滴在后颈`：1 条。
- `表达不完整，改为按体重喂药`：1 条。
- `缺少结构助词，改为听到你的声音`：1 条。
- `表达不完整，改为猫咪挤在中间`：1 条。
- `量词后缺少中心语，改为小狗真活泼`：1 条。
- `搭配生硬，改为有熟悉的味道`：1 条。
- `术语使用不自然，删除`：1 条。
- `outside_batch_length_scope`：32 条。
- `表达不自然，改为轮流打一局`：1 条。
- `表达不完整，改为再来一个好球`：1 条。
- `搭配不自然，改为谁接先喊一声`：1 条。
- `表达不完整，改为会员有折扣`：1 条。
- `时间单位缺失，删除`：1 条。
- `搭配不完整，改为今天练得轻松`：1 条。
- `说法不自然，改为黄色粘虫板`：1 条。
- `表达不完整，改为花盆别摔坏`：1 条。
- `duplicate:batch648`：2 条。
- `表达不自然，改为上午能晒到`：1 条。
- `duplicate:batch646`：1 条。
- `表达生硬，删除`：1 条。
- `搭配不自然，改为这枝状态不错`：1 条。
- `表达不完整，改为出门加件衣服`：1 条。
- `口语搭配不自然，改为我这儿多一把`：1 条。
- `表达不完整，改为阳台东西收好`：1 条。
- `语义不当，改为老人慢点走`：1 条。
- `说法不自然，改为里面加件外套`：1 条。
- `表达不完整，改为早晚出门冷`：1 条。
- `duplicate:batch649`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1535 个含多音字的词在各批复核表逐字记录；17 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

本轮含多音字的短语和长句逐字核对复核表；按宠物照料、运动约练、花草养护及天气穿衣的具体语境选择读音。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 24/24 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，280356 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round70_validation.json`；短词批次另存工具生成的 `.validation.json`。
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

- `docs/lexicon-keep-adding-round70.md`
- `resources/dictionary/expansion/batch646_authored_draft.txt`
- `resources/dictionary/expansion/batch646_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch647_authored_draft.txt`
- `resources/dictionary/expansion/batch647_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch648_authored_draft.txt`
- `resources/dictionary/expansion/batch648_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch649_authored_draft.txt`
- `resources/dictionary/expansion/batch649_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch650_draft.txt`
- `resources/dictionary/expansion/batch650_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch651_draft.txt`
- `resources/dictionary/expansion/batch651_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch652_draft.txt`
- `resources/dictionary/expansion/batch652_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch653_draft.txt`
- `resources/dictionary/expansion/batch653_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_646.md`
- `resources/dictionary/expansion/batch_647.md`
- `resources/dictionary/expansion/batch_648.md`
- `resources/dictionary/expansion/batch_649.md`
- `resources/dictionary/expansion/batch_650.md`
- `resources/dictionary/expansion/batch_651.md`
- `resources/dictionary/expansion/batch_652.md`
- `resources/dictionary/expansion/batch_653.md`
- `resources/dictionary/expansion/lexicon_batch_646.applied.json`
- `resources/dictionary/expansion/lexicon_batch_646.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_646.json`
- `resources/dictionary/expansion/lexicon_batch_646.tsv`
- `resources/dictionary/expansion/lexicon_batch_646.validation.json`
- `resources/dictionary/expansion/lexicon_batch_647.applied.json`
- `resources/dictionary/expansion/lexicon_batch_647.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_647.json`
- `resources/dictionary/expansion/lexicon_batch_647.tsv`
- `resources/dictionary/expansion/lexicon_batch_647.validation.json`
- `resources/dictionary/expansion/lexicon_batch_648.applied.json`
- `resources/dictionary/expansion/lexicon_batch_648.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_648.json`
- `resources/dictionary/expansion/lexicon_batch_648.tsv`
- `resources/dictionary/expansion/lexicon_batch_648.validation.json`
- `resources/dictionary/expansion/lexicon_batch_649.applied.json`
- `resources/dictionary/expansion/lexicon_batch_649.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_649.json`
- `resources/dictionary/expansion/lexicon_batch_649.tsv`
- `resources/dictionary/expansion/lexicon_batch_649.validation.json`
- `resources/dictionary/expansion/lexicon_batch_650.json`
- `resources/dictionary/expansion/lexicon_batch_650.tsv`
- `resources/dictionary/expansion/lexicon_batch_651.json`
- `resources/dictionary/expansion/lexicon_batch_651.tsv`
- `resources/dictionary/expansion/lexicon_batch_652.json`
- `resources/dictionary/expansion/lexicon_batch_652.tsv`
- `resources/dictionary/expansion/lexicon_batch_653.json`
- `resources/dictionary/expansion/lexicon_batch_653.tsv`
- `resources/dictionary/expansion/probes/batch646_probe.tsv`
- `resources/dictionary/expansion/probes/batch647_probe.tsv`
- `resources/dictionary/expansion/probes/batch648_probe.tsv`
- `resources/dictionary/expansion/probes/batch649_probe.tsv`
- `resources/dictionary/expansion/probes/batch650_probe.tsv`
- `resources/dictionary/expansion/probes/batch651_probe.tsv`
- `resources/dictionary/expansion/probes/batch652_probe.tsv`
- `resources/dictionary/expansion/probes/batch653_probe.tsv`
- `resources/dictionary/expansion/round70_exclusions.tsv`
- `resources/dictionary/expansion/round70_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
