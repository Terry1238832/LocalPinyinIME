# 词库续加第24轮（批次 350–357）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `9b0e2c203d6d1761726774fd5ffe6869666778ce`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：228008 → 229511，新增 **1503 条**。
- 权重 6000 / 5500 / 5000：0 / 1497 / 6 条。
- 最终 SHA-256：`583ADB8D51CE0751D42572217B91AD302185B1517991F0B614E4F69C73E69E23`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

主题清单已完成。本轮自拟滑雪滑冰、毛线编织与钩针、出版印刷与图书制作、花艺插花与花束养护四个专题：分别覆盖季节性运动体验、家庭编织爱好、日常印书沟通、送花与鲜花养护中会整词输入的说法。它们与既有体育、文具手作、图书阅读、园艺主题有相关处，但此前没有专批；按所有既有词面去重，优先补实际器材、工序与沟通用语，不扩写参数排列。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 350 | 滑雪滑冰与冰上活动 | curate | 395 | 0 | 395 | 0 | 88.61% | 100.00% | `e155f678147df48037498c1db5c10a9c7c92066b` |
| 351 | 毛线编织与钩针 | curate | 322 | 0 | 320 | 2 | 88.82% | 100.00% | `f5c96672b6fcae1a5a0f586d27a490d53b100172` |
| 352 | 出版印刷与图书制作 | curate | 365 | 0 | 363 | 2 | 91.23% | 100.00% | `4dfe06877a0c14239fc8f609a67b1c48c90bca92` |
| 353 | 花艺插花与花束养护 | curate | 341 | 0 | 339 | 2 | 92.67% | 100.00% | `f9dd31608c10aeb3f0de7ca2d84694cb8dffcfb0` |
| 354 | 滑雪滑冰与冰上活动较长说法 | merge | 20 | 0 | 20 | 0 | 45.00% | 100.00% | `3b6f5f00e75adcffe2ad69b4517dce7dc7491b1c` |
| 355 | 毛线编织与钩针较长说法 | merge | 20 | 0 | 20 | 0 | 20.00% | 100.00% | `83e3de04a75d088e982788adb6d392b3547669f0` |
| 356 | 出版印刷与图书制作较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `441ab2ffed513c2d66aecbc4765cd6258b90de17` |
| 357 | 花艺插花与花束养护较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `a259cd0fb7333d6de675182789c0881a666794a4` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round24_exclusions.tsv`，共 2659 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality: overly narrow, redundant description or unnatural whole-input phrase`：509 条。
- `duplicate:local_core_zh_pinyin.tsv`：490 条。
- `quality: cold specialist vocabulary, non-Han notation, name-derived label or unnatural expression`：29 条。
- `duplicate:standard_zh_pinyin.tsv`：73 条。
- `batch_size_limit`：1556 条。
- `non_BMP_Han_or_non_Han`：1 条。
- `duplicate:core_zh_pinyin.tsv`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

985 个含多音字的词在各批复核表逐字记录；18 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐条阅读全拼：编织的行数、逐行按 hang；校对、校样按 jiao；重印、重排按 chong；发行按 xing；勒口按 le；花束按 shu，倒挂按 dao。ü/üe 继续写 v/lve/nve，轻声按本字写。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1308/1308 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，229511 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round24_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.09% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.44% → 73.44% |
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
- `docs/lexicon-keep-adding-round24.md`
- `resources/dictionary/expansion/batch350_authored_draft.txt`
- `resources/dictionary/expansion/batch350_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch351_authored_draft.txt`
- `resources/dictionary/expansion/batch351_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch352_authored_draft.txt`
- `resources/dictionary/expansion/batch352_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch353_authored_draft.txt`
- `resources/dictionary/expansion/batch353_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch354_draft.txt`
- `resources/dictionary/expansion/batch354_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch355_draft.txt`
- `resources/dictionary/expansion/batch355_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch356_draft.txt`
- `resources/dictionary/expansion/batch356_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch357_draft.txt`
- `resources/dictionary/expansion/batch357_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_350.md`
- `resources/dictionary/expansion/batch_351.md`
- `resources/dictionary/expansion/batch_352.md`
- `resources/dictionary/expansion/batch_353.md`
- `resources/dictionary/expansion/batch_354.md`
- `resources/dictionary/expansion/batch_355.md`
- `resources/dictionary/expansion/batch_356.md`
- `resources/dictionary/expansion/batch_357.md`
- `resources/dictionary/expansion/lexicon_batch_350.applied.json`
- `resources/dictionary/expansion/lexicon_batch_350.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_350.json`
- `resources/dictionary/expansion/lexicon_batch_350.tsv`
- `resources/dictionary/expansion/lexicon_batch_350.validation.json`
- `resources/dictionary/expansion/lexicon_batch_351.applied.json`
- `resources/dictionary/expansion/lexicon_batch_351.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_351.json`
- `resources/dictionary/expansion/lexicon_batch_351.tsv`
- `resources/dictionary/expansion/lexicon_batch_351.validation.json`
- `resources/dictionary/expansion/lexicon_batch_352.applied.json`
- `resources/dictionary/expansion/lexicon_batch_352.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_352.json`
- `resources/dictionary/expansion/lexicon_batch_352.tsv`
- `resources/dictionary/expansion/lexicon_batch_352.validation.json`
- `resources/dictionary/expansion/lexicon_batch_353.applied.json`
- `resources/dictionary/expansion/lexicon_batch_353.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_353.json`
- `resources/dictionary/expansion/lexicon_batch_353.tsv`
- `resources/dictionary/expansion/lexicon_batch_353.validation.json`
- `resources/dictionary/expansion/lexicon_batch_354.json`
- `resources/dictionary/expansion/lexicon_batch_354.tsv`
- `resources/dictionary/expansion/lexicon_batch_355.json`
- `resources/dictionary/expansion/lexicon_batch_355.tsv`
- `resources/dictionary/expansion/lexicon_batch_356.json`
- `resources/dictionary/expansion/lexicon_batch_356.tsv`
- `resources/dictionary/expansion/lexicon_batch_357.json`
- `resources/dictionary/expansion/lexicon_batch_357.tsv`
- `resources/dictionary/expansion/probes/batch350_probe.tsv`
- `resources/dictionary/expansion/probes/batch351_probe.tsv`
- `resources/dictionary/expansion/probes/batch352_probe.tsv`
- `resources/dictionary/expansion/probes/batch353_probe.tsv`
- `resources/dictionary/expansion/probes/batch354_probe.tsv`
- `resources/dictionary/expansion/probes/batch355_probe.tsv`
- `resources/dictionary/expansion/probes/batch356_probe.tsv`
- `resources/dictionary/expansion/probes/batch357_probe.tsv`
- `resources/dictionary/expansion/round24_exclusions.tsv`
- `resources/dictionary/expansion/round24_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
