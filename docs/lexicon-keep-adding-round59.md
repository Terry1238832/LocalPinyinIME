# 词库续加第59轮（批次 574–579）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `64fed7624d0fea3437de467c5669bbb330fc203f`，引擎版本 1.8.3。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第51轮起累计。

- `local_core`：264633 → 265694，新增 **1061 条**。
- 权重 6000 / 5500 / 5000：0 / 1061 / 0 条。
- 最终 SHA-256：`185976E4EAB55D402CB7934E9BDFC3DBAA2CE8D5CEBF2241E29E6133153482A1`。
- CMake 上限 281456，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮围绕聊天中的三个窄场景补词：聚会散场后的告别与报平安、旧友重逢后的叙旧、睡前晚安与次日再聊。词条逐条自拟，没有用测试内容或评测失败项挑词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 574 | 聚会散场后的道别与到家报平安 | curate | 333 | 0 | 333 | 0 | 91.59% | 100.00% | `5771ec5ae04a9ba205115912842a810241597055` |
| 575 | 旧友重逢后的近况与共同回忆闲聊 | curate | 328 | 0 | 328 | 0 | 82.62% | 100.00% | `f36bf95eb3205ecf74a94475aa660946627be826` |
| 576 | 睡前晚安、夜间关心与明早再聊 | curate | 323 | 0 | 323 | 0 | 82.97% | 100.00% | `e5c3559926eaaf054fac3b72d2e585b6b52a04fb` |
| 577 | 散场后的告别、同行与到家报平安长句 | merge | 28 | 0 | 28 | 0 | 78.57% | 89.29% | `c2a84ea67a9141c7445212366976e8f83809578c` |
| 578 | 久别重逢后的近况叙旧与共同回忆长句 | merge | 24 | 0 | 24 | 0 | 62.50% | 75.00% | `6e44990f7e2c75700b5e54bdb320ce18fe175436` |
| 579 | 睡前互道晚安、关心休息与明早再聊长句 | merge | 25 | 0 | 25 | 0 | 72.00% | 80.00% | `b9f02c4f80c50d2f2a4984971652c926dd9a7f1c` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round59_exclusions.tsv`，共 385 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：101 条。
- `偏正式书面告别，和日常聚会聊天语气不符。`：1 条。
- `表达不完整；通常说“到家给我说一声”或“到家告诉我”。`：1 条。
- `表达不完整；缺少“消息”等宾语。`：1 条。
- `表达残缺；没有说明回去后要说什么。`：1 条。
- `语义重复，聊天里搭配生硬。`：1 条。
- `语序不自然；常说“别忘了拿包”。`：1 条。
- `表达别扭；通常说“不用送我下楼”。`：1 条。
- `表达残缺；常说“我坐地铁回去”。`：1 条。
- `表达残缺；常说“我坐公交回去”。`：1 条。
- `搭配不自然；常说“到家再跟你说一声”。`：1 条。
- `缺少“上了”等结果补语，表达残缺。`：1 条。
- `语义含混，不是自然的AA或请客说法。`：1 条。
- `搭配生硬；通常说“下次我请客”。`：1 条。
- `表达残缺；常说“不知不觉就很晚了”。`：1 条。
- `搭配不自然，日常通常说“时间过得真快”。`：1 条。
- `outside_batch_length_scope`：192 条。
- `语义不完整，缺少明确的聚会或活动。`：1 条。
- `只是临时指示，没有形成常用聊天表达。`：1 条。
- `搭配生硬；通常提醒“别把联系方式删了”。`：1 条。
- `搭配不自然；常说“一路顺风”或“回去路上小心”。`：1 条。
- `语序和搭配不自然，带有生硬的送客语气。`：1 条。
- `表达不明确，不是常见固定提醒。`：1 条。
- `搭配生硬；常说“这次聚得真开心”。`：1 条。
- `与本批聚会散场语境关联弱，表达也缺少上下文。`：1 条。
- `duplicate:batch574`：5 条。
- `表达不完整；常说“我下楼等你的车”。`：1 条。
- `“随身物”不自然，通常说“随身物品”。`：1 条。
- `表达残缺；常说“走前看一下桌上有没有东西”。`：1 条。
- `泛化的动作拼接，没有体现重逢叙旧的固定表达。`：1 条。
- `泛化的活动拼接，聊天里缺少具体语境。`：1 条。
- `语境不完整，搭配指向不清。`：1 条。
- `搭配生硬，不是自然的叙旧说法。`：1 条。
- `表达生硬，不是自然的叙旧说法。`：1 条。
- `语义突兀，与重逢叙旧的常见表达关联弱。`：1 条。
- `表达不完整；通常说“以前你教我做题”。`：1 条。
- `语序生硬，不是常见自然说法。`：1 条。
- `搭配不自然；常说“见到你觉得很亲切”。`：1 条。
- `缺少结构助词“的”，表达不完整。`：1 条。
- `语义不清，叙旧中不自然。`：1 条。
- `书面色彩偏重，不像日常聊天用语。`：1 条。
- `搭配不完整；常说“这些年辛苦了”。`：1 条。
- `偏书面，日常通常说“还是很聊得来”。`：1 条。
- `搭配不自然；常说“一见面就熟了”。`：1 条。
- `语序不自然；常说“离远了真没认出来”。`：1 条。
- `语序不自然；常说“咱俩变化都大”。`：1 条。
- `搭配生硬，不是自然口语。`：1 条。
- `表达含混且偏书面，不是常用聊天说法。`：1 条。
- `语序不自然；常说“把以前的照片找出来”。`：1 条。
- `表达不完整；常说“一路聊到家门口”。`：1 条。
- `搭配不自然。`：1 条。
- `表达残缺，缺少动作或判断。`：2 条。
- `泛化活动拼接，没有叙旧语境。`：1 条。
- `单纯名词短语，缺少聊天固定表达。`：2 条。
- `搭配不自然；常说“回头联系”或“加个联系方式”。`：1 条。
- `语序不自然，表达不完整。`：1 条。
- `偏书面，不是常见聊天说法。`：1 条。
- `偏书面文学表达，不像日常聊天。`：1 条。
- `偏书面且语义含混，不是自然的聊天表达。`：1 条。
- `语气词搭配不够自然，常说“早点休息呀”。`：1 条。
- `搭配生硬；常说“祝你好梦”或“做个好梦”。`：1 条。
- `搭配不自然；常说“你先睡吧”。`：1 条。
- `表达残缺；常见说法是“困得睁不开眼”。`：1 条。
- `偏书面，不是常见聊天表达。`：1 条。
- `表达残缺；常说“明天提醒我起床”。`：1 条。
- `表达残缺；缺少说明醒来后说什么。`：1 条。
- `语序生硬；常说“睡醒了再回我”。`：1 条。
- `语义不清，搭配不自然。`：1 条。
- `像设备操作说明，不是自然的睡前聊天用语。`：1 条。
- `语气词重复，表达不自然。`：1 条。
- `搭配生硬；常说“困了就去睡吧”。`：1 条。
- `语序不自然；通常说“你先睡，不用等我”。`：1 条。
- `表达含混，不是自然提醒。`：1 条。
- `像书面提示语，不是常见聊天整词。`：1 条。
- `搭配不自然；常说“记得给手机充电”。`：1 条。
- `语序生硬；常说“去洗把脸吧”。`：1 条。
- `像标题或祝愿残句，不是自然聊天表达。`：1 条。
- `语序不自然；常说“晚安，明早见”。`：1 条。
- `搭配生硬，像生硬的劝告。`：1 条。
- `与睡前问候关联弱，表达也依赖上下文。`：1 条。
- `语法错误；常说“别玩到天亮”。`：1 条。
- `语序不自然；常说“晚安，不聊了”。`：1 条。
- `语义不合适，晚安应在睡前说。`：1 条。
- `语境依赖强，容易被理解为已早睡的过去陈述。`：1 条。
- `指短暂休息，不是本批的睡前告别表达。`：1 条。
- `语序不自然；常说“下次再聊，晚安”。`：1 条。
- `搭配不自然；常说“早点睡”或“睡个好觉”。`：1 条。
- `重复生硬，不是稳定常见说法。`：1 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1000 个含多音字的词在各批复核表逐字记录；13 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

本轮逐字语境决定和默认键差异均见各批 `pronunciation_review.tsv` 与批次 JSON。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：47/47 通过。
- x86 Release 原生构建与 ctest：47/47 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 71/71 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 281456` 通过，265694 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round59_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round59.md`
- `resources/dictionary/expansion/batch574_authored_draft.txt`
- `resources/dictionary/expansion/batch574_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch575_authored_draft.txt`
- `resources/dictionary/expansion/batch575_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch576_authored_draft.txt`
- `resources/dictionary/expansion/batch576_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch577_draft.txt`
- `resources/dictionary/expansion/batch577_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch578_draft.txt`
- `resources/dictionary/expansion/batch578_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch579_draft.txt`
- `resources/dictionary/expansion/batch579_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_574.md`
- `resources/dictionary/expansion/batch_575.md`
- `resources/dictionary/expansion/batch_576.md`
- `resources/dictionary/expansion/batch_577.md`
- `resources/dictionary/expansion/batch_578.md`
- `resources/dictionary/expansion/batch_579.md`
- `resources/dictionary/expansion/lexicon_batch_574.applied.json`
- `resources/dictionary/expansion/lexicon_batch_574.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_574.json`
- `resources/dictionary/expansion/lexicon_batch_574.tsv`
- `resources/dictionary/expansion/lexicon_batch_574.validation.json`
- `resources/dictionary/expansion/lexicon_batch_575.applied.json`
- `resources/dictionary/expansion/lexicon_batch_575.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_575.json`
- `resources/dictionary/expansion/lexicon_batch_575.tsv`
- `resources/dictionary/expansion/lexicon_batch_575.validation.json`
- `resources/dictionary/expansion/lexicon_batch_576.applied.json`
- `resources/dictionary/expansion/lexicon_batch_576.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_576.json`
- `resources/dictionary/expansion/lexicon_batch_576.tsv`
- `resources/dictionary/expansion/lexicon_batch_576.validation.json`
- `resources/dictionary/expansion/lexicon_batch_577.json`
- `resources/dictionary/expansion/lexicon_batch_577.tsv`
- `resources/dictionary/expansion/lexicon_batch_578.json`
- `resources/dictionary/expansion/lexicon_batch_578.tsv`
- `resources/dictionary/expansion/lexicon_batch_579.json`
- `resources/dictionary/expansion/lexicon_batch_579.tsv`
- `resources/dictionary/expansion/probes/batch574_probe.tsv`
- `resources/dictionary/expansion/probes/batch575_probe.tsv`
- `resources/dictionary/expansion/probes/batch576_probe.tsv`
- `resources/dictionary/expansion/probes/batch577_probe.tsv`
- `resources/dictionary/expansion/probes/batch578_probe.tsv`
- `resources/dictionary/expansion/probes/batch579_probe.tsv`
- `resources/dictionary/expansion/round59_exclusions.tsv`
- `resources/dictionary/expansion/round59_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
