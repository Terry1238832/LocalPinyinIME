# 词库续加第15轮（批次 278–285）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `e1ae9656e0be994141794bd4abdc690582ff9072`，引擎版本 1.0.0。分支通过合并吸收 main，不 rebase、不 force push。持续 PR 的总表从第十轮起累计。

- `local_core`：212502 → 214332，新增 **1830 条**。
- 权重 6000 / 5500 / 5000：0 / 1826 / 4 条。
- 最终 SHA-256：`A4DB9B574F19AD29DAC50AB9EBC6172F064391AA9933F46EADB81EDBED2B8700`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 278 | 餐饮门店经营 | curate | 500 | 0 | 499 | 1 | 95.40% | 100.00% | `1841b0bfd8a7c129606cae10fcf21a99c2ab8421` |
| 279 | 烘焙与甜点 | curate | 500 | 0 | 499 | 1 | 94.80% | 100.00% | `d36bde85b78c6ed4d84e89707eebff54f22e4195` |
| 280 | 亲属称谓与家庭关系 | curate | 400 | 0 | 399 | 1 | 98.50% | 100.00% | `7d9b778a59e29ac0c832527c5087d5116bbb2410` |
| 281 | 天文与科普常识 | curate | 350 | 0 | 349 | 1 | 90.29% | 100.00% | `4e121db5d63aa0571edc67fcce1dd09a5481db18` |
| 282 | 餐饮门店经营较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `ff33675f7aabd10d7ae451917aa4a1517afe956b` |
| 283 | 烘焙与甜点较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `f8c8df8c59fbfb346b13d02e39fe62f1960a73fb` |
| 284 | 亲属称谓与家庭关系较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `c368882eb14bf13aa65dad87295a021510264515` |
| 285 | 天文与科普常识较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `4ca2a2c6b0491f0601b3e7aa9b309512718bc047` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round15_exclusions.tsv`，共 1033 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:incomplete_or_uncommon_label`：137 条。
- `duplicate:local_core_zh_pinyin.tsv`：322 条。
- `duplicate:standard_zh_pinyin.tsv`：23 条。
- `batch_size_limit`：547 条。
- `duplicate:batch278`：1 条。
- `quality:food_addition_label_with_unsettled_context_omit`：3 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1405 个含多音字的词在各批复核表逐字记录；33 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 餐饮：菜品重做、成本重算的重 `chong`；店长、副店长的长 `zhang`，传菜的传 `chuan`；打烊用 `dayang`，打荷用 `dahe`，食材冷藏的藏 `cang`。
- 烘焙：模具、脱模、震模、各类蛋糕模的模 `mu`，烤箱校温的校 `jiao`。磅蛋糕的磅 `bang`，长条模的长 `chang`，蛋糕长高的长 `zhang`。和面钩的和 `huo`；面团弹性的弹 `tan`，薄膜状态的薄 `bo`；奶油、蛋糕调色的调 `tiao`。
- 亲属家庭：家中长辈、长幼顺序的长 `zhang`；单亲家庭的亲 `qin`；家庭重组、重组家庭的重 `chong`。排行的行 `hang`，旅行、行程的行 `xing`；亲属、家属的属 `shu`。
- 天文：系外行星、小行星及相关行星术语的行均 `xing`；长周期彗星、长焦星空的长 `chang`。天文模拟的模保持 `mo`；望远镜校准的校 `jiao`，调焦的调 `tiao`；银河、星系的晕 `yun`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1580/1580 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，214332 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round15_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round15.md`
- `resources/dictionary/expansion/batch278_authored_draft.txt`
- `resources/dictionary/expansion/batch278_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch279_authored_draft.txt`
- `resources/dictionary/expansion/batch279_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch280_authored_draft.txt`
- `resources/dictionary/expansion/batch280_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch281_authored_draft.txt`
- `resources/dictionary/expansion/batch281_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch282_draft.txt`
- `resources/dictionary/expansion/batch282_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch283_draft.txt`
- `resources/dictionary/expansion/batch283_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch284_draft.txt`
- `resources/dictionary/expansion/batch284_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch285_draft.txt`
- `resources/dictionary/expansion/batch285_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_278.md`
- `resources/dictionary/expansion/batch_279.md`
- `resources/dictionary/expansion/batch_280.md`
- `resources/dictionary/expansion/batch_281.md`
- `resources/dictionary/expansion/batch_282.md`
- `resources/dictionary/expansion/batch_283.md`
- `resources/dictionary/expansion/batch_284.md`
- `resources/dictionary/expansion/batch_285.md`
- `resources/dictionary/expansion/lexicon_batch_278.applied.json`
- `resources/dictionary/expansion/lexicon_batch_278.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_278.json`
- `resources/dictionary/expansion/lexicon_batch_278.tsv`
- `resources/dictionary/expansion/lexicon_batch_278.validation.json`
- `resources/dictionary/expansion/lexicon_batch_279.applied.json`
- `resources/dictionary/expansion/lexicon_batch_279.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_279.json`
- `resources/dictionary/expansion/lexicon_batch_279.tsv`
- `resources/dictionary/expansion/lexicon_batch_279.validation.json`
- `resources/dictionary/expansion/lexicon_batch_280.applied.json`
- `resources/dictionary/expansion/lexicon_batch_280.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_280.json`
- `resources/dictionary/expansion/lexicon_batch_280.tsv`
- `resources/dictionary/expansion/lexicon_batch_280.validation.json`
- `resources/dictionary/expansion/lexicon_batch_281.applied.json`
- `resources/dictionary/expansion/lexicon_batch_281.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_281.json`
- `resources/dictionary/expansion/lexicon_batch_281.tsv`
- `resources/dictionary/expansion/lexicon_batch_281.validation.json`
- `resources/dictionary/expansion/lexicon_batch_282.json`
- `resources/dictionary/expansion/lexicon_batch_282.tsv`
- `resources/dictionary/expansion/lexicon_batch_283.json`
- `resources/dictionary/expansion/lexicon_batch_283.tsv`
- `resources/dictionary/expansion/lexicon_batch_284.json`
- `resources/dictionary/expansion/lexicon_batch_284.tsv`
- `resources/dictionary/expansion/lexicon_batch_285.json`
- `resources/dictionary/expansion/lexicon_batch_285.tsv`
- `resources/dictionary/expansion/probes/batch278_probe.tsv`
- `resources/dictionary/expansion/probes/batch279_probe.tsv`
- `resources/dictionary/expansion/probes/batch280_probe.tsv`
- `resources/dictionary/expansion/probes/batch281_probe.tsv`
- `resources/dictionary/expansion/probes/batch282_probe.tsv`
- `resources/dictionary/expansion/probes/batch283_probe.tsv`
- `resources/dictionary/expansion/probes/batch284_probe.tsv`
- `resources/dictionary/expansion/probes/batch285_probe.tsv`
- `resources/dictionary/expansion/round15_exclusions.tsv`
- `resources/dictionary/expansion/round15_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
