# 词库续加第53轮（批次 538–543）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `7bac7074bf0b5a58b4a3f0dded3299d22a4b56a0`，引擎版本 1.8.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：257922 → 259107，新增 **1185 条**。
- 权重 6000 / 5500 / 5000：0 / 1185 / 0 条。
- 最终 SHA-256：`98A9B7A4861C7DAA0C48F5BE2F71F65EC9475DBCFA5853C7BA045101899EDF49`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮补充温和提出建议、接受反馈并说明调整、意见不同后的协商折中；短批只收日常交流中可独立输入的自然说法，长批为完整沟通句。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 538 | 温和提出意见与可行建议聊天 | curate | 306 | 0 | 306 | 0 | 69.61% | 100.00% | `e902a4049e7c2e7654e31f75efa079a920f8e67b` |
| 539 | 接收反馈与说明调整聊天 | curate | 403 | 0 | 403 | 0 | 69.98% | 100.00% | `90ba248371a68c5bb2ee470eae48053a3604c925` |
| 540 | 意见不同与协商折中聊天 | curate | 446 | 0 | 446 | 0 | 77.13% | 100.00% | `e67b181908438098de5f3ff4000d96535a7b743c` |
| 541 | 温和提出意见较长说法 | merge | 10 | 0 | 10 | 0 | 60.00% | 70.00% | `5d7c6e5802520fa359320e5770e14a184f9480c7` |
| 542 | 接收反馈与说明调整较长说法 | merge | 10 | 0 | 10 | 0 | 70.00% | 90.00% | `81453ff0d2d188c4a7e005cd4dd840da0efa75a7` |
| 543 | 意见不同与协商折中较长说法 | merge | 10 | 0 | 10 | 0 | 50.00% | 60.00% | `a1da2bb7da661ed07ddf1b51fc049bd45d2506b2` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round53_exclusions.tsv`，共 239 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：99 条。
- `quality: specialist editing shorthand rather than a common standalone chat phrase`：2 条。
- `outside_batch_length_scope`：100 条。
- `quality: clipped phrase; the complement is incomplete`：1 条。
- `quality: clipped and awkward wording`：1 条。
- `quality: clipped phrase; unclear what is clear`：1 条。
- `quality: specialist shorthand and context dependent`：1 条。
- `quality: generic assessment without a clear object`：1 条。
- `quality: specialist design instruction, not common chat`：2 条。
- `quality: clipped design instruction`：1 条。
- `quality: unnatural wording and specialist instruction`：1 条。
- `quality: specialist layout terminology, not common chat`：1 条。
- `quality: ungrammatical phrase`：2 条。
- `quality: clipped comparative phrase`：1 条。
- `quality: unclear reference and context dependent`：2 条。
- `quality: clipped and unclear instruction`：1 条。
- `quality: clipped phrase; the verb complement is incomplete`：1 条。
- `quality: ungrammatical wording`：1 条。
- `quality: unnatural word order`：1 条。
- `quality: awkward standalone feedback`：1 条。
- `quality: vague and unnatural standalone assurance`：1 条。
- `quality: incomplete and unclear standalone feedback`：1 条。
- `duplicate:batch539`：3 条。
- `quality: unnatural wording`：1 条。
- `quality: technical or mathematical wording, not ordinary chat`：1 条。
- `duplicate:batch538`：2 条。
- `quality: ungrammatical and unclear wording`：1 条。
- `quality: vague and incomplete without context`：1 条。
- `quality: overly controlling and context dependent`：1 条。
- `duplicate:batch540`：3 条。
- `quality: clipped and unnatural wording`：1 条。
- `quality: clipped and context dependent`：1 条。
- `quality: corporate jargon rather than ordinary chat`：1 条。



同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1070 个含多音字的词在各批复核表逐字记录；13 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

多音字按完整建议、反馈和协商语境逐字复核并记录；轻声按本字读音。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 25/25 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，259107 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round53_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.23% → 88.23% | +0.00 | 98.65% → 98.65% |
| 逐词输入·无前文 | 20626 | 79.64% → 79.64% | +0.00 | 97.53% → 97.53% |
| 留出整句 | 4126 | 64.44% → 64.44% | +0.00 | 77.70% → 77.70% |
| 口语对话·整句 | 3368 | 84.80% → 84.80% | +0.00 | 94.24% → 94.24% |
| 口语对话·逐词 | 19504 | 95.50% → 95.50% | +0.00 | 99.63% → 99.63% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.00% → 96.00% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round53.md`
- `resources/dictionary/expansion/batch538_authored_draft.txt`
- `resources/dictionary/expansion/batch538_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch539_authored_draft.txt`
- `resources/dictionary/expansion/batch539_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch540_authored_draft.txt`
- `resources/dictionary/expansion/batch540_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch541_draft.txt`
- `resources/dictionary/expansion/batch541_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch542_draft.txt`
- `resources/dictionary/expansion/batch542_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch543_draft.txt`
- `resources/dictionary/expansion/batch543_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_538.md`
- `resources/dictionary/expansion/batch_539.md`
- `resources/dictionary/expansion/batch_540.md`
- `resources/dictionary/expansion/batch_541.md`
- `resources/dictionary/expansion/batch_542.md`
- `resources/dictionary/expansion/batch_543.md`
- `resources/dictionary/expansion/lexicon_batch_538.applied.json`
- `resources/dictionary/expansion/lexicon_batch_538.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_538.json`
- `resources/dictionary/expansion/lexicon_batch_538.tsv`
- `resources/dictionary/expansion/lexicon_batch_538.validation.json`
- `resources/dictionary/expansion/lexicon_batch_539.applied.json`
- `resources/dictionary/expansion/lexicon_batch_539.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_539.json`
- `resources/dictionary/expansion/lexicon_batch_539.tsv`
- `resources/dictionary/expansion/lexicon_batch_539.validation.json`
- `resources/dictionary/expansion/lexicon_batch_540.applied.json`
- `resources/dictionary/expansion/lexicon_batch_540.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_540.json`
- `resources/dictionary/expansion/lexicon_batch_540.tsv`
- `resources/dictionary/expansion/lexicon_batch_540.validation.json`
- `resources/dictionary/expansion/lexicon_batch_541.json`
- `resources/dictionary/expansion/lexicon_batch_541.tsv`
- `resources/dictionary/expansion/lexicon_batch_542.json`
- `resources/dictionary/expansion/lexicon_batch_542.tsv`
- `resources/dictionary/expansion/lexicon_batch_543.json`
- `resources/dictionary/expansion/lexicon_batch_543.tsv`
- `resources/dictionary/expansion/probes/batch538_probe.tsv`
- `resources/dictionary/expansion/probes/batch539_probe.tsv`
- `resources/dictionary/expansion/probes/batch540_probe.tsv`
- `resources/dictionary/expansion/probes/batch541_probe.tsv`
- `resources/dictionary/expansion/probes/batch542_probe.tsv`
- `resources/dictionary/expansion/probes/batch543_probe.tsv`
- `resources/dictionary/expansion/round53_exclusions.tsv`
- `resources/dictionary/expansion/round53_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
