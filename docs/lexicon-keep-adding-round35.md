# 词库续加第35轮（批次 420–427）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `3891b79ccad5ed1186d524009f6024971648b711`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

合并提交 `3891b79ccad5ed1186d524009f6024971648b711` 单独保留。合并 1.6.0 main 时保留其删除的 1,036 条单字行，条目数先从 237,251 降至 236,215，再开始本轮追加；没有恢复或新增单字。元数据由实际 merge_lexicon_batch.py 合并既有批次（新增 0）重新计算 entry_count、sha256、category_counts 和 length_distribution。合并后、加词前已完成 x64/x86 Release 构建及各 46/46 测试；词语搭配模型、神经重排模型及其测试源文件的 SHA-256 在加词前后保持一致。

- `local_core`：236215 → 238144，新增 **1929 条**。
- 权重 6000 / 5500 / 5000：0 / 1929 / 0 条。
- 最终 SHA-256：`4CE5C637A87C78F7E25D44FAB716B1404493CB3AB67060350A24C5E8AB5FD302`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。按2026-10-28最新要求，从第35轮改为优先补聊天时自然整段输入、容易拆分成同音字的常用搭配；不再沿用此前的300条阶段停止阈值，逐轮继续直到用户叫停。四个短词专题分别为消息联络、约见等候、互助小事、口头反应熟语，各配一批较长常用说法。全为逐条自拟，不从测试集或评测失败答案挑词，不导入第三方词库，不机械替换人称、时间、句尾或数字凑数。短草稿中超出6字的候选按长度排除；长批另行逐条拟写。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 420 | 聊天联络与消息往来 | curate | 431 | 0 | 431 | 0 | 83.06% | 100.00% | `27a3ad38deda7549ba73dd3c8dbae75d5a9e24fa` |
| 421 | 约见等候与临近出发 | curate | 432 | 0 | 432 | 0 | 78.70% | 100.00% | `1db83418354fb9e63a2cc002ffb49fe6da63e738` |
| 422 | 日常互助与临时小事口语 | curate | 427 | 0 | 427 | 0 | 74.94% | 100.00% | `f2b073d950322fc8ed05a2b416ebe12bd338817d` |
| 423 | 口头反应与反问熟语 | curate | 559 | 0 | 559 | 0 | 79.61% | 100.00% | `e040b1e988fd25006f283213ebdeef5474d22fbb` |
| 424 | 聊天联络与消息往来较长说法 | merge | 20 | 0 | 20 | 0 | 95.00% | 100.00% | `03290a03d09041f29e8bf3abef6f3454c1ae6e9e` |
| 425 | 约见等候与临近出发较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `47f1a56741581223ec34b7f918d963c45f7f94c7` |
| 426 | 日常互助与临时小事较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `c3eb95ee7403d73cace109810941d7a9630bb84b` |
| 427 | 口头反应与反问熟语较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `9d207f65d44097ebc5cd51514411c98559a6a67e` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round35_exclusions.tsv`，共 298 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：145 条。
- `quality: awkward ending, truncated clause or unnatural conversational word order`：49 条。
- `quality: unnecessary equivalent variation or uncertainty about stable everyday usage`：68 条。
- `outside_batch_length_scope`：10 条。
- `quality: elliptical reply with unclear or unnatural standalone meaning`：6 条。
- `quality: incomplete timing clause or weakly idiomatic help request`：7 条。
- `quality: incomplete clause or unnecessarily forced conversational ending`：13 条。

去重排除145条；按措辞、重复变体及稳定性排除143条，另按长短批范围排除10条。例如“我已经走到”“我来取的时候”缺少后续内容，“帮我签个收”“我备忘一下”不够自然，“你品你细品”不作为稳定惯用说法收录；相近的无必要句尾变体也删去。未从评测错误案例选词，2–4字进不了前九时只删除，不提权。最终删词与原因以本轮排除表为准。 原生验收首次并行执行时x86的test_tsf_integration出现Tab键消费不符；保留首次日志哈希，随后单独重跑完整x86构建及ctest。未修改引擎、测试或词库来规避此集成测试。最终结果以验收表和验证JSON为准。 提交前追加读音自查发现“别绊着了啊”的着误取默认zhe，改为zhao后重新冻结和合并全轮文件，重新进行最终原生验收及补词后准确率测量。 读音修正后的最终构建中x86同一Tab检查再次失败；两次失败日志及中途通过日志的哈希均记入验证JSON。最终再次使用系统默认临时目录单独运行完整x86构建及ctest，未修改测试源码或跳过测试。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1759 个含多音字的词在各批复核表逐字记录；28 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

按上下文逐字复核：还没、还在、还要取 hai；归还或返还取 huan；我得、得等多久中的得取 dei，拍得清楚的得取 de；着急取 zhao ji，看着取 kan zhe；方便取 bian，便宜取 pian；行不行取 xing，排多长队取 chang；长见识、长个记性取 zhang，深藏不露取 cang、lu。对默认读音不合语境的词逐条覆盖，轻声按本字写。 重发、重打的重取 chong；传出去、别外传的传取 chuan；文件下不了的了取 liao；你猜怎么着取 zhao；琢磨作思索取 zuo mo；长长见识取 zhang zhang；我得缓缓神取 dei；累着了取 zhao；话长的长取 chang。 追加复核将“别绊着了啊”的着改为zhao，表示绊到的结果。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 113/113 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，238144 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round35_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
| 日常口语·留出 | 150 | 89.33% → 90.00% | +0.67 | 96.00% → 96.00% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round35.md`
- `resources/dictionary/expansion/batch420_authored_draft.txt`
- `resources/dictionary/expansion/batch420_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch421_authored_draft.txt`
- `resources/dictionary/expansion/batch421_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch422_authored_draft.txt`
- `resources/dictionary/expansion/batch422_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch423_authored_draft.txt`
- `resources/dictionary/expansion/batch423_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch424_draft.txt`
- `resources/dictionary/expansion/batch424_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch425_draft.txt`
- `resources/dictionary/expansion/batch425_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch426_draft.txt`
- `resources/dictionary/expansion/batch426_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch427_draft.txt`
- `resources/dictionary/expansion/batch427_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_420.md`
- `resources/dictionary/expansion/batch_421.md`
- `resources/dictionary/expansion/batch_422.md`
- `resources/dictionary/expansion/batch_423.md`
- `resources/dictionary/expansion/batch_424.md`
- `resources/dictionary/expansion/batch_425.md`
- `resources/dictionary/expansion/batch_426.md`
- `resources/dictionary/expansion/batch_427.md`
- `resources/dictionary/expansion/lexicon_batch_420.applied.json`
- `resources/dictionary/expansion/lexicon_batch_420.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_420.json`
- `resources/dictionary/expansion/lexicon_batch_420.tsv`
- `resources/dictionary/expansion/lexicon_batch_420.validation.json`
- `resources/dictionary/expansion/lexicon_batch_421.applied.json`
- `resources/dictionary/expansion/lexicon_batch_421.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_421.json`
- `resources/dictionary/expansion/lexicon_batch_421.tsv`
- `resources/dictionary/expansion/lexicon_batch_421.validation.json`
- `resources/dictionary/expansion/lexicon_batch_422.applied.json`
- `resources/dictionary/expansion/lexicon_batch_422.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_422.json`
- `resources/dictionary/expansion/lexicon_batch_422.tsv`
- `resources/dictionary/expansion/lexicon_batch_422.validation.json`
- `resources/dictionary/expansion/lexicon_batch_423.applied.json`
- `resources/dictionary/expansion/lexicon_batch_423.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_423.json`
- `resources/dictionary/expansion/lexicon_batch_423.tsv`
- `resources/dictionary/expansion/lexicon_batch_423.validation.json`
- `resources/dictionary/expansion/lexicon_batch_424.json`
- `resources/dictionary/expansion/lexicon_batch_424.tsv`
- `resources/dictionary/expansion/lexicon_batch_425.json`
- `resources/dictionary/expansion/lexicon_batch_425.tsv`
- `resources/dictionary/expansion/lexicon_batch_426.json`
- `resources/dictionary/expansion/lexicon_batch_426.tsv`
- `resources/dictionary/expansion/lexicon_batch_427.json`
- `resources/dictionary/expansion/lexicon_batch_427.tsv`
- `resources/dictionary/expansion/probes/batch420_probe.tsv`
- `resources/dictionary/expansion/probes/batch421_probe.tsv`
- `resources/dictionary/expansion/probes/batch422_probe.tsv`
- `resources/dictionary/expansion/probes/batch423_probe.tsv`
- `resources/dictionary/expansion/probes/batch424_probe.tsv`
- `resources/dictionary/expansion/probes/batch425_probe.tsv`
- `resources/dictionary/expansion/probes/batch426_probe.tsv`
- `resources/dictionary/expansion/probes/batch427_probe.tsv`
- `resources/dictionary/expansion/round35_exclusions.tsv`
- `resources/dictionary/expansion/round35_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
