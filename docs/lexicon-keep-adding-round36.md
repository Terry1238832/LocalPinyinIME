# 词库续加第36轮（批次 428–435）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `bb50051111238147d40d810c399ed0dfe87fdaf4`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：238144 → 239525，新增 **1381 条**。
- 权重 6000 / 5500 / 5000：0 / 1381 / 0 条。
- 最终 SHA-256：`598AC1E5DB029EC8EB862A9DBF5DDB82FDCE3D2AC209474AAE3FC949C78A49AE`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。承接第35轮消息联络、约见互助与反应熟语，逐条自拟吃饭餐桌、作息起居、忙闲交接和解释商量四类自然整段说法；按已有全部批次词面去重，不机械替换人称、数字和句尾，不根据测试样例或评测失败选词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 428 | 吃饭点单与餐桌聊天 | curate | 339 | 0 | 339 | 0 | 68.73% | 100.00% | `b1935a19af84423cfdf9f6d6f5bb95c2050fa2d5` |
| 429 | 作息起居与家里聊天 | curate | 338 | 0 | 338 | 0 | 67.16% | 100.00% | `be40f98be9509e6af14b514504edd5845abbbbfb` |
| 430 | 忙闲交接与临时安排聊天 | curate | 309 | 0 | 309 | 0 | 78.64% | 100.00% | `f8d5999843fe0b81d7069a8f6661df8a9d6a7998` |
| 431 | 解释确认与商量说法 | curate | 317 | 0 | 317 | 0 | 85.17% | 100.00% | `d779726e7e2af0d3d17371e8a82641e3b292180a` |
| 432 | 吃饭点单与餐桌聊天较长说法 | merge | 19 | 0 | 19 | 0 | 94.74% | 100.00% | `df9bf1eeb96a8fb526c2723bd9793025faddad91` |
| 433 | 作息起居与家里聊天较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `ab5abd52b31bd9925147d74f686a9bae8c436109` |
| 434 | 忙闲交接与临时安排聊天较长说法 | merge | 19 | 0 | 19 | 0 | 84.21% | 100.00% | `9c939b015f5b774472ce9203bae4b991d59bcf51` |
| 435 | 解释确认与商量较长说法 | merge | 20 | 0 | 20 | 0 | 95.00% | 100.00% | `77832d3415c34bb560ae3cf689bc3923f25cbe9e` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round36_exclusions.tsv`，共 248 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：110 条。
- `outside_batch_length_scope`：29 条。
- `quality: incomplete clause, forced word order or unnecessary equivalent variant`：52 条。
- `quality: weak standalone expression or redundant variant`：18 条。
- `quality: unnecessary stacked sentence particles or forced inverted ending`：3 条。
- `quality: ambiguous underspecified object or incomplete timing clause`：14 条。
- `quality: unclear referent, incomplete explanation or unnatural verb choice`：17 条。
- `quality: unnecessarily awkward full-sentence wording`：5 条。

最终测量前追加复核删除“还躺着呢啊”“门开着呢啊”“秒睡了啊我”，避免堆叠句末语气词或生硬倒装；将“我又睡不着了”的着由默认zhe改为zhao，重新冻结各批文件后再进行最终测量。其他排除词包括重复词、截断的时间或解释分句、不自然的动词搭配、无必要的同义句尾变体及不符合当前长短批范围的词。逐词见排除表。未从评测失败样例取词，没有通过提高权重修复探针。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1284 个含多音字的词在各批复核表逐字记录；45 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐字按聊天语境复核：还没、还在的还取hai；我得、得看、得再取dei，吃得、说得等取de；睡不着、夹不着、够不着、烫着、噎着表示结果取zhao，先点着（点菜）、吃着、拿着表示持续取zhe；一觉、回笼觉的觉取jiao；有点嚼不动、越嚼越香中的单字口语动词嚼取jiao；剥好了、剥鸡蛋取bao；我看漏了一行的行取hang；调高、调凉、调整取tiao；袋子扎起来取za；言归正传中的传取zhuan；轻声按本字写，üe规范化为lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 4/4 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，239525 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round36_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round36.md`
- `resources/dictionary/expansion/batch428_authored_draft.txt`
- `resources/dictionary/expansion/batch428_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch429_authored_draft.txt`
- `resources/dictionary/expansion/batch429_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch430_authored_draft.txt`
- `resources/dictionary/expansion/batch430_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch431_authored_draft.txt`
- `resources/dictionary/expansion/batch431_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch432_draft.txt`
- `resources/dictionary/expansion/batch432_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch433_draft.txt`
- `resources/dictionary/expansion/batch433_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch434_draft.txt`
- `resources/dictionary/expansion/batch434_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch435_draft.txt`
- `resources/dictionary/expansion/batch435_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_428.md`
- `resources/dictionary/expansion/batch_429.md`
- `resources/dictionary/expansion/batch_430.md`
- `resources/dictionary/expansion/batch_431.md`
- `resources/dictionary/expansion/batch_432.md`
- `resources/dictionary/expansion/batch_433.md`
- `resources/dictionary/expansion/batch_434.md`
- `resources/dictionary/expansion/batch_435.md`
- `resources/dictionary/expansion/lexicon_batch_428.applied.json`
- `resources/dictionary/expansion/lexicon_batch_428.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_428.json`
- `resources/dictionary/expansion/lexicon_batch_428.tsv`
- `resources/dictionary/expansion/lexicon_batch_428.validation.json`
- `resources/dictionary/expansion/lexicon_batch_429.applied.json`
- `resources/dictionary/expansion/lexicon_batch_429.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_429.json`
- `resources/dictionary/expansion/lexicon_batch_429.tsv`
- `resources/dictionary/expansion/lexicon_batch_429.validation.json`
- `resources/dictionary/expansion/lexicon_batch_430.applied.json`
- `resources/dictionary/expansion/lexicon_batch_430.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_430.json`
- `resources/dictionary/expansion/lexicon_batch_430.tsv`
- `resources/dictionary/expansion/lexicon_batch_430.validation.json`
- `resources/dictionary/expansion/lexicon_batch_431.applied.json`
- `resources/dictionary/expansion/lexicon_batch_431.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_431.json`
- `resources/dictionary/expansion/lexicon_batch_431.tsv`
- `resources/dictionary/expansion/lexicon_batch_431.validation.json`
- `resources/dictionary/expansion/lexicon_batch_432.json`
- `resources/dictionary/expansion/lexicon_batch_432.tsv`
- `resources/dictionary/expansion/lexicon_batch_433.json`
- `resources/dictionary/expansion/lexicon_batch_433.tsv`
- `resources/dictionary/expansion/lexicon_batch_434.json`
- `resources/dictionary/expansion/lexicon_batch_434.tsv`
- `resources/dictionary/expansion/lexicon_batch_435.json`
- `resources/dictionary/expansion/lexicon_batch_435.tsv`
- `resources/dictionary/expansion/probes/batch428_probe.tsv`
- `resources/dictionary/expansion/probes/batch429_probe.tsv`
- `resources/dictionary/expansion/probes/batch430_probe.tsv`
- `resources/dictionary/expansion/probes/batch431_probe.tsv`
- `resources/dictionary/expansion/probes/batch432_probe.tsv`
- `resources/dictionary/expansion/probes/batch433_probe.tsv`
- `resources/dictionary/expansion/probes/batch434_probe.tsv`
- `resources/dictionary/expansion/probes/batch435_probe.tsv`
- `resources/dictionary/expansion/round36_exclusions.tsv`
- `resources/dictionary/expansion/round36_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
