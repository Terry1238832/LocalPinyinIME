# 词库续加第10轮（批次 238–245）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `3120dea4be589a794f2913893262e9c9a694a39f`，引擎版本 0.25.0。分支通过合并吸收 main，不 rebase、不 force push。持续 PR 的总表从第十轮起累计。

- `local_core`：203097 → 204939，新增 **1842 条**。
- 权重 6000 / 5500 / 5000：0 / 1830 / 12 条。
- 最终 SHA-256：`67FFCF3DF09D056349E06CAAA333C7F4D344BBBF04CDA4DF018933793987E0E7`。
- CMake 上限 209495，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 238 | 茶与咖啡酒水饮品 | curate | 537 | 0 | 527 | 10 | 85.85% | 100.00% | `a63a1ca070cc6860c6773b286f572be037fa2d59` |
| 239 | 电脑与网络日常 | curate | 320 | 0 | 320 | 0 | 89.69% | 100.00% | `124bd9855b0358bcd33be70e2b22a81fd7740448` |
| 240 | 中学物理化学生物课本用语 | curate | 488 | 0 | 486 | 2 | 88.93% | 100.00% | `d337ed0a36d7983ca6c68255ba917fb2870b22b1` |
| 241 | 心理与情绪的日常表达 | curate | 417 | 0 | 417 | 0 | 89.69% | 100.00% | `fd0deeaf2612d097c5301caa20221028e43974ab` |
| 242 | 茶与咖啡酒水饮品较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `940c445f2fd267f2eddadced573ad1a63e1bc171` |
| 243 | 电脑与网络日常较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `1313072772bef552e2193e7f7180ce5e7fe9cf25` |
| 244 | 中学物理化学生物课本用语较长说法 | merge | 20 | 0 | 20 | 0 | 50.00% | 100.00% | `8f542cb4933ac4dbde7b30a2ac11976b8bf89973` |
| 245 | 心理与情绪的日常表达较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `f61d45a616acd41ce6898e1d8b1fe20daa56066c` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round10_exclusions.tsv`，共 629 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:unnecessary_concatenation_or_unstable_shortening`：42 条。
- `duplicate:local_core_zh_pinyin.tsv`：555 条。
- `quality:ambiguous_brand_association_omit`：1 条。
- `duplicate:standard_zh_pinyin.tsv`：25 条。
- `quality:brand_association_omit`：1 条。
- `quality:uncertain_contextual_reading_omit`：1 条。
- `outside_batch_length_scope`：4 条。

候选初检全部通过。提交前另保守排除“冰博克拿铁”和“小罐茶叶”，避免品牌关联或歧义；在临时副本重建全部冻结链，再对最终词库重新运行原生构建、候选和准确率验收。没有因准确率撤回整批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1428 个含多音字的词在各批复核表逐字记录；19 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 饮品：长萃、悠长的长 `chang`；勾调的调 `tiao`，重发酵的重 `zhong`；薄荷的薄 `bo`。茶匙的匙 `chi`，还原果汁的还 `huan`。
- 电脑网络：重装、重置、重启、重影用 `chong`；上行／下行速度的行 `xing`。打印机校准的校 `jiao`，音乐文件夹的乐 `yue`。
- 理化生：天平调平的调 `tiao`，燃烧匙的匙 `chi`，长颈漏斗的长 `chang`；标志重捕的重 `chong`，重力的重 `zhong`。浸没的没 `mo`；生长／增长的长 `zhang`。
- 情绪口语：有着落的着 `zhuo`；憋着眼泪、陪着、学着的着 `zhe`；着急、着慌的着 `zhao`；哽咽的咽 `ye`，长舒一口气的长 `chang`。接受不了的了 `liao`，语气“了”保留 `le`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1221/1221 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 209495` 通过，204939 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round10_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round10.md`
- `resources/dictionary/expansion/batch238_authored_draft.txt`
- `resources/dictionary/expansion/batch238_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch239_authored_draft.txt`
- `resources/dictionary/expansion/batch239_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch240_authored_draft.txt`
- `resources/dictionary/expansion/batch240_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch241_authored_draft.txt`
- `resources/dictionary/expansion/batch241_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch242_draft.txt`
- `resources/dictionary/expansion/batch242_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch243_draft.txt`
- `resources/dictionary/expansion/batch243_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch244_draft.txt`
- `resources/dictionary/expansion/batch244_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch245_draft.txt`
- `resources/dictionary/expansion/batch245_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_238.md`
- `resources/dictionary/expansion/batch_239.md`
- `resources/dictionary/expansion/batch_240.md`
- `resources/dictionary/expansion/batch_241.md`
- `resources/dictionary/expansion/batch_242.md`
- `resources/dictionary/expansion/batch_243.md`
- `resources/dictionary/expansion/batch_244.md`
- `resources/dictionary/expansion/batch_245.md`
- `resources/dictionary/expansion/lexicon_batch_238.applied.json`
- `resources/dictionary/expansion/lexicon_batch_238.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_238.json`
- `resources/dictionary/expansion/lexicon_batch_238.tsv`
- `resources/dictionary/expansion/lexicon_batch_238.validation.json`
- `resources/dictionary/expansion/lexicon_batch_239.applied.json`
- `resources/dictionary/expansion/lexicon_batch_239.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_239.json`
- `resources/dictionary/expansion/lexicon_batch_239.tsv`
- `resources/dictionary/expansion/lexicon_batch_239.validation.json`
- `resources/dictionary/expansion/lexicon_batch_240.applied.json`
- `resources/dictionary/expansion/lexicon_batch_240.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_240.json`
- `resources/dictionary/expansion/lexicon_batch_240.tsv`
- `resources/dictionary/expansion/lexicon_batch_240.validation.json`
- `resources/dictionary/expansion/lexicon_batch_241.applied.json`
- `resources/dictionary/expansion/lexicon_batch_241.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_241.json`
- `resources/dictionary/expansion/lexicon_batch_241.tsv`
- `resources/dictionary/expansion/lexicon_batch_241.validation.json`
- `resources/dictionary/expansion/lexicon_batch_242.json`
- `resources/dictionary/expansion/lexicon_batch_242.tsv`
- `resources/dictionary/expansion/lexicon_batch_243.json`
- `resources/dictionary/expansion/lexicon_batch_243.tsv`
- `resources/dictionary/expansion/lexicon_batch_244.json`
- `resources/dictionary/expansion/lexicon_batch_244.tsv`
- `resources/dictionary/expansion/lexicon_batch_245.json`
- `resources/dictionary/expansion/lexicon_batch_245.tsv`
- `resources/dictionary/expansion/probes/batch238_probe.tsv`
- `resources/dictionary/expansion/probes/batch239_probe.tsv`
- `resources/dictionary/expansion/probes/batch240_probe.tsv`
- `resources/dictionary/expansion/probes/batch241_probe.tsv`
- `resources/dictionary/expansion/probes/batch242_probe.tsv`
- `resources/dictionary/expansion/probes/batch243_probe.tsv`
- `resources/dictionary/expansion/probes/batch244_probe.tsv`
- `resources/dictionary/expansion/probes/batch245_probe.tsv`
- `resources/dictionary/expansion/round10_exclusions.tsv`
- `resources/dictionary/expansion/round10_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
