# 词库续加第16轮（批次 286–293）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `67cef8641e7bf1c96e14711f586aae37b3e360a9`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：214332 → 216403，新增 **2071 条**。
- 权重 6000 / 5500 / 5000：0 / 2061 / 10 条。
- 最终 SHA-256：`8462B4650277A61827FD10988F714C8CB52E1B11FBE71D50D6F21C14F47BC914`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 286 | 环保与能源 | curate | 500 | 0 | 499 | 1 | 93.00% | 100.00% | `f17edffea08a488c401b1443d60201c5c4e74397` |
| 287 | 新能源车与充电 | curate | 491 | 0 | 491 | 0 | 88.59% | 100.00% | `faff55a4b9da032fe428934f1c6eaaa46b32970e` |
| 288 | 舞蹈 | curate | 500 | 0 | 496 | 4 | 92.60% | 100.00% | `37ab320f17bb57621682f9904f4a3cf1166f1151` |
| 289 | 戏曲与曲艺 | curate | 500 | 0 | 495 | 5 | 87.80% | 100.00% | `8eb6eda7b12b229db7088694cf5a673c11003898` |
| 290 | 环保与能源较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `3b8b4e19e3857b732a443f0c593a233f7976d02c` |
| 291 | 新能源车与充电较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `0355eb3e6f4463d485aa3683c4a70765c2f584f5` |
| 292 | 舞蹈较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `e418f4654174a2cb9ac1ea3bb67998103768182c` |
| 293 | 戏曲与曲艺较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `bd8a62404406ee79e099be93d66b61971b43abc1` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round16_exclusions.tsv`，共 566 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：244 条。
- `duplicate:standard_zh_pinyin.tsv`：38 条。
- `quality:incomplete_or_uncommon_label`：63 条。
- `quality:uncertain_or_uncommon_label`：28 条。
- `duplicate:batch286`：1 条。
- `outside_batch_length_scope`：2 条。
- `batch_size_limit`：181 条。
- `quality:prefer_clear_automotive_亏电_wording`：4 条。
- `quality:prefer_generic_unbranded_快充站`：1 条。
- `quality:specific_performance_title_omit`：1 条。
- `authoring_checker:contextual_lao_not_listed_by_pypinyin_0.55.0`：1 条。
- `duplicate:batch288`：2 条。

冻结前固定作者校验器拒绝“乐亭大鼓”的语境读音 lao（字符表仅 le/yue）；未改读音、未改工具，整词记入排除表，并在临时副本重建全部未提交批次及哈希链。未因准确率撤回整批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1478 个含多音字的词在各批复核表逐字记录；8 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 环保能源：电网调峰、调频、负荷调节的调 `tiao`；重污染天气、重力储能的重 `zhong`；出行和运行的行 `xing`；绿地、绿氢的绿 `lv`，空气滤网的滤 `lv`。
- 新能源车：纯电长途、充电时长和延长线的长 `chang`；电量校准的校 `jiao`；空调、预调温的调 `tiao`；导航的行 `hang`，行驶、蠕行、滑行的行 `xing`。汽车低电量用较明确的“亏电”词面；不修改历史词条。
- 舞蹈：长扇舞、长绸舞的长 `chang`；找重拍和重心的重 `zhong`；脚掌着地、脚跟着地的着 `zhuo`。对称、不对称的称 `chen`；舞台调度的调 `diao`，站位调整、肢体协调的调 `tiao`。
- 戏曲曲艺：行当的行 `hang`，行腔的行 `xing`；曲调、腔调、时调的调 `diao`；弹词、评弹、乱弹腔的弹 `tan`。乐亭大鼓需乐 `lao`，但固定作者工具不列此读音，因此整词剔除，不改作 le/yue；乐队、乐师、配乐和打击乐的乐 `yue`；藏戏的藏 `zang`。独角戏用 `dujiaoxi`；轻声仍按本字写，如折子戏 `zhezixi`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1697/1697 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，216403 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round16_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round16.md`
- `resources/dictionary/expansion/batch286_authored_draft.txt`
- `resources/dictionary/expansion/batch286_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch287_authored_draft.txt`
- `resources/dictionary/expansion/batch287_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch288_authored_draft.txt`
- `resources/dictionary/expansion/batch288_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch289_authored_draft.txt`
- `resources/dictionary/expansion/batch289_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch290_draft.txt`
- `resources/dictionary/expansion/batch290_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch291_draft.txt`
- `resources/dictionary/expansion/batch291_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch292_draft.txt`
- `resources/dictionary/expansion/batch292_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch293_draft.txt`
- `resources/dictionary/expansion/batch293_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_286.md`
- `resources/dictionary/expansion/batch_287.md`
- `resources/dictionary/expansion/batch_288.md`
- `resources/dictionary/expansion/batch_289.md`
- `resources/dictionary/expansion/batch_290.md`
- `resources/dictionary/expansion/batch_291.md`
- `resources/dictionary/expansion/batch_292.md`
- `resources/dictionary/expansion/batch_293.md`
- `resources/dictionary/expansion/lexicon_batch_286.applied.json`
- `resources/dictionary/expansion/lexicon_batch_286.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_286.json`
- `resources/dictionary/expansion/lexicon_batch_286.tsv`
- `resources/dictionary/expansion/lexicon_batch_286.validation.json`
- `resources/dictionary/expansion/lexicon_batch_287.applied.json`
- `resources/dictionary/expansion/lexicon_batch_287.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_287.json`
- `resources/dictionary/expansion/lexicon_batch_287.tsv`
- `resources/dictionary/expansion/lexicon_batch_287.validation.json`
- `resources/dictionary/expansion/lexicon_batch_288.applied.json`
- `resources/dictionary/expansion/lexicon_batch_288.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_288.json`
- `resources/dictionary/expansion/lexicon_batch_288.tsv`
- `resources/dictionary/expansion/lexicon_batch_288.validation.json`
- `resources/dictionary/expansion/lexicon_batch_289.applied.json`
- `resources/dictionary/expansion/lexicon_batch_289.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_289.json`
- `resources/dictionary/expansion/lexicon_batch_289.tsv`
- `resources/dictionary/expansion/lexicon_batch_289.validation.json`
- `resources/dictionary/expansion/lexicon_batch_290.json`
- `resources/dictionary/expansion/lexicon_batch_290.tsv`
- `resources/dictionary/expansion/lexicon_batch_291.json`
- `resources/dictionary/expansion/lexicon_batch_291.tsv`
- `resources/dictionary/expansion/lexicon_batch_292.json`
- `resources/dictionary/expansion/lexicon_batch_292.tsv`
- `resources/dictionary/expansion/lexicon_batch_293.json`
- `resources/dictionary/expansion/lexicon_batch_293.tsv`
- `resources/dictionary/expansion/probes/batch286_probe.tsv`
- `resources/dictionary/expansion/probes/batch287_probe.tsv`
- `resources/dictionary/expansion/probes/batch288_probe.tsv`
- `resources/dictionary/expansion/probes/batch289_probe.tsv`
- `resources/dictionary/expansion/probes/batch290_probe.tsv`
- `resources/dictionary/expansion/probes/batch291_probe.tsv`
- `resources/dictionary/expansion/probes/batch292_probe.tsv`
- `resources/dictionary/expansion/probes/batch293_probe.tsv`
- `resources/dictionary/expansion/round16_exclusions.tsv`
- `resources/dictionary/expansion/round16_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
