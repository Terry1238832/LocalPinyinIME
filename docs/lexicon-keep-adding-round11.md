# 词库续加第11轮（批次 246–253）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `2b02d7ccb4874418374518168810652cd3940e4f`，引擎版本 1.0.0。分支通过合并吸收 main，不 rebase、不 force push。持续 PR 的总表从第十轮起累计。

- `local_core`：204939 → 206741，新增 **1802 条**。
- 权重 6000 / 5500 / 5000：0 / 1802 / 0 条。
- 最终 SHA-256：`66E6BF9526E50C2AAFA3CF22B7680942F0D80E6D3ECAE49126AD194A6AADF3AA`。
- CMake 上限 209495，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 246 | 社保医保与公积金 | curate | 345 | 0 | 345 | 0 | 76.81% | 100.00% | `7d8cc9073f80bc86a926f97e99bbd0d3212f33b4` |
| 247 | 房屋买卖贷款与过户 | curate | 455 | 0 | 455 | 0 | 84.18% | 100.00% | `8c9519e272815ac83570c57a3b70a55997440b28` |
| 248 | 婚恋与婚礼 | curate | 441 | 0 | 441 | 0 | 94.33% | 100.00% | `38e7c3de3eb07d1a58d659237d39b3ae7dab1f65` |
| 249 | 老年生活与养老 | curate | 480 | 0 | 480 | 0 | 80.62% | 100.00% | `a9ddb8269168971ed84eb0fa1abed3fc3ae82b07` |
| 250 | 社保医保与公积金较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `bfea370f6bf257bdffc547d4659f8ef1adad9371` |
| 251 | 房屋买卖贷款与过户较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `d431a65f8bbf0323c89474f775ed206bc1f4a818` |
| 252 | 婚恋与婚礼较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `0e932ae324a9bf89d2566a16b1687941a8cfe49d` |
| 253 | 老年生活与养老较长说法 | merge | 21 | 0 | 21 | 0 | 90.48% | 100.00% | `15470a38474d838cf90e2f0946e9100350c100fd` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round11_exclusions.tsv`，共 313 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：200 条。
- `quality:incomplete_or_uncommon_label`：37 条。
- `quality:incomplete_label`：3 条。
- `outside_batch_length_scope`：62 条。
- `outside_short_length_scope`：2 条。
- `duplicate:standard_zh_pinyin.tsv`：6 条。
- `quality:potentially_misleading_service_term`：1 条。
- `quality:uncertain_contextual_reading_omit`：1 条。
- `duplicate:batch246`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1478 个含多音字的词在各批复核表逐字记录；37 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 社保医保：重复参保的重 `chong`；参保的参 `can`，稽核的稽 `ji`；社保卡开户行的行 `hang`；更正、更换和变更用 `geng`。
- 房贷过户：所有还款、还贷、定金退还的还 `huan`，不是 `hai`；利率重定价的重 `chong`，利率调整的调 `tiao`；银行与开户行的行 `hang`；钥匙的匙 `shi`。
- 婚恋婚礼：亲家的亲 `qing`，相亲的相 `xiang`；家长的长 `zhang`，长期交往的长 `chang`。首饰的首 `shou`；音乐的乐 `yue`；婚礼露台的露 `lu`。
- 养老：长护险、长住养老、长期照护的长 `chang`；长者、长辈的长 `zhang`。手机声音调大的调 `tiao`；重阳的重 `chong`；精神慰藉的藉 `jie`；行动、助行的行 `xing`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 759/759 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 209495` 通过，206741 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round11_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round11.md`
- `resources/dictionary/expansion/batch246_authored_draft.txt`
- `resources/dictionary/expansion/batch246_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch247_authored_draft.txt`
- `resources/dictionary/expansion/batch247_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch248_authored_draft.txt`
- `resources/dictionary/expansion/batch248_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch249_authored_draft.txt`
- `resources/dictionary/expansion/batch249_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch250_draft.txt`
- `resources/dictionary/expansion/batch250_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch251_draft.txt`
- `resources/dictionary/expansion/batch251_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch252_draft.txt`
- `resources/dictionary/expansion/batch252_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch253_draft.txt`
- `resources/dictionary/expansion/batch253_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_246.md`
- `resources/dictionary/expansion/batch_247.md`
- `resources/dictionary/expansion/batch_248.md`
- `resources/dictionary/expansion/batch_249.md`
- `resources/dictionary/expansion/batch_250.md`
- `resources/dictionary/expansion/batch_251.md`
- `resources/dictionary/expansion/batch_252.md`
- `resources/dictionary/expansion/batch_253.md`
- `resources/dictionary/expansion/lexicon_batch_246.applied.json`
- `resources/dictionary/expansion/lexicon_batch_246.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_246.json`
- `resources/dictionary/expansion/lexicon_batch_246.tsv`
- `resources/dictionary/expansion/lexicon_batch_246.validation.json`
- `resources/dictionary/expansion/lexicon_batch_247.applied.json`
- `resources/dictionary/expansion/lexicon_batch_247.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_247.json`
- `resources/dictionary/expansion/lexicon_batch_247.tsv`
- `resources/dictionary/expansion/lexicon_batch_247.validation.json`
- `resources/dictionary/expansion/lexicon_batch_248.applied.json`
- `resources/dictionary/expansion/lexicon_batch_248.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_248.json`
- `resources/dictionary/expansion/lexicon_batch_248.tsv`
- `resources/dictionary/expansion/lexicon_batch_248.validation.json`
- `resources/dictionary/expansion/lexicon_batch_249.applied.json`
- `resources/dictionary/expansion/lexicon_batch_249.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_249.json`
- `resources/dictionary/expansion/lexicon_batch_249.tsv`
- `resources/dictionary/expansion/lexicon_batch_249.validation.json`
- `resources/dictionary/expansion/lexicon_batch_250.json`
- `resources/dictionary/expansion/lexicon_batch_250.tsv`
- `resources/dictionary/expansion/lexicon_batch_251.json`
- `resources/dictionary/expansion/lexicon_batch_251.tsv`
- `resources/dictionary/expansion/lexicon_batch_252.json`
- `resources/dictionary/expansion/lexicon_batch_252.tsv`
- `resources/dictionary/expansion/lexicon_batch_253.json`
- `resources/dictionary/expansion/lexicon_batch_253.tsv`
- `resources/dictionary/expansion/probes/batch246_probe.tsv`
- `resources/dictionary/expansion/probes/batch247_probe.tsv`
- `resources/dictionary/expansion/probes/batch248_probe.tsv`
- `resources/dictionary/expansion/probes/batch249_probe.tsv`
- `resources/dictionary/expansion/probes/batch250_probe.tsv`
- `resources/dictionary/expansion/probes/batch251_probe.tsv`
- `resources/dictionary/expansion/probes/batch252_probe.tsv`
- `resources/dictionary/expansion/probes/batch253_probe.tsv`
- `resources/dictionary/expansion/round11_exclusions.tsv`
- `resources/dictionary/expansion/round11_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
