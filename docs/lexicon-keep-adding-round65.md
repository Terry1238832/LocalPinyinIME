# 词库续加第65轮（批次 612–617）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `93f7f886fdcf4e2d76b7c575f52f42607abf8338`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：271954 → 273354，新增 **1400 条**。
- 权重 6000 / 5500 / 5000：0 / 1400 / 0 条。
- 最终 SHA-256：`B98EBC2A3828CA409390EFC2A2DCCE3159C1DA1911A6D475E1AAD51DC39BBAAD`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 612 | 约饭选店、口味偏好与点单聊天 | curate | 438 | 0 | 438 | 0 | 81.96% | 100.00% | `5ec0e58a963b138c39491cdceec91a9881191bcc` |
| 613 | 分享照片与日常动态的互动评论 | curate | 430 | 0 | 430 | 0 | 78.37% | 100.00% | `2a1729784652d3dfab99512b553f40d7cf7df57f` |
| 614 | 遇到小麻烦、临时求助与问题解决 | curate | 497 | 0 | 497 | 0 | 78.27% | 100.00% | `552a2692c9bd67891734a274498d47c58a802dd6` |
| 615 | 约饭选店、口味偏好与买单的长句 | merge | 15 | 0 | 15 | 0 | 73.33% | 73.33% | `aed89a5234e5fe930f007c35466f0df9e4e4f717` |
| 616 | 分享照片、动态与互动评论的长句 | merge | 10 | 0 | 10 | 0 | 80.00% | 90.00% | `3c87ef43f9f1cfb4627ac4e11a8b8c392d2535f5` |
| 617 | 遇到小麻烦、求助与解决问题后的长句 | merge | 10 | 0 | 10 | 0 | 60.00% | 60.00% | `a869aec6697992e9648b99ac94281d7a24133033` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round65_exclusions.tsv`，共 661 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：494 条。
- `duplicate:local_core_zh_pinyin.tsv`：153 条。
- `non_BMP_Han_or_non_Han`：5 条。
- `搭配不自然，改为‘配乐很合适’`：1 条。
- `表达不完整，改为‘老街很有味道’`：1 条。
- `表达不完整，改为‘这个角落好美’`：1 条。
- `表达不完整，改为‘好像一幅画’`：1 条。
- `duplicate:batch613`：2 条。
- `表达残缺，改为‘这提示啥意思’`：1 条。
- `表达残缺，改为‘切个网络试试’`：1 条。
- `搭配不完整，改为‘不行就换个办法’`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1254 个含多音字的词在各批复核表逐字记录；9 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

“这行啥意思”和“有一行看不懂”按文本行读 háng；“还原”按恢复义读 huán，其余‘还要、还是、还没、还行’按 hái；“色调调得好”分别读 diào / tiáo；弹出提示中的“弹”读 tán（不是 dan）；‘小心烫着’的‘着’读 zháo，‘着急’读 zháo，‘看着、照着、留着’读 zhe。每个词逐字记录在复核表。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 122/122 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，273354 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round65_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round65.md`
- `resources/dictionary/expansion/batch612_authored_draft.txt`
- `resources/dictionary/expansion/batch612_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch613_authored_draft.txt`
- `resources/dictionary/expansion/batch613_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch614_authored_draft.txt`
- `resources/dictionary/expansion/batch614_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch615_draft.txt`
- `resources/dictionary/expansion/batch615_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch616_draft.txt`
- `resources/dictionary/expansion/batch616_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch617_draft.txt`
- `resources/dictionary/expansion/batch617_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_612.md`
- `resources/dictionary/expansion/batch_613.md`
- `resources/dictionary/expansion/batch_614.md`
- `resources/dictionary/expansion/batch_615.md`
- `resources/dictionary/expansion/batch_616.md`
- `resources/dictionary/expansion/batch_617.md`
- `resources/dictionary/expansion/lexicon_batch_612.applied.json`
- `resources/dictionary/expansion/lexicon_batch_612.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_612.json`
- `resources/dictionary/expansion/lexicon_batch_612.tsv`
- `resources/dictionary/expansion/lexicon_batch_612.validation.json`
- `resources/dictionary/expansion/lexicon_batch_613.applied.json`
- `resources/dictionary/expansion/lexicon_batch_613.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_613.json`
- `resources/dictionary/expansion/lexicon_batch_613.tsv`
- `resources/dictionary/expansion/lexicon_batch_613.validation.json`
- `resources/dictionary/expansion/lexicon_batch_614.applied.json`
- `resources/dictionary/expansion/lexicon_batch_614.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_614.json`
- `resources/dictionary/expansion/lexicon_batch_614.tsv`
- `resources/dictionary/expansion/lexicon_batch_614.validation.json`
- `resources/dictionary/expansion/lexicon_batch_615.json`
- `resources/dictionary/expansion/lexicon_batch_615.tsv`
- `resources/dictionary/expansion/lexicon_batch_616.json`
- `resources/dictionary/expansion/lexicon_batch_616.tsv`
- `resources/dictionary/expansion/lexicon_batch_617.json`
- `resources/dictionary/expansion/lexicon_batch_617.tsv`
- `resources/dictionary/expansion/probes/batch612_probe.tsv`
- `resources/dictionary/expansion/probes/batch613_probe.tsv`
- `resources/dictionary/expansion/probes/batch614_probe.tsv`
- `resources/dictionary/expansion/probes/batch615_probe.tsv`
- `resources/dictionary/expansion/probes/batch616_probe.tsv`
- `resources/dictionary/expansion/probes/batch617_probe.tsv`
- `resources/dictionary/expansion/round65_exclusions.tsv`
- `resources/dictionary/expansion/round65_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
