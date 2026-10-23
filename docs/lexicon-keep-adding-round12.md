# 词库续加第12轮（批次 254–261）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `0e98f47ae4252e0677e313e313f790b83a437cb6`，引擎版本 1.0.0。分支通过合并吸收 main，不 rebase、不 force push。持续 PR 的总表从第十轮起累计。

- `local_core`：206741 → 208652，新增 **1911 条**。
- 权重 6000 / 5500 / 5000：0 / 1902 / 9 条。
- 最终 SHA-256：`4A0A100FF16ECD30F0CA30A8F46D9B3E8FDF2300AF59C96C54A0F5EE2BFFAB65`。
- CMake 上限 209495，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 254 | 家电安装维修 | curate | 372 | 0 | 372 | 0 | 67.47% | 100.00% | `2599b2c19203e3ccc2d0b9f633c26cdbc7b1891f` |
| 255 | 户外露营徒步 | curate | 456 | 0 | 454 | 2 | 90.13% | 100.00% | `b347ab315838489a4ea777447d8c23eecd186d41` |
| 256 | 钓鱼 | curate | 500 | 0 | 494 | 6 | 86.00% | 100.00% | `a48273ea85c02718ffd2c45843dad4fc363c2bca` |
| 257 | 球类运动与赛事 | curate | 500 | 0 | 499 | 1 | 86.80% | 100.00% | `60b5f2eaaad5488024a780ae04cfd5206e05b647` |
| 258 | 家电安装维修较长说法 | merge | 21 | 0 | 21 | 0 | 71.43% | 100.00% | `9d30679bd2dda000f444a009167e0d579aa90102` |
| 259 | 户外露营徒步较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `f7fbd8009c3376917bf737dd31a1624964176abb` |
| 260 | 钓鱼较长说法 | merge | 20 | 0 | 20 | 0 | 40.00% | 100.00% | `2da10a7df4cfa78f7cfb59f66cabf97ad2e95e15` |
| 261 | 球类运动与赛事较长说法 | merge | 22 | 0 | 22 | 0 | 81.82% | 100.00% | `b59a5bd359aec50f8c2b666adcb346880eb9d942` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round12_exclusions.tsv`，共 874 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：400 条。
- `duplicate:standard_zh_pinyin.tsv`：11 条。
- `quality:incomplete_label`：4 条。
- `quality:uncertain_or_nonstandard_term_omit`：8 条。
- `outside_batch_length_scope`：5 条。
- `quality:nonstandard_fragment`：1 条。
- `quality:prefer_standard_泄力_label`：1 条。
- `quality:prefer_standard_variant_遛鱼`：1 条。
- `quality:uncommon_technical_descriptor`：2 条。
- `batch_size_limit`：437 条。
- `quality:brand_association_omit`：2 条。
- `quality:unclear_scoring_label`：1 条。
- `quality:concatenated_roles`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1417 个含多音字的词在各批复核表逐字记录；32 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 家电：空调、调平和调试的调 `tiao`；加长管的长 `chang`；点不着、打不着的着 `zhao`，堵塞的塞 `se`。拆装用 `chai`，膜壳的壳 `ke`。
- 露营徒步：所有露营和结露的露 `lu`；长气罐、长线和长度的长 `chang`；弹开帐篷的弹 `tan`。行军床、骑行的行 `xing`；冰爪的爪 `zhao`；铝锅、铝合金的铝 `lv`。
- 钓鱼：调漂、调目、调平水的调 `tiao`；硬调、软调、鱼竿调性的调 `diao`。和饵的和 `huo`；饵料粘度的粘 `nian`；长身漂、长节竿的长 `chang`；弹弓的弹 `tan`。掠食鱼用 `lveshiyu`。
- 球类：长胶、长颗粒胶、劈长、长盘、长传和长钉的长 `chang`；发球重发的重 `chong`，重杀、拍头重的重 `zhong`；挡拆外弹的弹 `tan`，肋部穿插的肋 `lei`。胶皮粘性用 `nian`，粘贴用 `zhan`；磅数的磅 `bang`；策略用 `celve`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1214/1214 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 209495` 通过，208652 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round12_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round12.md`
- `resources/dictionary/expansion/batch254_authored_draft.txt`
- `resources/dictionary/expansion/batch254_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch255_authored_draft.txt`
- `resources/dictionary/expansion/batch255_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch256_authored_draft.txt`
- `resources/dictionary/expansion/batch256_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch257_authored_draft.txt`
- `resources/dictionary/expansion/batch257_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch258_draft.txt`
- `resources/dictionary/expansion/batch258_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch259_draft.txt`
- `resources/dictionary/expansion/batch259_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch260_draft.txt`
- `resources/dictionary/expansion/batch260_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch261_draft.txt`
- `resources/dictionary/expansion/batch261_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_254.md`
- `resources/dictionary/expansion/batch_255.md`
- `resources/dictionary/expansion/batch_256.md`
- `resources/dictionary/expansion/batch_257.md`
- `resources/dictionary/expansion/batch_258.md`
- `resources/dictionary/expansion/batch_259.md`
- `resources/dictionary/expansion/batch_260.md`
- `resources/dictionary/expansion/batch_261.md`
- `resources/dictionary/expansion/lexicon_batch_254.applied.json`
- `resources/dictionary/expansion/lexicon_batch_254.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_254.json`
- `resources/dictionary/expansion/lexicon_batch_254.tsv`
- `resources/dictionary/expansion/lexicon_batch_254.validation.json`
- `resources/dictionary/expansion/lexicon_batch_255.applied.json`
- `resources/dictionary/expansion/lexicon_batch_255.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_255.json`
- `resources/dictionary/expansion/lexicon_batch_255.tsv`
- `resources/dictionary/expansion/lexicon_batch_255.validation.json`
- `resources/dictionary/expansion/lexicon_batch_256.applied.json`
- `resources/dictionary/expansion/lexicon_batch_256.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_256.json`
- `resources/dictionary/expansion/lexicon_batch_256.tsv`
- `resources/dictionary/expansion/lexicon_batch_256.validation.json`
- `resources/dictionary/expansion/lexicon_batch_257.applied.json`
- `resources/dictionary/expansion/lexicon_batch_257.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_257.json`
- `resources/dictionary/expansion/lexicon_batch_257.tsv`
- `resources/dictionary/expansion/lexicon_batch_257.validation.json`
- `resources/dictionary/expansion/lexicon_batch_258.json`
- `resources/dictionary/expansion/lexicon_batch_258.tsv`
- `resources/dictionary/expansion/lexicon_batch_259.json`
- `resources/dictionary/expansion/lexicon_batch_259.tsv`
- `resources/dictionary/expansion/lexicon_batch_260.json`
- `resources/dictionary/expansion/lexicon_batch_260.tsv`
- `resources/dictionary/expansion/lexicon_batch_261.json`
- `resources/dictionary/expansion/lexicon_batch_261.tsv`
- `resources/dictionary/expansion/probes/batch254_probe.tsv`
- `resources/dictionary/expansion/probes/batch255_probe.tsv`
- `resources/dictionary/expansion/probes/batch256_probe.tsv`
- `resources/dictionary/expansion/probes/batch257_probe.tsv`
- `resources/dictionary/expansion/probes/batch258_probe.tsv`
- `resources/dictionary/expansion/probes/batch259_probe.tsv`
- `resources/dictionary/expansion/probes/batch260_probe.tsv`
- `resources/dictionary/expansion/probes/batch261_probe.tsv`
- `resources/dictionary/expansion/round12_exclusions.tsv`
- `resources/dictionary/expansion/round12_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
