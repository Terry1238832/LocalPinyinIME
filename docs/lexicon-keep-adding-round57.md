# 词库续加第57轮（批次 562–567）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `4bf066d0ee991084e79ebaadc2a2bd98824cc946`，引擎版本 1.8.2。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：262475 → 263683，新增 **1208 条**。
- 权重 6000 / 5500 / 5000：0 / 1208 / 0 条。
- 最终 SHA-256：`8B589BF3FEEF2A6E9314CBFB27E6FF73A199B558B8CD8882955C1050FB346D6A`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天里的致歉与解释。分成临时迟到与改约、漏看消息与回复晚、无心打扰与小误会三个细分主题；每个主题各有一批 2–6 字短词和一批 7 字以上较长说法。按词面与已有词库、全部历史批次及本轮先前批次去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 562 | 临时迟到与改约后的道歉回应聊天 | curate | 432 | 0 | 432 | 0 | 80.09% | 100.00% | `712a6117672d3873eff6ea9797c60c3d67e129c0` |
| 563 | 漏看消息与回复晚了后的道歉聊天 | curate | 380 | 0 | 380 | 0 | 78.16% | 100.00% | `da4a97fdf1d379c29d29df6892c5ed0b635d4d52` |
| 564 | 无心打扰与小误会后的致歉聊天 | curate | 364 | 0 | 364 | 0 | 80.22% | 100.00% | `13b46831b24ee44eba9703b294ead5e2b5fd3801` |
| 565 | 临时迟到与改约后的道歉较长说法 | merge | 11 | 0 | 11 | 0 | 81.82% | 81.82% | `1eb5ce0f2d1de565eaff4588887d422ca73c873f` |
| 566 | 漏看消息与回复晚了后的道歉较长说法 | merge | 11 | 0 | 11 | 0 | 90.91% | 90.91% | `76611076d97586aece78944d7cbbf3cb5fb42f29` |
| 567 | 无心打扰与小误会后的致歉较长说法 | merge | 10 | 0 | 10 | 0 | 70.00% | 70.00% | `fc0b339ba84ff212812ac6e17deccd67dc4ba693` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round57_exclusions.tsv`，共 849 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：757 条。
- `duplicate:local_core_zh_pinyin.tsv`：67 条。
- `搭配不自然，常用说法为“到了跟你说”；删除。`：1 条。
- `搭配生硬；用“我来晚了”等常见说法。`：1 条。
- `表达生硬；本批已有更自然的“临时多了点事”。`：1 条。
- `表达残缺；完整说法是“计划赶不上变化”。`：1 条。
- `duplicate:batch562`：3 条。
- `语序不自然；用“刚看到消息”等常见说法。`：1 条。
- `duplicate:batch563`：2 条。
- `表达不自然；同主题已有“漏回的我补上”。`：1 条。
- `搭配不自然，且“补给”会误导“给”的读音；删除。`：1 条。
- `搭配不自然；用“我没有恶意”等常见说法。`：1 条。
- `duplicate:batch564`：7 条。
- `语法不成立；常用说法是“我会错意了”。`：1 条。
- `语义不明确，不是自然的聊天致歉表达。`：1 条。
- `表达残缺；同主题已有“发错群聊了”等完整说法。`：1 条。
- `对象不清且表达生硬，删除。`：1 条。
- `搭配不自然；用“确认错了信息”等清楚说法。`：1 条。

按质量理由排除 13 个不自然、残缺或搭配不清的短语；另有重复词面和超出批次字数的候选记录在 round57_exclusions.tsv。没有整批撤回。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1114 个含多音字的词在各批复核表逐字记录；16 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

关键读音：否定“没收到”读 mei；“没收好”的“没”按 mei；长队的“长”读 chang；通知弹出读 tan；调静音/调低音量的“调”读 tiao；口语“谁”记 shei；“重说”读 chong、“重点”读 zhong；表示需要的“得”读 dei，结构助词读 de；“来不了/去不了”的了读 liao，句末了读 le；着急的“着”读 zhao，急着的“着”读 zhe。七字以上批次按 merge 工具处理；为遵守测试隔离，没有人工打开或搜索 tests/data 内容，也未查看评测用例或按缺词选词；accuracy_snapshot 与覆盖率只作任务要求的汇总测量。merge 工具会直接读取 holdout 文本，因此传入空 holdout 文件，避免该工具访问测试集。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 28/28 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，263683 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round57_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round57.md`
- `resources/dictionary/expansion/batch562_authored_draft.txt`
- `resources/dictionary/expansion/batch562_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch563_authored_draft.txt`
- `resources/dictionary/expansion/batch563_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch564_authored_draft.txt`
- `resources/dictionary/expansion/batch564_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch565_draft.txt`
- `resources/dictionary/expansion/batch565_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch566_draft.txt`
- `resources/dictionary/expansion/batch566_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch567_draft.txt`
- `resources/dictionary/expansion/batch567_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_562.md`
- `resources/dictionary/expansion/batch_563.md`
- `resources/dictionary/expansion/batch_564.md`
- `resources/dictionary/expansion/batch_565.md`
- `resources/dictionary/expansion/batch_566.md`
- `resources/dictionary/expansion/batch_567.md`
- `resources/dictionary/expansion/lexicon_batch_562.applied.json`
- `resources/dictionary/expansion/lexicon_batch_562.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_562.json`
- `resources/dictionary/expansion/lexicon_batch_562.tsv`
- `resources/dictionary/expansion/lexicon_batch_562.validation.json`
- `resources/dictionary/expansion/lexicon_batch_563.applied.json`
- `resources/dictionary/expansion/lexicon_batch_563.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_563.json`
- `resources/dictionary/expansion/lexicon_batch_563.tsv`
- `resources/dictionary/expansion/lexicon_batch_563.validation.json`
- `resources/dictionary/expansion/lexicon_batch_564.applied.json`
- `resources/dictionary/expansion/lexicon_batch_564.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_564.json`
- `resources/dictionary/expansion/lexicon_batch_564.tsv`
- `resources/dictionary/expansion/lexicon_batch_564.validation.json`
- `resources/dictionary/expansion/lexicon_batch_565.json`
- `resources/dictionary/expansion/lexicon_batch_565.tsv`
- `resources/dictionary/expansion/lexicon_batch_566.json`
- `resources/dictionary/expansion/lexicon_batch_566.tsv`
- `resources/dictionary/expansion/lexicon_batch_567.json`
- `resources/dictionary/expansion/lexicon_batch_567.tsv`
- `resources/dictionary/expansion/probes/batch562_probe.tsv`
- `resources/dictionary/expansion/probes/batch563_probe.tsv`
- `resources/dictionary/expansion/probes/batch564_probe.tsv`
- `resources/dictionary/expansion/probes/batch565_probe.tsv`
- `resources/dictionary/expansion/probes/batch566_probe.tsv`
- `resources/dictionary/expansion/probes/batch567_probe.tsv`
- `resources/dictionary/expansion/round57_exclusions.tsv`
- `resources/dictionary/expansion/round57_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
