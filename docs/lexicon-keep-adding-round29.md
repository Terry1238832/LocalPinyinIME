# 词库续加第29轮（批次 390–397）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `f1cefe641fad86cfbccd82423171801334b9694c`，引擎版本 1.2.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：234937 → 235534，新增 **597 条**。
- 权重 6000 / 5500 / 5000：0 / 596 / 1 条。
- 最终 SHA-256：`C1A1455986708F00BEE84EE7B13D9B2C26AE59B556ED75B0E6515F0C3346AA74`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：A。实际打开饮食批次 150、宠物批次 181、园艺批次 218 和电脑网络批次 239 的 TSV，发现既有内容偏菜名食材、宠物用品与品种、栽培材料和通用电脑设置；本轮分别补凉菜腌拌操作、猫犬行为与牵绳互动、换盆及水肥状态、家庭打印扫描故障。只依据生活常识逐条拟写，再对全部历史词面去重，不依据测评缺词选词。短词每批 100–400 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 390 | 家常凉菜与腌拌处理 | curate | 133 | 0 | 133 | 0 | 93.23% | 100.00% | `0f33f56debddf9437ce6b53966a108d9a6116251` |
| 391 | 猫犬行为与遛宠互动 | curate | 132 | 0 | 131 | 1 | 91.67% | 100.00% | `489d0b6400954633d42718b0a5238b5312aee3ba` |
| 392 | 盆栽换盆与水肥状态 | curate | 145 | 0 | 145 | 0 | 92.41% | 100.00% | `3867f0f962b37ac0181f4bc7848068ee7a8aee4c` |
| 393 | 家庭打印扫描与纸张故障 | curate | 147 | 0 | 147 | 0 | 82.99% | 100.00% | `b53271dfa173407c52a9e9424c884fe708241569` |
| 394 | 家常凉菜与腌拌处理较长说法 | merge | 10 | 0 | 10 | 0 | 40.00% | 100.00% | `7eb1ed22702c22d02aaf73e4d961e0ac2f4060b5` |
| 395 | 猫犬行为与遛宠互动较长说法 | merge | 10 | 0 | 10 | 0 | 40.00% | 100.00% | `b2804f0adb623c86b1d56f12ee84b53066ed515d` |
| 396 | 盆栽换盆与水肥状态较长说法 | merge | 10 | 0 | 10 | 0 | 60.00% | 100.00% | `3f6cb86096730b2c6a7829dc7a914dad9f533afb` |
| 397 | 家庭打印扫描与纸张故障较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `0b03946e6621926376af24281141e98ef6faab62` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round29_exclusions.tsv`，共 228 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：137 条。
- `quality: fragmentary, less natural or unnecessarily repetitive phrasing`：91 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

520 个含多音字的词在各批复核表逐字记录；11 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

凉拌中的拌取 ban，泡发的发取 fa；猫犬尾巴的尾取 wei、爪子取 zhua、长牵引绳取 chang；园艺长叶和长势取 zhang，节间变长取 chang，少量多次的量取 liang；打印长边取 chang、短边取 duan，纸张的纸取 zhi。全部词条逐条核对全拼，含多音字的词逐字记录。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 419/419 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，235534 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round29_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.97% → 85.97% | +0.00 | 98.11% → 98.11% |
| 逐词输入·无前文 | 20626 | 77.98% → 77.98% | +0.00 | 97.19% → 97.19% |
| 留出整句 | 4126 | 58.07% → 58.07% | +0.00 | 73.44% → 73.44% |
| 口语对话·整句 | 3368 | 80.40% → 80.40% | +0.00 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round29.md`
- `resources/dictionary/expansion/batch390_authored_draft.txt`
- `resources/dictionary/expansion/batch390_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch391_authored_draft.txt`
- `resources/dictionary/expansion/batch391_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch392_authored_draft.txt`
- `resources/dictionary/expansion/batch392_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch393_authored_draft.txt`
- `resources/dictionary/expansion/batch393_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch394_draft.txt`
- `resources/dictionary/expansion/batch394_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch395_draft.txt`
- `resources/dictionary/expansion/batch395_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch396_draft.txt`
- `resources/dictionary/expansion/batch396_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch397_draft.txt`
- `resources/dictionary/expansion/batch397_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_390.md`
- `resources/dictionary/expansion/batch_391.md`
- `resources/dictionary/expansion/batch_392.md`
- `resources/dictionary/expansion/batch_393.md`
- `resources/dictionary/expansion/batch_394.md`
- `resources/dictionary/expansion/batch_395.md`
- `resources/dictionary/expansion/batch_396.md`
- `resources/dictionary/expansion/batch_397.md`
- `resources/dictionary/expansion/lexicon_batch_390.applied.json`
- `resources/dictionary/expansion/lexicon_batch_390.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_390.json`
- `resources/dictionary/expansion/lexicon_batch_390.tsv`
- `resources/dictionary/expansion/lexicon_batch_390.validation.json`
- `resources/dictionary/expansion/lexicon_batch_391.applied.json`
- `resources/dictionary/expansion/lexicon_batch_391.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_391.json`
- `resources/dictionary/expansion/lexicon_batch_391.tsv`
- `resources/dictionary/expansion/lexicon_batch_391.validation.json`
- `resources/dictionary/expansion/lexicon_batch_392.applied.json`
- `resources/dictionary/expansion/lexicon_batch_392.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_392.json`
- `resources/dictionary/expansion/lexicon_batch_392.tsv`
- `resources/dictionary/expansion/lexicon_batch_392.validation.json`
- `resources/dictionary/expansion/lexicon_batch_393.applied.json`
- `resources/dictionary/expansion/lexicon_batch_393.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_393.json`
- `resources/dictionary/expansion/lexicon_batch_393.tsv`
- `resources/dictionary/expansion/lexicon_batch_393.validation.json`
- `resources/dictionary/expansion/lexicon_batch_394.json`
- `resources/dictionary/expansion/lexicon_batch_394.tsv`
- `resources/dictionary/expansion/lexicon_batch_395.json`
- `resources/dictionary/expansion/lexicon_batch_395.tsv`
- `resources/dictionary/expansion/lexicon_batch_396.json`
- `resources/dictionary/expansion/lexicon_batch_396.tsv`
- `resources/dictionary/expansion/lexicon_batch_397.json`
- `resources/dictionary/expansion/lexicon_batch_397.tsv`
- `resources/dictionary/expansion/probes/batch390_probe.tsv`
- `resources/dictionary/expansion/probes/batch391_probe.tsv`
- `resources/dictionary/expansion/probes/batch392_probe.tsv`
- `resources/dictionary/expansion/probes/batch393_probe.tsv`
- `resources/dictionary/expansion/probes/batch394_probe.tsv`
- `resources/dictionary/expansion/probes/batch395_probe.tsv`
- `resources/dictionary/expansion/probes/batch396_probe.tsv`
- `resources/dictionary/expansion/probes/batch397_probe.tsv`
- `resources/dictionary/expansion/round29_exclusions.tsv`
- `resources/dictionary/expansion/round29_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
