# 词库续加第58轮（批次 568–573）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `49d4f3d4ac9063b558761f052a372860ae7daa53`，引擎版本 1.8.2。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：263683 → 264633，新增 **950 条**。
- 权重 6000 / 5500 / 5000：0 / 950 / 0 条。
- 最终 SHA-256：`9D3EA33A7D0D9C52E4D8D58704A11CD9AF6B34442D79951F79708FC9524A8FD5`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮按日常聊天里的自我调侃与小失误、谦让接话、意外发现后的核实追问三个细分方向补词，并对历史批次和前序批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 568 | 自嘲与小失误的轻松回应 | curate | 300 | 0 | 300 | 0 | 78.67% | 100.00% | `fcfbb5f6617e7b8e0a74a24325452eb8c81742b1` |
| 569 | 谦虚回应、改口与玩笑接话 | curate | 307 | 0 | 307 | 0 | 84.04% | 100.00% | `0c5aeba316eb5c78bf42f6ff49fb38677cc842e2` |
| 570 | 意外发现、核实与好奇追问 | curate | 310 | 0 | 310 | 0 | 86.45% | 100.00% | `424294a93f89b21af7f4326d412c86c5b0515312` |
| 571 | 轻松自嘲与小失误后的较长聊天说法 | merge | 11 | 0 | 11 | 0 | 72.73% | 72.73% | `9f3774f24df317c8068015e4dcc8e0642f170bf0` |
| 572 | 夸奖后的谦让与轻松接话较长说法 | merge | 11 | 0 | 11 | 0 | 90.91% | 100.00% | `f3e3b425ee167fb2ec384a7acdd99fd5d2f3f76e` |
| 573 | 意外反应与继续了解的较长聊天说法 | merge | 11 | 0 | 11 | 0 | 90.91% | 90.91% | `00c3462944125b64241a84c1ee0eda23e42b0ee7` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round58_exclusions.tsv`，共 488 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `搭配残缺；日常说法是“我又忘了”或“我又忘事了”。`：1 条。
- `duplicate:local_core_zh_pinyin.tsv`：40 条。
- `outside_batch_length_scope`：410 条。
- `搭配不自然；日常通常说“运气不太好”。`：1 条。
- `表达残缺；常见说法是“功夫不到家”，本批不收缩写。`：1 条。
- `书面色彩较重，日常聊天少这样说。`：1 条。
- `书面色彩较重；本批已有更自然的没看见、没留神等说法。`：1 条。
- `搭配生硬；已有自然的“我真是粗心”等表达。`：1 条。
- `成语化说法，与本批日常口语短语范围不符。`：1 条。
- `搭配不自然；常说“我耳背了”或“我没听清”。`：1 条。
- `偏书面说明，不是常见聊天整词短语。`：1 条。
- `搭配生硬；聊天里更常说“我也值得夸”等完整说法。`：1 条。
- `表达不完整；常说“灵光一闪”。`：1 条。
- `搭配生硬；常说“练出成果”。`：1 条。
- `语序不自然，聊天里不这样表达。`：1 条。
- `表达不完整；常见说法是“笑得肚子疼”。`：1 条。
- `搭配不自然，不是常见聊天说法。`：1 条。
- `搭配不自然，语义也不完整。`：1 条。
- `表达残缺，缺少“一个/一回事”等宾语。`：1 条。
- `语序不自然。`：4 条。
- `duplicate:batch568`：1 条。
- `谚语残句，表达不完整。`：1 条。
- `语序重复、生硬；常说“你没听错吧”。`：1 条。
- `搭配不自然；常说“我得想清楚”。`：1 条。
- `语序不自然；常说“你刚说什么”。`：1 条。
- `语序不自然；常说“你最近还好吧”。`：1 条。
- `语序不自然；常说“当时谁在场”。`：1 条。
- `表达残缺；常说“我听完再跟你说”。`：1 条。
- `表达残缺；常说“说得太神奇了”。`：1 条。
- `搭配不自然；常说“我想问清楚”。`：1 条。
- `表达残缺；常说“我想听你讲清楚”。`：1 条。
- `搭配不完整；通常说“你用简单的话说”。`：1 条。
- `搭配不自然；常说“能打个比方吗”。`：1 条。
- `书面色彩较重，不是常见聊天说法。`：1 条。
- `搭配不自然；常说“我想问个明白”等表达。`：1 条。
- `成语化书面表达，与本批聊天短语范围不符。`：1 条。
- `表达残缺，缺少自然的句末成分。`：1 条。

按语义质量筛除表达残缺、语序不自然、搭配生硬及偏书面的短语；具体词面和原因完整记录于 round58_exclusions.tsv。未因长词探针未进前九而删词；长词结果按要求记录在 round58_validation.json。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

895 个含多音字的词在各批复核表逐字记录；15 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

关键决定：程度义“比我还清楚”的还读 hai；表示需要的“还得练练、得向你学习、得消化一下、真得写下来、地方得慢慢学”等得读 dei，结构助词“说得、接得”仍读 de。否定词“没过脑子”读 mei；“了解”的了读 liao，完成体的了读 le。“一句接一句地夸、认真地夸”中地作结构助词读轻声 de；“地方”的地读 di。口语思路或说话受阻的“卡壳、卡住”中卡读 qia，壳读 ke。每个含多音字词均在复核表逐字列出上下文读音。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 13/13 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，264633 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round58_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 88.21% → 88.21% | +0.00 | 98.63% → 98.63% |
| 逐词输入·无前文 | 20626 | 79.64% → 79.64% | +0.00 | 97.53% → 97.53% |
| 留出整句 | 4126 | 64.49% → 64.49% | +0.00 | 77.58% → 77.58% |
| 口语对话·整句 | 3368 | 84.80% → 84.80% | +0.00 | 94.12% → 94.12% |
| 口语对话·逐词 | 19504 | 95.49% → 95.49% | +0.00 | 99.62% → 99.62% |
| 项目留出 | 138 | 84.78% → 84.78% | +0.00 | 89.13% → 89.13% |
| 新增用例 | 67 | 71.64% → 71.64% | +0.00 | 83.58% → 83.58% |
| 独立用例 | 62 | 53.23% → 53.23% | +0.00 | 79.03% → 79.03% |
| 日常口语·开发 | 149 | 97.32% → 97.32% | +0.00 | 99.33% → 99.33% |
| 日常口语·留出 | 150 | 90.00% → 90.00% | +0.00 | 96.67% → 96.67% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round58.md`
- `resources/dictionary/expansion/batch568_authored_draft.txt`
- `resources/dictionary/expansion/batch568_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch569_authored_draft.txt`
- `resources/dictionary/expansion/batch569_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch570_authored_draft.txt`
- `resources/dictionary/expansion/batch570_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch571_draft.txt`
- `resources/dictionary/expansion/batch571_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch572_draft.txt`
- `resources/dictionary/expansion/batch572_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch573_draft.txt`
- `resources/dictionary/expansion/batch573_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_568.md`
- `resources/dictionary/expansion/batch_569.md`
- `resources/dictionary/expansion/batch_570.md`
- `resources/dictionary/expansion/batch_571.md`
- `resources/dictionary/expansion/batch_572.md`
- `resources/dictionary/expansion/batch_573.md`
- `resources/dictionary/expansion/lexicon_batch_568.applied.json`
- `resources/dictionary/expansion/lexicon_batch_568.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_568.json`
- `resources/dictionary/expansion/lexicon_batch_568.tsv`
- `resources/dictionary/expansion/lexicon_batch_568.validation.json`
- `resources/dictionary/expansion/lexicon_batch_569.applied.json`
- `resources/dictionary/expansion/lexicon_batch_569.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_569.json`
- `resources/dictionary/expansion/lexicon_batch_569.tsv`
- `resources/dictionary/expansion/lexicon_batch_569.validation.json`
- `resources/dictionary/expansion/lexicon_batch_570.applied.json`
- `resources/dictionary/expansion/lexicon_batch_570.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_570.json`
- `resources/dictionary/expansion/lexicon_batch_570.tsv`
- `resources/dictionary/expansion/lexicon_batch_570.validation.json`
- `resources/dictionary/expansion/lexicon_batch_571.json`
- `resources/dictionary/expansion/lexicon_batch_571.tsv`
- `resources/dictionary/expansion/lexicon_batch_572.json`
- `resources/dictionary/expansion/lexicon_batch_572.tsv`
- `resources/dictionary/expansion/lexicon_batch_573.json`
- `resources/dictionary/expansion/lexicon_batch_573.tsv`
- `resources/dictionary/expansion/probes/batch568_probe.tsv`
- `resources/dictionary/expansion/probes/batch569_probe.tsv`
- `resources/dictionary/expansion/probes/batch570_probe.tsv`
- `resources/dictionary/expansion/probes/batch571_probe.tsv`
- `resources/dictionary/expansion/probes/batch572_probe.tsv`
- `resources/dictionary/expansion/probes/batch573_probe.tsv`
- `resources/dictionary/expansion/round58_exclusions.tsv`
- `resources/dictionary/expansion/round58_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
