# 词库续加第九轮（批次 230–237）

从最新 `main` 的 `e4f4e4c175117d0eb3a8bd731b73cc26958b84da`（0.24.1）开始。音乐与乐器放在第一批，另补手机与 App 操作、求职招聘与劳动、旅游与酒店住宿。四个主题各有一批 2–6 字词和一批 7 字以上较长说法。

- 仓库：https://github.com/Terry1238832/LocalPinyinIME
- 目标分支：`lexicon/keep-adding-round9`，向 `main` 提交 PR，不直接推送 main。
- `local_core`：201393 → 203097，新增 **1704 条**。6000 / 5500 / 5000 分别为 **276 / 1409 / 19 条**。
- 最终 SHA-256：`9EB7DCA1225CF5DBA233858BC118223616D6FADAFFA521C57ADD1A5CA2B91EF9`。
- 未超过 `--max-entries 209495`，未改 CMake 上限。版本号、引擎、界面、安装程序、场景词表和测试集均未改；未打 tag，未打包。
- 原工作目录在执行期间出现其他未提交改动，最终验收与提交使用独立检出，不包含这些改动。

## 批次

2–6 字走 `curate_lexicon.py` prepare → 写逐字读音复核 → freeze → apply → evaluate；pypinyin 0.55.0 只用作作者侧工具。7 字以上另批使用 `merge_lexicon_batch.py`。来源均为 `first_party_individually_authored`，分别登记为 `first_party_lexicon_batch_230` 至 `first_party_lexicon_batch_237`。权重为预先选择的项目相对粗档，不是第三方频率。

| 批次 | 主题 | 工具 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 230 | 音乐与乐器 | curate | 489 | 242 | 240 | 7 | 85.28% | 100.00% | `c663631f488597c6eeaea39776061d2627474a0a` |
| 231 | 手机与应用操作 | curate | 347 | 2 | 345 | 0 | 89.91% | 100.00% | `3597c7c38bef7feec0307b2ba4625ce2fd2ffb30` |
| 232 | 求职招聘与劳动 | curate | 394 | 7 | 386 | 1 | 92.64% | 100.00% | `3bf63346be3e6cc3d0739a15e060e0f6e352c0b5` |
| 233 | 旅游与酒店住宿 | curate | 389 | 25 | 360 | 4 | 92.80% | 100.00% | `ba1eee6ec51fdf593f539bdb952bd9a45c9c3c52` |
| 234 | 音乐与乐器较长说法 | merge | 20 | 0 | 14 | 6 | 70.00% | 100.00% | `069c6992bbf3faab4d0e9a960742f4fa8b9567e7` |
| 235 | 手机与应用操作较长说法 | merge | 23 | 0 | 23 | 0 | 69.57% | 100.00% | `3017172dc229a50b3ccf5524ea4a71dfba1ceb71` |
| 236 | 求职招聘与劳动较长说法 | merge | 21 | 0 | 20 | 1 | 61.90% | 100.00% | `ca87313e7fafda0b7890d1177a2bb3e43edf65f3` |
| 237 | 旅游与酒店住宿较长说法 | merge | 21 | 0 | 21 | 0 | 90.48% | 100.00% | `18a6ab2e4cf608eab7604c736bdba9dbad1e380a` |

四个主题含长词分别为 509、370、415、410 条。每批原始添加各一个提交；另有修正提交 `68eb418`，修正本轮新词读音并在临时副本中重跑相关冻结、合并及下游哈希链。原提交历史保留，最终冻结文件和记录均已重新校验。最后的文档提交包含本报告及原生验收记录。

## 去掉的词

共逐条拟写 2622 个候选，保留 1704，去掉 918。按词面检查 standard、core、local_core、全部 domains 和本轮先前词面；其中与 local_core 重复 884 条，与 standard 重复 28 条，与 core、domains 或本轮其他批次重复为 0。完整逐词清单见 `resources/dictionary/expansion/round9_exclusions.tsv`。

质量不收 6 条：竹笛曲笛、竹笛梆笛、评价返现投诉属于不自然拼接；前奏间奏是两个术语的拼接；男高声部、女高声部为不够规范的缩略表达。没有为凑数机械生成词语组合，没有导入第三方词库，没有从 `tests/data/` 取词。工具只将测试文件用作整句排除、哈希绑定及评估，未据评测缺词补词。

全部新词均在 BMP 范围，没有 U+20000 及以后的字。已有 201393 行按顺序逐行比对，词条和权重全部保留。权重遵守同全拼已有 ≥5000 词时不高于最低档：音乐 26 条、旅游住宿 3 条因此降档，其他批次无需降档。

## 读音

所有含 pypinyin 多读音字符的词均逐字登记选择，合计 1265 条复核记录；不同于默认全拼的语义修正分别为 14 / 6 / 3 / 6 / 1 / 0 / 0 / 0 条。不是直接照搬默认读音，也不声称独立人工语言学审校。

- 音乐：重奏、重升记号、重降记号的重 `chong`；配重、重锤、重音的重 `zhong`。调号、大小调、移调、变调夹的调 `diao`，调音、调琴弦、调鼓、微调的调 `tiao`。弹琴、指弹、联弹、回弹用 `tan`，手机弹窗用 `dan`。校音器的校 `jiao`，哨卡的卡 `ka`。音乐和乐器相关的乐 `yue`。
- 手机：重置 `chongzhi`；长截屏、长按、步长的长 `chang`，家长的长 `zhang`。相机／麦克风调用的调 `diao`。还原的还 `huan`。刷新率、帧率、分辨率的率 `lv`。
- 招聘劳动：背调、调班、调岗、调动用 `diao`；调休、调薪、调整用 `tiao`。出差的差 `chai`，差额、补差的差 `cha`。工资卡开户行、银行卡的行 `hang`。归还的还 `huan`。
- 旅游：索道上行、下行、同行人、行李、旅行的行 `xing`；长住、长包的长 `chang`。更换、变更用 `geng`。轻声按本字写，如拍子 `paizi`、枕头 `zhentou`。
- 引擎 üe 使用 `lve` / `nve`。本轮没有需要 üe 的新词，也没有引入 `lue` / `nue` 键。

## 验收

已运行：

1. 独立 Windows 检出完成 x64、x86 的 CMake 配置、Release 原生构建与全部测试，两种架构均 **44/44 通过**。采用 `Build.ps1` 中的原生步骤与 `/Z7` 配置；完整脚本包含 Electron 和安装包打包，按本任务“不打包”的要求未执行打包段。
2. 四个短词批次 prepare、freeze、apply、evaluate 均完成；四个长词批次 merge 完成。每批均运行 `LocalPinyinImeEval.exe .build/bin .../batchNNN_probe.tsv --no-context --scores`。所有 1704 个词均在前九；其中二至四字 1300 个全部在前九，无需按评测名次调权重。
3. `validate_local_core_lexicon.py --max-entries 209495` 通过，203097 条；另核对全部冻结哈希、来源策略、权重策略、旧行不变、全层词面去重、拼音格式、逐字读音和 probe 一致性、UTF-8 无 BOM / LF。
4. `accuracy_snapshot.py` 对最新 main 补词前和最终词库各运行一次，10 组首选率下降均不超过 0.2 个百分点，未改测试集。结果和各批摘要见 `resources/dictionary/expansion/round9_validation.json`，短词批次还保留工具生成的 `.validation.json`。
5. `report_lexicon_coverage.py` 使用官方 Unihan 17.0.0 和 jieba 0.42.1 重测，更新 `docs/LEXICON_COVERAGE.md` 与 `docs/lexicon-coverage.json`。三层 249975 个拼音词面组合；留出句词次／词种覆盖 84.01% / 71.53%。测量在冻结后运行，没有据此补词。
6. 按旧任务文档重测 19 个抽查词（下表）。竖式仍只在数学场景进入前九，这是已有情况，本轮未改它。

未运行：完整 `Build.ps1` 的打包段；未改场景词表，因此未重跑六场景回归或更新场景指标；仓库外 Tatoeba 日常句子和中文维基新闻未重测。此次原生验证已运行。

### 准确率（同为 0.24.1 引擎，补词前 → 后）

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

### 旧 19 词抽查（0 代表不在前九）

| 词 | 无场景名次 | 对应场景 | 场景名次 |
| --- | ---: | --- | ---: |
| 式子 | 8 | math | 1 |
| 算式 | 2 | math | 1 |
| 根号 | 1 | math | 1 |
| 竖式 | 0 | math | 1 |
| 演草纸 | 1 | math | 1 |
| 三角板 | 1 | math | 1 |
| 甩锅 | 1 | work | 1 |
| 背锅 | 3 | work | 1 |
| 内卷 | 1 | daily | 1 |
| 躺平 | 1 | daily | 1 |
| 社死 | 1 | daily | 1 |
| 开黑 | 1 | games | 1 |
| 上分 | 1 | games | 1 |
| 掉分 | 1 | games | 1 |
| 坑队友 | 1 | games | 1 |
| 抽卡 | 1 | games | 1 |
| 氪金 | 1 | games | 1 |
| 刷怪 | 1 | games | 1 |
| 锅子 | 3 | daily | 3 |

旧任务第二轮 B 节的 101 个场景词本轮不再迁移，按现有主词库／场景词表保留。本轮只扩充指定四个新主题。

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round9.md`
- `resources/dictionary/expansion/batch230_authored_draft.txt`
- `resources/dictionary/expansion/batch230_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch231_authored_draft.txt`
- `resources/dictionary/expansion/batch231_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch232_authored_draft.txt`
- `resources/dictionary/expansion/batch232_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch233_authored_draft.txt`
- `resources/dictionary/expansion/batch233_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch234_draft.txt`
- `resources/dictionary/expansion/batch234_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch235_draft.txt`
- `resources/dictionary/expansion/batch235_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch236_draft.txt`
- `resources/dictionary/expansion/batch236_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch237_draft.txt`
- `resources/dictionary/expansion/batch237_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_230.md`
- `resources/dictionary/expansion/batch_231.md`
- `resources/dictionary/expansion/batch_232.md`
- `resources/dictionary/expansion/batch_233.md`
- `resources/dictionary/expansion/batch_234.md`
- `resources/dictionary/expansion/batch_235.md`
- `resources/dictionary/expansion/batch_236.md`
- `resources/dictionary/expansion/batch_237.md`
- `resources/dictionary/expansion/lexicon_batch_230.applied.json`
- `resources/dictionary/expansion/lexicon_batch_230.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_230.json`
- `resources/dictionary/expansion/lexicon_batch_230.tsv`
- `resources/dictionary/expansion/lexicon_batch_230.validation.json`
- `resources/dictionary/expansion/lexicon_batch_231.applied.json`
- `resources/dictionary/expansion/lexicon_batch_231.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_231.json`
- `resources/dictionary/expansion/lexicon_batch_231.tsv`
- `resources/dictionary/expansion/lexicon_batch_231.validation.json`
- `resources/dictionary/expansion/lexicon_batch_232.applied.json`
- `resources/dictionary/expansion/lexicon_batch_232.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_232.json`
- `resources/dictionary/expansion/lexicon_batch_232.tsv`
- `resources/dictionary/expansion/lexicon_batch_232.validation.json`
- `resources/dictionary/expansion/lexicon_batch_233.applied.json`
- `resources/dictionary/expansion/lexicon_batch_233.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_233.json`
- `resources/dictionary/expansion/lexicon_batch_233.tsv`
- `resources/dictionary/expansion/lexicon_batch_233.validation.json`
- `resources/dictionary/expansion/lexicon_batch_234.json`
- `resources/dictionary/expansion/lexicon_batch_234.tsv`
- `resources/dictionary/expansion/lexicon_batch_235.json`
- `resources/dictionary/expansion/lexicon_batch_235.tsv`
- `resources/dictionary/expansion/lexicon_batch_236.json`
- `resources/dictionary/expansion/lexicon_batch_236.tsv`
- `resources/dictionary/expansion/lexicon_batch_237.json`
- `resources/dictionary/expansion/lexicon_batch_237.tsv`
- `resources/dictionary/expansion/probes/batch230_probe.tsv`
- `resources/dictionary/expansion/probes/batch231_probe.tsv`
- `resources/dictionary/expansion/probes/batch232_probe.tsv`
- `resources/dictionary/expansion/probes/batch233_probe.tsv`
- `resources/dictionary/expansion/probes/batch234_probe.tsv`
- `resources/dictionary/expansion/probes/batch235_probe.tsv`
- `resources/dictionary/expansion/probes/batch236_probe.tsv`
- `resources/dictionary/expansion/probes/batch237_probe.tsv`
- `resources/dictionary/expansion/round9_exclusions.tsv`
- `resources/dictionary/expansion/round9_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
