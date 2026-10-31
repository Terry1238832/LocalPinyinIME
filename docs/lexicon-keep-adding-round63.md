# 词库续加第63轮（批次 600–605）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `4afdf559e98472d3140c841ebd21c9ad4322832e`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：269741 → 270895，新增 **1154 条**。
- 权重 6000 / 5500 / 5000：0 / 1154 / 0 条。
- 最终 SHA-256：`D30D34910B7E8794445602BAF06B87A1EAB5A734A3F33AB5F0BC8D69CAFC691E`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 600 | 早晚问候、起床与当天状态寒暄 | curate | 344 | 0 | 344 | 0 | 87.50% | 100.00% | `ba234e12f6010a16f03a5314062f38ef55e808f5` |
| 601 | 日常近况分享与忙闲状态接话 | curate | 375 | 0 | 375 | 0 | 84.53% | 100.00% | `1161d45ee6cda830d8a97b01c3fed70e4695c35b` |
| 602 | 日常推荐、试用体验与使用反馈聊天 | curate | 360 | 0 | 360 | 0 | 81.94% | 100.00% | `812018eca6a8769d0f2e39c9351f93b170708676` |
| 603 | 起床问候、早餐与当天安排长句 | merge | 30 | 0 | 30 | 0 | 73.33% | 90.00% | `eb6465881702f1c62e5ac938950631f26a0bda66` |
| 604 | 分享近期状态、工作与生活近况长句 | merge | 22 | 0 | 22 | 0 | 77.27% | 86.36% | `27a65c100c982b43f9913a3a627ed3f341f0788f` |
| 605 | 询问推荐、分享试用体验与购买建议长句 | merge | 23 | 0 | 23 | 0 | 65.22% | 69.57% | `5461cd7f4a1bab2ff6b6d1f78d0444dcfa3ce27a` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round63_exclusions.tsv`，共 411 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：80 条。
- `单独口号式词项较泛，不属于固定聊天短语`：1 条。
- `outside_batch_length_scope`：306 条。
- `duplicate:batch600`：5 条。
- `身体评价容易冒犯，不作为普遍聊天词条`：2 条。
- `duplicate:batch601`：5 条。
- `不自然的购物体验问法`：1 条。
- `duplicate:batch602`：2 条。
- `batch_size_limit`：5 条。
- `non_BMP_Han_or_non_Han`：3 条。
- `落下休息的搭配不自然，整句不作为常用聊天短语`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1069 个含多音字的词在各批复核表逐字记录；0 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

关键读音决定包括：还给你中‘还’读huan；睡着、着急中‘着’读zhao，看着、用着中读zhe；睡觉读jiao，觉得读jue；出差读chai，便宜读pian，差别读cha；重新读chong，看重读zhong。每个入选词中含多音字的字位均见逐批复核表。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 27/27 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，270895 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round63_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.25% → 88.25% | +0.00 | 98.70% → 98.70% |
| 逐词输入·无前文 | 20626 | 79.65% → 79.65% | +0.00 | 97.56% → 97.56% |
| 留出整句 | 4126 | 64.49% → 64.49% | +0.00 | 77.61% → 77.61% |
| 口语对话·整句 | 3368 | 85.36% → 85.36% | +0.00 | 94.69% → 94.69% |
| 口语对话·逐词 | 19504 | 95.58% → 95.58% | +0.00 | 99.72% → 99.72% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.67% → 96.67% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round63.md`
- `resources/dictionary/expansion/batch600_authored_draft.txt`
- `resources/dictionary/expansion/batch600_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch601_authored_draft.txt`
- `resources/dictionary/expansion/batch601_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch602_authored_draft.txt`
- `resources/dictionary/expansion/batch602_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch603_draft.txt`
- `resources/dictionary/expansion/batch603_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch604_draft.txt`
- `resources/dictionary/expansion/batch604_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch605_draft.txt`
- `resources/dictionary/expansion/batch605_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_600.md`
- `resources/dictionary/expansion/batch_601.md`
- `resources/dictionary/expansion/batch_602.md`
- `resources/dictionary/expansion/batch_603.md`
- `resources/dictionary/expansion/batch_604.md`
- `resources/dictionary/expansion/batch_605.md`
- `resources/dictionary/expansion/lexicon_batch_600.applied.json`
- `resources/dictionary/expansion/lexicon_batch_600.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_600.json`
- `resources/dictionary/expansion/lexicon_batch_600.tsv`
- `resources/dictionary/expansion/lexicon_batch_600.validation.json`
- `resources/dictionary/expansion/lexicon_batch_601.applied.json`
- `resources/dictionary/expansion/lexicon_batch_601.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_601.json`
- `resources/dictionary/expansion/lexicon_batch_601.tsv`
- `resources/dictionary/expansion/lexicon_batch_601.validation.json`
- `resources/dictionary/expansion/lexicon_batch_602.applied.json`
- `resources/dictionary/expansion/lexicon_batch_602.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_602.json`
- `resources/dictionary/expansion/lexicon_batch_602.tsv`
- `resources/dictionary/expansion/lexicon_batch_602.validation.json`
- `resources/dictionary/expansion/lexicon_batch_603.json`
- `resources/dictionary/expansion/lexicon_batch_603.tsv`
- `resources/dictionary/expansion/lexicon_batch_604.json`
- `resources/dictionary/expansion/lexicon_batch_604.tsv`
- `resources/dictionary/expansion/lexicon_batch_605.json`
- `resources/dictionary/expansion/lexicon_batch_605.tsv`
- `resources/dictionary/expansion/probes/batch600_probe.tsv`
- `resources/dictionary/expansion/probes/batch601_probe.tsv`
- `resources/dictionary/expansion/probes/batch602_probe.tsv`
- `resources/dictionary/expansion/probes/batch603_probe.tsv`
- `resources/dictionary/expansion/probes/batch604_probe.tsv`
- `resources/dictionary/expansion/probes/batch605_probe.tsv`
- `resources/dictionary/expansion/round63_exclusions.tsv`
- `resources/dictionary/expansion/round63_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
