# 词库续加第38轮（批次 444–451）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `2000abb5a1f5bd7f41bf13c8b98b4fda64b394ad`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：240935 → 242593，新增 **1658 条**。
- 权重 6000 / 5500 / 5000：0 / 1658 / 0 条。
- 最终 SHA-256：`A9DD6CBC8C3852B29BFB35C2C5391BFE89FA3267DC0C615FA385404EBB03A006`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。逐条拟写并复核时间先后与期限、数量份额与计量、物件指认与位置、接话补充与追问四个方向。时间和数量包含作者此前未提交草稿中重新逐条审定的短语；没有使用旧共享词库快照覆盖当前数据。日期和数量只选择生活里常见的说法，不按星期×时段或数词×量词×名词生成组合，不机械替换人物和句尾凑数。按全部现有词面与本轮前批去重，不从测试数据或评测答案选词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 444 | 时间紧慢与先后安排聊天 | curate | 350 | 0 | 350 | 0 | 87.71% | 100.00% | `0e7e3f1a018de93292270267ad4c17d51c36e8bf` |
| 445 | 数量分量与多少问答聊天 | curate | 460 | 0 | 460 | 0 | 76.74% | 100.00% | `7c1586b45da60dbd2dd100479db590c2f93e28de` |
| 446 | 物件指认与位置远近聊天 | curate | 367 | 0 | 367 | 0 | 65.67% | 100.00% | `78dcb33c56eaa9f491519131ce6550184b8c1a0d` |
| 447 | 接话补充与句尾追问聊天 | curate | 401 | 0 | 401 | 0 | 80.30% | 100.00% | `684acfbc88260e73344bec843827dbfc8cda34f5` |
| 448 | 时间紧慢与先后安排较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `0070829683e8e21c520f5f259eb055391ba44312` |
| 449 | 数量分量与多少问答较长说法 | merge | 20 | 0 | 20 | 0 | 65.00% | 100.00% | `8141a4519ddd914fe3f1bb471c2ac9eb7fe1f9cc` |
| 450 | 物件指认与位置远近较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `657f346427d41dfdb9e0f3532e02e251e2873665` |
| 451 | 接话补充与句尾追问较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 95.00% | `99c163c7f5ab7835ba12331cd83b5ef1603cbfd3` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round38_exclusions.tsv`，共 195 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：49 条。
- `quality: incomplete lead-in, stilted whole-input wording or unnecessarily equivalent variation`：71 条。
- `pronunciation: time 卡 wording not confidently resolved; defer rather than use the default`：3 条。
- `outside_batch_length_scope`：36 条。
- `quality: incomplete ending or less natural standalone time reply`：7 条。
- `duplicate:standard_zh_pinyin.tsv`：2 条。
- `quality: awkward month-boundary or overly vague approximate-quantity wording`：3 条。
- `quality: unnecessary reordered equivalent of the more natural 少给我一点`：1 条。
- `quality: incomplete quantity response or weakly idiomatic standalone wording`：5 条。
- `orthography: choose the common 分量 form in the authored phrase instead of 份量`：1 条。
- `quality: incomplete referent, clipped explanation or unnecessary conversational variation`：11 条。
- `quality: stilted follow-up, clipped phrase or unnecessary synonymous variation`：5 条。
- `orthography: use structural particle 得 in the authored description 你说得太形象`：1 条。

按所有层和domains的词面及本轮前批去重，剔除残缺的引导语、拗口时间标签、过于含混的近似计量、冗余同义句尾等。没有机械穷举星期、时段、人物、数量和名词的组合。时间卡的三条候选因字音把握不足不收；另用明确自然的日程表述。对份量和的/得两处草稿重新拟定表达，旧写法排除。所有纠音在正式入库和补词后测量前完成。具体候选与原因见本轮排除表。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1482 个含多音字的词在各批复核表逐字记录；58 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐条审定：表示必须的得取dei，来得及、得空和程度补语的得取de；盛饭、盛半碗、给我盛点的盛取cheng；数数取shu，称重量取cheng，计量动作的量取liang；重复领取或计算的拿重了、算重了取chong，物品重量的重取zhong；长度的长取chang，物品长得像的长取zhang，同行出游取xing；顺着、靠着、分着装表示持续或沿随，着取zhe；用不着、用得着、够不着的着取zhao；琢磨思想取zuo mo，捋清思路取lv；薄的单字口语取bao，厚薄复合词取bo；一沓白纸取da，一小撮盐取cuo，一拃宽取zha，抹零头取mo。两个谁的候选统一选择shui；同词面不另加异读变体。份量差不多吧改拟为分量差不多吧，描述说话程度用你说得太形象，两个旧草稿写法均记入排除表。三条时间卡的说法暂不定音收录，长句另拟为别排得太紧。轻声按本字写，üe写lve/nve。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 170/170 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，242593 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round38_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round38.md`
- `resources/dictionary/expansion/batch444_authored_draft.txt`
- `resources/dictionary/expansion/batch444_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch445_authored_draft.txt`
- `resources/dictionary/expansion/batch445_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch446_authored_draft.txt`
- `resources/dictionary/expansion/batch446_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch447_authored_draft.txt`
- `resources/dictionary/expansion/batch447_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch448_draft.txt`
- `resources/dictionary/expansion/batch448_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch449_draft.txt`
- `resources/dictionary/expansion/batch449_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch450_draft.txt`
- `resources/dictionary/expansion/batch450_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch451_draft.txt`
- `resources/dictionary/expansion/batch451_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_444.md`
- `resources/dictionary/expansion/batch_445.md`
- `resources/dictionary/expansion/batch_446.md`
- `resources/dictionary/expansion/batch_447.md`
- `resources/dictionary/expansion/batch_448.md`
- `resources/dictionary/expansion/batch_449.md`
- `resources/dictionary/expansion/batch_450.md`
- `resources/dictionary/expansion/batch_451.md`
- `resources/dictionary/expansion/lexicon_batch_444.applied.json`
- `resources/dictionary/expansion/lexicon_batch_444.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_444.json`
- `resources/dictionary/expansion/lexicon_batch_444.tsv`
- `resources/dictionary/expansion/lexicon_batch_444.validation.json`
- `resources/dictionary/expansion/lexicon_batch_445.applied.json`
- `resources/dictionary/expansion/lexicon_batch_445.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_445.json`
- `resources/dictionary/expansion/lexicon_batch_445.tsv`
- `resources/dictionary/expansion/lexicon_batch_445.validation.json`
- `resources/dictionary/expansion/lexicon_batch_446.applied.json`
- `resources/dictionary/expansion/lexicon_batch_446.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_446.json`
- `resources/dictionary/expansion/lexicon_batch_446.tsv`
- `resources/dictionary/expansion/lexicon_batch_446.validation.json`
- `resources/dictionary/expansion/lexicon_batch_447.applied.json`
- `resources/dictionary/expansion/lexicon_batch_447.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_447.json`
- `resources/dictionary/expansion/lexicon_batch_447.tsv`
- `resources/dictionary/expansion/lexicon_batch_447.validation.json`
- `resources/dictionary/expansion/lexicon_batch_448.json`
- `resources/dictionary/expansion/lexicon_batch_448.tsv`
- `resources/dictionary/expansion/lexicon_batch_449.json`
- `resources/dictionary/expansion/lexicon_batch_449.tsv`
- `resources/dictionary/expansion/lexicon_batch_450.json`
- `resources/dictionary/expansion/lexicon_batch_450.tsv`
- `resources/dictionary/expansion/lexicon_batch_451.json`
- `resources/dictionary/expansion/lexicon_batch_451.tsv`
- `resources/dictionary/expansion/probes/batch444_probe.tsv`
- `resources/dictionary/expansion/probes/batch445_probe.tsv`
- `resources/dictionary/expansion/probes/batch446_probe.tsv`
- `resources/dictionary/expansion/probes/batch447_probe.tsv`
- `resources/dictionary/expansion/probes/batch448_probe.tsv`
- `resources/dictionary/expansion/probes/batch449_probe.tsv`
- `resources/dictionary/expansion/probes/batch450_probe.tsv`
- `resources/dictionary/expansion/probes/batch451_probe.tsv`
- `resources/dictionary/expansion/round38_exclusions.tsv`
- `resources/dictionary/expansion/round38_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
