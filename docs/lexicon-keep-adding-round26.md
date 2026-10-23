# 词库续加第26轮（批次 366–373）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `4c157d91ca21d55dd794510b786131440c67d096`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：230848 → 232171，新增 **1323 条**。
- 权重 6000 / 5500 / 5000：0 / 1314 / 9 条。
- 最终 SHA-256：`7781E96557E326FD7E763967B968A00A798AB34992D62041F70604AAC6D4E7C2`。
- CMake 上限从 231676 调整到 255390，约为新总数的 110%；独立提交 `0c95c251511c5f04996e631cfe72e76ba017c3d3`。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

原主题清单已完成。本轮自拟攀岩绳索、书信集邮、皮具手作、公园游乐四个专题，服务岩馆体验与装备沟通、写寄纸信及整理邮票、日常皮具制作修护、亲子游园和设备乘坐等实际输入。与既有户外、快递、服饰、旅游主题有相关处，但没有对应专批；所有词面按历史全库去重，删去品牌、人名衍生标签、冷僻参数和不自然组合。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 366 | 攀岩与绳索运动 | curate | 306 | 0 | 301 | 5 | 85.62% | 100.00% | `9d0ecfe9618cb4ad831df1d1467987f639aa0019` |
| 367 | 书信邮寄与集邮 | curate | 301 | 0 | 300 | 1 | 93.69% | 100.00% | `be7de2475ad993e1275988d80d94b36d6907053b` |
| 368 | 皮具手作与皮革养护 | curate | 302 | 0 | 300 | 2 | 93.38% | 100.00% | `f65eddf8c697aa3e12100d06bec1a3ca79c5fdcc` |
| 369 | 公园游乐与游乐设施 | curate | 334 | 0 | 333 | 1 | 93.11% | 100.00% | `1363ff7186036e88b9a3a88e0b258367d8d3f929` |
| 370 | 攀岩与绳索运动较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `37f0356548cd6f5d0b762ff2b5f12d36a5464b96` |
| 371 | 书信邮寄与集邮较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `82ccb6277780dd12a93978f533b864c1ca2d231c` |
| 372 | 皮具手作与皮革养护较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `69bf610712f189cd5e08dc7c0b70f293966f2cca` |
| 373 | 公园游乐与游乐设施较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `48c26175e36a2a73ae9aed7bf4a5bfff70fbdcda` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round26_exclusions.tsv`，共 1338 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：238 条。
- `quality: overly narrow parameters, repetitive description or uncommon trade jargon`：427 条。
- `quality: name-derived label, overly specialist or unnatural phrase`：45 条。
- `quality: less common label, unnatural combination or uncertain reading`：39 条。
- `duplicate:standard_zh_pinyin.tsv`：33 条。
- `probe_top9: qishi homophones cap the new term at 5000; absent from top nine, omit rather than raise weight`：1 条。
- `quality: uncommon trade detail, repetitive parameter or unnatural whole-input phrase`：554 条。
- `duplicate:batch366`：1 条。

首次探针中旗式 qishi 受已有同拼最低 5000 档限制，未进前九，删除该词。随后从本轮基线重建未提交批次，并重跑双架构构建、全部探针、curate evaluate、前后准确率与结构校验。未撤回整批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

901 个含多音字的词在各批复核表逐字记录；13 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐条审阅全拼。绳索下降及绳降的降读 jiang，扣件卡住读 ka，归还读 huan；信札读 zha，邮差读 chai，发行读 xing，邮票褪色读 se；皮具手缝读 feng，麂皮读 ji，铬鞣读 ge rou，刀具削薄、削边取 xiao，薄皮/削薄取 bao，皮面褪色取 se，包带加长取 chang；弹跳取 tan，乐园取 le，旋转取 zhuan，时间长和队伍长取 chang，家长陪同取 zhang。每个含多音字的词逐字列出所选拼读。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1075/1075 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，232171 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round26_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.95% → 85.95% | +0.00 | 98.09% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.44% → 73.44% |
| 口语对话·整句 | 3368 | 80.40% → 80.40% | +0.00 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `CMakeLists.txt`
- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round26.md`
- `resources/dictionary/expansion/batch366_authored_draft.txt`
- `resources/dictionary/expansion/batch366_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch367_authored_draft.txt`
- `resources/dictionary/expansion/batch367_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch368_authored_draft.txt`
- `resources/dictionary/expansion/batch368_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch369_authored_draft.txt`
- `resources/dictionary/expansion/batch369_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch370_draft.txt`
- `resources/dictionary/expansion/batch370_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch371_draft.txt`
- `resources/dictionary/expansion/batch371_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch372_draft.txt`
- `resources/dictionary/expansion/batch372_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch373_draft.txt`
- `resources/dictionary/expansion/batch373_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_366.md`
- `resources/dictionary/expansion/batch_367.md`
- `resources/dictionary/expansion/batch_368.md`
- `resources/dictionary/expansion/batch_369.md`
- `resources/dictionary/expansion/batch_370.md`
- `resources/dictionary/expansion/batch_371.md`
- `resources/dictionary/expansion/batch_372.md`
- `resources/dictionary/expansion/batch_373.md`
- `resources/dictionary/expansion/lexicon_batch_366.applied.json`
- `resources/dictionary/expansion/lexicon_batch_366.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_366.json`
- `resources/dictionary/expansion/lexicon_batch_366.tsv`
- `resources/dictionary/expansion/lexicon_batch_366.validation.json`
- `resources/dictionary/expansion/lexicon_batch_367.applied.json`
- `resources/dictionary/expansion/lexicon_batch_367.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_367.json`
- `resources/dictionary/expansion/lexicon_batch_367.tsv`
- `resources/dictionary/expansion/lexicon_batch_367.validation.json`
- `resources/dictionary/expansion/lexicon_batch_368.applied.json`
- `resources/dictionary/expansion/lexicon_batch_368.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_368.json`
- `resources/dictionary/expansion/lexicon_batch_368.tsv`
- `resources/dictionary/expansion/lexicon_batch_368.validation.json`
- `resources/dictionary/expansion/lexicon_batch_369.applied.json`
- `resources/dictionary/expansion/lexicon_batch_369.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_369.json`
- `resources/dictionary/expansion/lexicon_batch_369.tsv`
- `resources/dictionary/expansion/lexicon_batch_369.validation.json`
- `resources/dictionary/expansion/lexicon_batch_370.json`
- `resources/dictionary/expansion/lexicon_batch_370.tsv`
- `resources/dictionary/expansion/lexicon_batch_371.json`
- `resources/dictionary/expansion/lexicon_batch_371.tsv`
- `resources/dictionary/expansion/lexicon_batch_372.json`
- `resources/dictionary/expansion/lexicon_batch_372.tsv`
- `resources/dictionary/expansion/lexicon_batch_373.json`
- `resources/dictionary/expansion/lexicon_batch_373.tsv`
- `resources/dictionary/expansion/probes/batch366_probe.tsv`
- `resources/dictionary/expansion/probes/batch367_probe.tsv`
- `resources/dictionary/expansion/probes/batch368_probe.tsv`
- `resources/dictionary/expansion/probes/batch369_probe.tsv`
- `resources/dictionary/expansion/probes/batch370_probe.tsv`
- `resources/dictionary/expansion/probes/batch371_probe.tsv`
- `resources/dictionary/expansion/probes/batch372_probe.tsv`
- `resources/dictionary/expansion/probes/batch373_probe.tsv`
- `resources/dictionary/expansion/round26_exclusions.tsv`
- `resources/dictionary/expansion/round26_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
