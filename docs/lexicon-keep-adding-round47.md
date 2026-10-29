# 词库续加第47轮（批次 502–507）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `bdd92ac770c3562fd2e709c9d466375b537be7e7`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：251361 → 252476，新增 **1115 条**。
- 权重 6000 / 5500 / 5000：0 / 1115 / 0 条。
- 最终 SHA-256：`37C7D2707443D9105B9ABB11E5D2A3C02F9AF2D7CFA908C1366FD8C998C6870F`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮继续自然的日常聊天，自拟临时工作交接协作、现场排队问询、周末活动组织协调三个方向，各一短一长共六批。按完整意思逐条拟写，不按人称、数字、句尾批量套模板，不从测试失败或词表找词；对太长及含混的草稿记录排除原因。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 502 | 临时工作交接与协作聊天 | curate | 388 | 0 | 388 | 0 | 73.45% | 100.00% | `16a3d33c4ba1433eda2ee0448d0327199bacaa6b` |
| 503 | 现场排队等候与问询聊天 | curate | 340 | 0 | 340 | 0 | 65.59% | 100.00% | `6e11f12b52b1c580435cc630c87ee9441f90ba4a` |
| 504 | 周末活动与组织协调聊天 | curate | 326 | 0 | 326 | 0 | 68.40% | 100.00% | `fd8ed345e9291165f651edf803b9edb005176890` |
| 505 | 临时工作交接与协作较长说法 | merge | 21 | 0 | 21 | 0 | 76.19% | 90.48% | `669378a67937ae6ef4a30d99ac89ed3a73cfd4c9` |
| 506 | 现场排队等候与问询较长说法 | merge | 20 | 0 | 20 | 0 | 60.00% | 90.00% | `f3448cd9e957b302d8e89f22f23da4c105240866` |
| 507 | 周末活动与组织协调较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 95.00% | `dd90a11cf0455984ecd5b868b673f90447cc03b5` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round47_exclusions.tsv`，共 481 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：37 条。
- `outside_batch_length_scope`：403 条。
- `quality: clipped permission or deadline noun, unclear task predicate or awkward cooperation wording`：15 条。
- `quality: missing directional complement, unclear document noun or clipped application request`：10 条。
- `quality: incomplete late-arrival condition, awkward location wording, clipped participant or cup reference`：14 条。
- `quality: unclear party host or overly clipped travel-to-party purpose in longer wording`：2 条。

不把七字以上说法截成不完整短句。旧稿中权限编辑、核实数、截止、材料拿出、纸张带回、介绍、邮寄收、杯子归属、迟到条件、收走垃圾、轮流游戏等不完整或不自然表达逐条排除，另拟明确的数字核实、截止时间、只读权限、材料收放、说明单、邮寄、杯子和活动提醒。两条长句的赶路目的和开心的主体改成自然说法；所有旧稿和原因写入排除表。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1024 个含多音字的词在各批复核表逐字记录；35 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：重发、重写、重做、重来均chong，调行距和边距取tiao，文字行、填串行、行距取hang；可以行得通的行取xing。必须义的得取dei，补语得及省得、免得用de，了按句末le或改不了的liao各自语境判断。归还笔、材料及还人情取huan，仍然和未完事项的还取hai，这回还带谁来不能套还带huan dai。饿着肚子等、饿着过来是持续空腹状态取zhe；够不着、踩着食物、绊着别人表示触及结果取zhao；猜着做、看着号、坐着均zhe。扎好袋口取za；谁按常用口语选择shui；排队长度取chang，思路卡在某一步按ka。轻声按本字，üe写lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，252476 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round47_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round47.md`
- `resources/dictionary/expansion/batch502_authored_draft.txt`
- `resources/dictionary/expansion/batch502_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch503_authored_draft.txt`
- `resources/dictionary/expansion/batch503_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch504_authored_draft.txt`
- `resources/dictionary/expansion/batch504_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch505_draft.txt`
- `resources/dictionary/expansion/batch505_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch506_draft.txt`
- `resources/dictionary/expansion/batch506_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch507_draft.txt`
- `resources/dictionary/expansion/batch507_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_502.md`
- `resources/dictionary/expansion/batch_503.md`
- `resources/dictionary/expansion/batch_504.md`
- `resources/dictionary/expansion/batch_505.md`
- `resources/dictionary/expansion/batch_506.md`
- `resources/dictionary/expansion/batch_507.md`
- `resources/dictionary/expansion/lexicon_batch_502.applied.json`
- `resources/dictionary/expansion/lexicon_batch_502.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_502.json`
- `resources/dictionary/expansion/lexicon_batch_502.tsv`
- `resources/dictionary/expansion/lexicon_batch_502.validation.json`
- `resources/dictionary/expansion/lexicon_batch_503.applied.json`
- `resources/dictionary/expansion/lexicon_batch_503.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_503.json`
- `resources/dictionary/expansion/lexicon_batch_503.tsv`
- `resources/dictionary/expansion/lexicon_batch_503.validation.json`
- `resources/dictionary/expansion/lexicon_batch_504.applied.json`
- `resources/dictionary/expansion/lexicon_batch_504.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_504.json`
- `resources/dictionary/expansion/lexicon_batch_504.tsv`
- `resources/dictionary/expansion/lexicon_batch_504.validation.json`
- `resources/dictionary/expansion/lexicon_batch_505.json`
- `resources/dictionary/expansion/lexicon_batch_505.tsv`
- `resources/dictionary/expansion/lexicon_batch_506.json`
- `resources/dictionary/expansion/lexicon_batch_506.tsv`
- `resources/dictionary/expansion/lexicon_batch_507.json`
- `resources/dictionary/expansion/lexicon_batch_507.tsv`
- `resources/dictionary/expansion/probes/batch502_probe.tsv`
- `resources/dictionary/expansion/probes/batch503_probe.tsv`
- `resources/dictionary/expansion/probes/batch504_probe.tsv`
- `resources/dictionary/expansion/probes/batch505_probe.tsv`
- `resources/dictionary/expansion/probes/batch506_probe.tsv`
- `resources/dictionary/expansion/probes/batch507_probe.tsv`
- `resources/dictionary/expansion/round47_exclusions.tsv`
- `resources/dictionary/expansion/round47_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
