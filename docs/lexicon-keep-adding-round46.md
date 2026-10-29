# 词库续加第46轮（批次 496–501）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `70889fda22912ac005822fa839c5abc08a07dbaf`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：250329 → 251361，新增 **1032 条**。
- 权重 6000 / 5500 / 5000：0 / 1032 / 0 条。
- 最终 SHA-256：`272E95D320380A93AB25483448BF705FFAFD00065585FDF09C39DCB14F02DDC6`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮继续日常聊天，自拟观影听歌、出游行李行前确认、逛店试穿挑选三个方向，每类一短一长共六批。按真实生活交流拟写完整说法，不抄作品台词歌词，不按人物数字句尾套模板，不从测试或第三方词库取词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 496 | 观影追剧与听歌交流聊天 | curate | 318 | 0 | 318 | 0 | 73.58% | 100.00% | `0ce0587cf5234a97d96e9bb1f8e3b1f3fd287a3e` |
| 497 | 出游行李与行前确认聊天 | curate | 327 | 0 | 327 | 0 | 71.87% | 100.00% | `5e482790b46a3e847c1e6abab16e5721003008a1` |
| 498 | 逛店试穿与挑选商量聊天 | curate | 327 | 0 | 327 | 0 | 64.83% | 100.00% | `a67726d52db879d6e283940dbcf81a5b9069689c` |
| 499 | 观影追剧与听歌交流较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `932a2c5ae21870b11d3ec1c52a59ddc1ea87746f` |
| 500 | 出游行李与行前确认较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `560d1d4715c7b92ca453aca9066f70dea7c720b8` |
| 501 | 逛店试穿与挑选商量较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 95.00% | `6ab426cd581a7df8af84866e5f220bb41289670e` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round46_exclusions.tsv`，共 471 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：30 条。
- `outside_batch_length_scope`：405 条。
- `quality: incomplete musical lead-in, unclear headphone reference, awkward volume instruction or clipped seating description`：8 条。
- `quality: clipped beat-leading request, incomplete queue reply or unnatural checklist and handle wording`：5 条。
- `quality: incomplete station question, unclear map or luggage reference, awkward travel reminder or stacked particles`：10 条。
- `quality: missing clothing object, awkward stable-product description, unclear sharing or incomplete lid instruction`：9 条。
- `pronunciation: defer mechanical 卡 wording rather than guess`：1 条。
- `quality: ambiguous viewing wait request, missing luggage pickup object or incomplete seat inspection wording`：3 条。

不收未说完的前奏引句、我当背景音、过度残缺的座位指代、不自然的音量问法；改成背景音听、明确耳机和屏幕动作。行前草稿去掉语气叠加、锁密码和地图点等错误或残缺搭配，改为定位、密码、行李箱和过道边的明确说法。逛店草稿去掉残缺的衣物描述、共享数量和盖子配套指令，另拟自然说法；题材扩展到同一逛店语境中的日用物件手感、规格、包装与结账核对。长句补清行李领取和下车检查的对象，不从评测失败取词。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

953 个含多音字的词在各批复核表逐字记录；27 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字审阅：重看、重进和重复用chong；调音量、调静音、调亮度用tiao，歌曲的调和跑调、降调用diao，调货用diao，空调用tiao。归还观影眼镜用huan，仍然和未完事项的还用hai；情态得为dei，补语和省得、免得用de。系鞋带用ji；裤腿、商品尺寸、歌曲尾音的长用chang。烫着手是结果zhao，穿着、戴着、看着、拧着和卡着点的着按持续zhe；卡着点是时间卡点的常见ka，轮子卡住的机械语境不猜，原稿排除。测量尺寸量用liang，口语捋思路选lv，鞋楦用xuan；谁选shui，不增同词面异读。水杯别落下和落东西取la；厚重的重为zhong。轻声按本字，üe为lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，251361 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round46_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round46.md`
- `resources/dictionary/expansion/batch496_authored_draft.txt`
- `resources/dictionary/expansion/batch496_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch497_authored_draft.txt`
- `resources/dictionary/expansion/batch497_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch498_authored_draft.txt`
- `resources/dictionary/expansion/batch498_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch499_draft.txt`
- `resources/dictionary/expansion/batch499_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch500_draft.txt`
- `resources/dictionary/expansion/batch500_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch501_draft.txt`
- `resources/dictionary/expansion/batch501_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_496.md`
- `resources/dictionary/expansion/batch_497.md`
- `resources/dictionary/expansion/batch_498.md`
- `resources/dictionary/expansion/batch_499.md`
- `resources/dictionary/expansion/batch_500.md`
- `resources/dictionary/expansion/batch_501.md`
- `resources/dictionary/expansion/lexicon_batch_496.applied.json`
- `resources/dictionary/expansion/lexicon_batch_496.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_496.json`
- `resources/dictionary/expansion/lexicon_batch_496.tsv`
- `resources/dictionary/expansion/lexicon_batch_496.validation.json`
- `resources/dictionary/expansion/lexicon_batch_497.applied.json`
- `resources/dictionary/expansion/lexicon_batch_497.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_497.json`
- `resources/dictionary/expansion/lexicon_batch_497.tsv`
- `resources/dictionary/expansion/lexicon_batch_497.validation.json`
- `resources/dictionary/expansion/lexicon_batch_498.applied.json`
- `resources/dictionary/expansion/lexicon_batch_498.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_498.json`
- `resources/dictionary/expansion/lexicon_batch_498.tsv`
- `resources/dictionary/expansion/lexicon_batch_498.validation.json`
- `resources/dictionary/expansion/lexicon_batch_499.json`
- `resources/dictionary/expansion/lexicon_batch_499.tsv`
- `resources/dictionary/expansion/lexicon_batch_500.json`
- `resources/dictionary/expansion/lexicon_batch_500.tsv`
- `resources/dictionary/expansion/lexicon_batch_501.json`
- `resources/dictionary/expansion/lexicon_batch_501.tsv`
- `resources/dictionary/expansion/probes/batch496_probe.tsv`
- `resources/dictionary/expansion/probes/batch497_probe.tsv`
- `resources/dictionary/expansion/probes/batch498_probe.tsv`
- `resources/dictionary/expansion/probes/batch499_probe.tsv`
- `resources/dictionary/expansion/probes/batch500_probe.tsv`
- `resources/dictionary/expansion/probes/batch501_probe.tsv`
- `resources/dictionary/expansion/round46_exclusions.tsv`
- `resources/dictionary/expansion/round46_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
