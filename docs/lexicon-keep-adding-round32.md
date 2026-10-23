# 词库续加第32轮（批次 410–413）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `f6254efd66d58109ab865979ea9c80db268621d1`，引擎版本 1.2.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：236431 → 236787，新增 **356 条**。
- 权重 6000 / 5500 / 5000：0 / 356 / 0 条。
- 最终 SHA-256：`BAC5FEBBBD0B1D1B4F9081D9C54A28E7A8A3342924146E8C96B2CB2E9A556139`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：B。按第二阶段类型顺序继续劝慰鼓励、请求拒绝两类；此前第 30 轮已做客套礼貌、道歉感谢、祝福问候，均跳过。只逐条自拟自然回复与常用表达，保留边界清楚的说法，删去装饰性口号、残句及不必要的重复变体。用户本次将连续两轮新增量的停止阈值从 500 条降为 300 条，本轮起按新阈值继续；短词每批规模仍为 100–400 条。短词每批 100–400 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 410 | 劝慰与鼓励常用表达补漏 | curate | 151 | 0 | 151 | 0 | 84.77% | 100.00% | `b236943fca198c69fbec6b2c5370d2e021ad4c6d` |
| 411 | 请求与拒绝日常表达补漏 | curate | 185 | 0 | 185 | 0 | 88.65% | 100.00% | `10d55a45cc5551235f32c0b44c3da826f117dfa2` |
| 412 | 劝慰与鼓励较长表达补漏 | merge | 10 | 0 | 10 | 0 | 80.00% | 100.00% | `bb08c9546b85c8ae4bcec4dc997e099a73a1a79c` |
| 413 | 请求与拒绝较长表达补漏 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `31006bc94f8c1c7d72033af9c7a07430edb7497b` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round32_exclusions.tsv`，共 105 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：51 条。
- `quality: decorative wording, incomplete fragment or unnecessarily repeated phrasing`：31 条。
- `quality: less idiomatic wording or unnecessary reply variation`：10 条。
- `outside_batch_length_scope`：13 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

325 个含多音字的词在各批复核表逐字记录；12 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

别跟自己较劲的较取 jiao；别勉强和不用逞强的强取 qiang；累着、绷着等持续状态的着取 zhe；抽时间的抽取 chou。请求中的方便取 fang bian，我得考虑中的得取 dei，还没想好中的还取 hai、没取 mei，重新开始的重取 chong。逐条复核全拼，不照搬默认读音。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 14/14 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，236787 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round32_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round32.md`
- `docs/lexicon-stage2-summary.md`
- `resources/dictionary/expansion/batch410_authored_draft.txt`
- `resources/dictionary/expansion/batch410_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch411_authored_draft.txt`
- `resources/dictionary/expansion/batch411_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch412_draft.txt`
- `resources/dictionary/expansion/batch412_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch413_draft.txt`
- `resources/dictionary/expansion/batch413_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_410.md`
- `resources/dictionary/expansion/batch_411.md`
- `resources/dictionary/expansion/batch_412.md`
- `resources/dictionary/expansion/batch_413.md`
- `resources/dictionary/expansion/lexicon_batch_410.applied.json`
- `resources/dictionary/expansion/lexicon_batch_410.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_410.json`
- `resources/dictionary/expansion/lexicon_batch_410.tsv`
- `resources/dictionary/expansion/lexicon_batch_410.validation.json`
- `resources/dictionary/expansion/lexicon_batch_411.applied.json`
- `resources/dictionary/expansion/lexicon_batch_411.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_411.json`
- `resources/dictionary/expansion/lexicon_batch_411.tsv`
- `resources/dictionary/expansion/lexicon_batch_411.validation.json`
- `resources/dictionary/expansion/lexicon_batch_412.json`
- `resources/dictionary/expansion/lexicon_batch_412.tsv`
- `resources/dictionary/expansion/lexicon_batch_413.json`
- `resources/dictionary/expansion/lexicon_batch_413.tsv`
- `resources/dictionary/expansion/probes/batch410_probe.tsv`
- `resources/dictionary/expansion/probes/batch411_probe.tsv`
- `resources/dictionary/expansion/probes/batch412_probe.tsv`
- `resources/dictionary/expansion/probes/batch413_probe.tsv`
- `resources/dictionary/expansion/round32_exclusions.tsv`
- `resources/dictionary/expansion/round32_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
