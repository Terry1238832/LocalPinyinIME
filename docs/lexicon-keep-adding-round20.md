# 词库续加第20轮（批次 318–325）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `72afa798376faf25040b341c5581a39fc425af14`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：221998 → 223596，新增 **1598 条**。
- 权重 6000 / 5500 / 5000：0 / 1595 / 3 条。
- 最终 SHA-256：`2E874750D5787EDF7C8BE43A88BB365D0A2E9783123F8BDB68D307F548904DA2`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 318 | 防诈骗与网络安全 | curate | 338 | 0 | 336 | 2 | 96.15% | 100.00% | `94db0d3c627cd09f3aa8a1fd4f5514361c33ba9e` |
| 319 | 智能家居 | curate | 404 | 0 | 404 | 0 | 92.82% | 100.00% | `458667ca9729750fb62a6cfeac73f0329e80ff7d` |
| 320 | 人工智能与大模型日常说法 | curate | 385 | 0 | 384 | 1 | 96.62% | 100.00% | `751d959b7fab81a189030c0cabb442476b166ce7` |
| 321 | 短视频剪辑 | curate | 391 | 0 | 391 | 0 | 97.95% | 100.00% | `ef2d0d39d8e909b000068418a39c6d04b6aa06d1` |
| 322 | 防诈骗与网络安全较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `81332a79457dd1522f646be7d8c7b815f0714214` |
| 323 | 智能家居较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `7a166d753a867fe5e8e71a75641202673c504dbf` |
| 324 | 人工智能与大模型较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `6b791a81325891f754958fc2b2e6a90490414144` |
| 325 | 短视频剪辑较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `4935c071298824ad29631039ab8fa6a7c6fb0cbb` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round20_exclusions.tsv`，共 1422 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:incomplete_or_uncommon_label`：245 条。
- `duplicate:local_core_zh_pinyin.tsv`：838 条。
- `quality:incomplete_or_redundant_label`：279 条。
- `duplicate:standard_zh_pinyin.tsv`：40 条。
- `duplicate:batch318`：2 条。
- `duplicate:batch319`：2 条。
- `batch_size_limit`：15 条。
- `duplicate:batch320`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1180 个含多音字的词在各批复核表逐字记录；24 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 安全：执行的行 xing，重新、重置、重启的重 chong；账户冻结、解冻和解除授权按完整语境复核。只记录防骗生活用语，不导入任何组织名或具体软件名。
- 智能家居：重新配网、设备重连的重 chong；行程的行 xing；调光、调色、调节的调 tiao；窗帘校准的校 jiao。传感器、门锁、场景按日常设置语境。
- 人工智能：语调、色调的调 diao，调节与调整的调 tiao；上下文长度的长 chang。只收可理解的通用名词与操作短语，不收产品名、商标和临时宣传词。
- 剪辑：校对、校正的校 jiao，重做、重连、重绘的重 chong，重拍点中重 zhong；调色 tiao，色调 diao；蒙版 meng，抠像 kou，帧 zhen。轻声按本字写。
- 泄露中的露 lu；银行的行 hang；图片转文字的转 zhuan。字幕分行的行 hang；语调调节两个调依次 diao、tiao；工具调用的调 diao，肤色调整的调 tiao，避免跨词边界误匹配。无级调色采用“级”的规范写法。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1317/1317 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，223596 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round20_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.10% → 98.10% |
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
- `docs/lexicon-keep-adding-round20.md`
- `resources/dictionary/expansion/batch318_authored_draft.txt`
- `resources/dictionary/expansion/batch318_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch319_authored_draft.txt`
- `resources/dictionary/expansion/batch319_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch320_authored_draft.txt`
- `resources/dictionary/expansion/batch320_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch321_authored_draft.txt`
- `resources/dictionary/expansion/batch321_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch322_draft.txt`
- `resources/dictionary/expansion/batch322_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch323_draft.txt`
- `resources/dictionary/expansion/batch323_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch324_draft.txt`
- `resources/dictionary/expansion/batch324_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch325_draft.txt`
- `resources/dictionary/expansion/batch325_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_318.md`
- `resources/dictionary/expansion/batch_319.md`
- `resources/dictionary/expansion/batch_320.md`
- `resources/dictionary/expansion/batch_321.md`
- `resources/dictionary/expansion/batch_322.md`
- `resources/dictionary/expansion/batch_323.md`
- `resources/dictionary/expansion/batch_324.md`
- `resources/dictionary/expansion/batch_325.md`
- `resources/dictionary/expansion/lexicon_batch_318.applied.json`
- `resources/dictionary/expansion/lexicon_batch_318.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_318.json`
- `resources/dictionary/expansion/lexicon_batch_318.tsv`
- `resources/dictionary/expansion/lexicon_batch_318.validation.json`
- `resources/dictionary/expansion/lexicon_batch_319.applied.json`
- `resources/dictionary/expansion/lexicon_batch_319.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_319.json`
- `resources/dictionary/expansion/lexicon_batch_319.tsv`
- `resources/dictionary/expansion/lexicon_batch_319.validation.json`
- `resources/dictionary/expansion/lexicon_batch_320.applied.json`
- `resources/dictionary/expansion/lexicon_batch_320.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_320.json`
- `resources/dictionary/expansion/lexicon_batch_320.tsv`
- `resources/dictionary/expansion/lexicon_batch_320.validation.json`
- `resources/dictionary/expansion/lexicon_batch_321.applied.json`
- `resources/dictionary/expansion/lexicon_batch_321.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_321.json`
- `resources/dictionary/expansion/lexicon_batch_321.tsv`
- `resources/dictionary/expansion/lexicon_batch_321.validation.json`
- `resources/dictionary/expansion/lexicon_batch_322.json`
- `resources/dictionary/expansion/lexicon_batch_322.tsv`
- `resources/dictionary/expansion/lexicon_batch_323.json`
- `resources/dictionary/expansion/lexicon_batch_323.tsv`
- `resources/dictionary/expansion/lexicon_batch_324.json`
- `resources/dictionary/expansion/lexicon_batch_324.tsv`
- `resources/dictionary/expansion/lexicon_batch_325.json`
- `resources/dictionary/expansion/lexicon_batch_325.tsv`
- `resources/dictionary/expansion/probes/batch318_probe.tsv`
- `resources/dictionary/expansion/probes/batch319_probe.tsv`
- `resources/dictionary/expansion/probes/batch320_probe.tsv`
- `resources/dictionary/expansion/probes/batch321_probe.tsv`
- `resources/dictionary/expansion/probes/batch322_probe.tsv`
- `resources/dictionary/expansion/probes/batch323_probe.tsv`
- `resources/dictionary/expansion/probes/batch324_probe.tsv`
- `resources/dictionary/expansion/probes/batch325_probe.tsv`
- `resources/dictionary/expansion/round20_exclusions.tsv`
- `resources/dictionary/expansion/round20_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
