# 词库续加第33轮（自查批次 230–413；本轮未新增批次）

本轮方向：C。

在独立副本 `localpinyinime-lexicon`、分支 `lexicon/continuous` 工作。开始前 fetch 并合并最新 `origin/main`，无冲突；起点 `3cba17851ca2a8ab78fc067f2934f956ff351304`，版本 1.2.0。按用户恢复指令，连续两轮新增量的停止阈值现为 300 条，自查轮不计入。

- `local_core`：236787 → 236787；新增 0、删除 0、修正读音 5 条；五条均保留 5500 权重。上限 255390 未修改。
- 自查没有占用新批次号；下一批仍从 414 开始。
- 结构检查覆盖批次 230–413 的 184 批、35394 条：跨批/与基线词面重复 0，同音权重违规 0。
- 完整词面与自然度巡检为批次 230–237、1704 条；另对批次 230–277 的高风险字作语境筛查，并对本次发现的“弹窗”在所有已写批次中定向追查。没有声称全部 35394 条都完成新的穷尽式语义审校。
- 一名 AI 作者复核，不声称独立人工语言学审校；未看评测缺词选词，没有新增词面。

## 修正清单

| 批次 | 词 | 全拼 | 权重 | 原因 |
| --- | --- | --- | --- | --- |
| 231 | 权限弹窗 | `quanxiandanchuang` → `quanxiantanchuang` | 5500 → 5500 | 弹出窗口，弹取 tan |
| 239 | 禁止弹窗 | `jinzhidanchuang` → `jinzhitanchuang` | 5500 → 5500 | 弹出窗口，弹取 tan |
| 239 | 拦截弹窗 | `lanjiedanchuang` → `lanjietanchuang` | 5500 → 5500 | 弹出窗口，弹取 tan |
| 239 | 网站弹窗 | `wangzhandanchuang` → `wangzhantanchuang` | 5500 → 5500 | 弹出窗口，弹取 tan |
| 318 | 恶意弹窗 | `eyidanchuang` → `eyitanchuang` | 5500 → 5500 | 弹出窗口，弹取 tan |

单独纠错提交：`6efc6671aac9ef5feb0b7d2c0d6f73cccbdd0d36`，`fix(lexicon): correct round 9–20 entries`。音乐“大调/小调”的调取 diao、饮品“重发酵”的重取 zhong、小说“成长线”的长取 zhang，语境正确，未改动。

同步修改 canonical、对应批次 TSV/JSON、逐字读音复核、probe 和批次说明。复核表明确弹=tan，未照搬默认 dan。当前 TSV 与作者文件哈希重新核对，`curate evaluate` 重新运行；未重放历史 prepare/apply。原 `.applied.json` 的冻结哈希、canonical 前后哈希和原追加时间保留历史含义，当前修订另外标记在 `current_batch_revision`。旧追加回执不是本次纠错的新 apply 凭证。修正前后文件哈希及改动逐条见 `resources/dictionary/expansion/round33_corrections.json`，Git 历史保留原作者记录。

除上列五个拼音键外，canonical 其余行、词面和权重逐行保留；批次 230 之前全部不变。来源与权重策略不变，sources 注册表未变。未改引擎、界面、安装程序、版本号、domains 或 tests/data；未打 tag、未打包。

## 原生验收

- x64 Release 构建与 ctest：44/44 通过。
- x86 Release 构建与 ctest：44/44 通过。
- 修改过的三批全部 probe 使用 `--no-context --scores` 运行；本轮五个修正词都是 4 字，5/5 在前九。历史未改词缺失如有，列在验证 JSON 的 `unchanged_missing`，不隐去统计。
- 三批 `curate evaluate` 重新运行，当前资源哈希与 canonical 一致；`validate_local_core_lexicon.py --max-entries 255390` 通过。
- 十组 `accuracy_snapshot.py` 本轮前后分别运行，各组首选率下降不超过 0.2 个百分点，未修改测试集。
- 覆盖率在修正冻结后测量，只作测量，更新 docs/LEXICON_COVERAGE.md 与 docs/lexicon-coverage.json。

| 历史批次 | 全批用例 | 首选率 | 前九率 | 本轮修正词前九 |
| --- | ---: | ---: | ---: | ---: |
| 231 | 347 | 89.91% | 100.00% | 1/1 |
| 239 | 320 | 89.69% | 100.00% | 3/3 |
| 318 | 338 | 96.15% | 100.00% | 1/1 |

未运行：新增批次的 prepare/freeze/apply/merge（本轮无新增批次）；完整 Build.ps1 的 Electron 和安装包打包段（任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。没有撤回整批；纠错前后表中旧 dan 键仅作历史记录，不作为新别名保留。

## 准确率（修正前 → 后）

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
- `docs/lexicon-keep-adding-round33.md`
- `docs/lexicon-stage2-summary.md`
- `resources/dictionary/expansion/batch231_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch239_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch318_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_231.md`
- `resources/dictionary/expansion/batch_239.md`
- `resources/dictionary/expansion/batch_318.md`
- `resources/dictionary/expansion/lexicon_batch_231.applied.json`
- `resources/dictionary/expansion/lexicon_batch_231.json`
- `resources/dictionary/expansion/lexicon_batch_231.tsv`
- `resources/dictionary/expansion/lexicon_batch_231.validation.json`
- `resources/dictionary/expansion/lexicon_batch_239.applied.json`
- `resources/dictionary/expansion/lexicon_batch_239.json`
- `resources/dictionary/expansion/lexicon_batch_239.tsv`
- `resources/dictionary/expansion/lexicon_batch_239.validation.json`
- `resources/dictionary/expansion/lexicon_batch_318.applied.json`
- `resources/dictionary/expansion/lexicon_batch_318.json`
- `resources/dictionary/expansion/lexicon_batch_318.tsv`
- `resources/dictionary/expansion/lexicon_batch_318.validation.json`
- `resources/dictionary/expansion/probes/batch231_probe.tsv`
- `resources/dictionary/expansion/probes/batch239_probe.tsv`
- `resources/dictionary/expansion/probes/batch318_probe.tsv`
- `resources/dictionary/expansion/round33_corrections.json`
- `resources/dictionary/expansion/round33_exclusions.tsv`
- `resources/dictionary/expansion/round33_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
