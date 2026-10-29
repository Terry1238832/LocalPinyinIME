# 词库续加第48轮（批次 508–513）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `29bc01f847c536784e45d5319669e307fdea52a5`，引擎版本 1.7.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，当前持续 PR 的总表从第42轮起累计。

- `local_core`：252476 → 253593，新增 **1117 条**。
- 权重 6000 / 5500 / 5000：0 / 1117 / 0 条。
- 最终 SHA-256：`806921FD115AA51462F6D755708F469CA82AAB659FEB370AE352951659407F52`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮继续日常聊天，逐条自拟游戏组队进房开局、桌游棋牌休闲交流、拼图拼装与手作协作三个方向，各一短一长共六批。只收通用且自然的整段问答和提醒，不收具体游戏名、人物名、商标、押注金额和攻击性话语；不从测试数据或失败用例选词，不批量替换人称数字或语气词。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 508 | 游戏组队进房与开局交流聊天 | curate | 347 | 0 | 347 | 0 | 70.61% | 100.00% | `24795c8bb3c2e53fcf2d729990d1c3ccca4e5efd` |
| 509 | 桌游棋牌与对局商量聊天 | curate | 339 | 0 | 339 | 0 | 57.52% | 100.00% | `8bef85ab42299cfb04218893541a5fa95b44171b` |
| 510 | 拼图拼装与手作协作聊天 | curate | 371 | 0 | 371 | 0 | 54.18% | 100.00% | `2a4ebfcbf340a012125acc6aa39047ffe5611f95` |
| 511 | 游戏组队进房与开局较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 95.00% | `002277278e209648522e0f1410569a79453a494f` |
| 512 | 桌游棋牌与对局商量较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 85.00% | `661685c0c5244e5100abf2aabe580bb44708e576` |
| 513 | 拼图拼装与手作协作较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `4d7cca4a0af6243a463864340f17394a5fbeb52a` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round48_exclusions.tsv`，共 360 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `outside_batch_length_scope`：280 条。
- `duplicate:local_core_zh_pinyin.tsv`：27 条。
- `quality: clipped voice-channel or role reference, vague loading wording or less natural game-session phrasing`：14 条。
- `quality: clipped chess-piece or box reference, awkward first-player order, unclear card shuffling or turn wording`：13 条。
- `quality: missing directional complement, unclear instruction referent, awkward amount order or incomplete assembly wording`：21 条。
- `pronunciation: replace ambiguous mechanical 卡 description with an explicit snap sound`：1 条。
- `quality: contradictory button-timing description, awkward card identity or unclear trial-and-help sequence in longer wording`：4 条。

去掉不完整的频道、补位、邀请、加载描述，另拟旧频道、补位、正在加载、解散队伍等明确说法。棋牌草稿去掉洗单张牌、回合顺序及盒子指代的不自然表达；象棋车、落子和将棋子的语境逐字复核。拼图拼装去掉缺方向补语、缺物件或量词语序不自然的旧稿，另拟明确的零件、角片、尺寸和工具动作；机械卡扣改写为咔哒声，不猜卡字读音。几条长句纠正按键时长逻辑和求助顺序，旧稿与原因逐条写入排除表。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1028 个含多音字的词在各批复核表逐字记录；37 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：象棋语境的车走直线啊取ju，落子取luo zi，不取默认lao zi；棋面与手牌的露选择日常口语lou。重进、重连、重新发均chong，延迟设置、声音、画质及窗口调节取tiao，队长的长是zhang，长按、线缆和对局时长的长取chang。必须义得选dei，补语得及免得选de；仍然、下回还能用上、下回还用这套取hai，借工具用完还你取huan。留着数、松着装、急着装框的着是持续zhe；找着表示找到的原稿因表达不自然不收。物理下落到地毯的落是luo，不混作遗忘携带的la。牌发黏是nian，粘纸、粘牢、粘住与粘死是贴附动作zhan；薄片和涂薄取bao，削薄木片选xiao bao；扎袋口取za。谁统一选择shui；攻略用lve，üe统一写lve/nve；轻声按本字。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，253593 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round48_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round48.md`
- `resources/dictionary/expansion/batch508_authored_draft.txt`
- `resources/dictionary/expansion/batch508_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch509_authored_draft.txt`
- `resources/dictionary/expansion/batch509_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch510_authored_draft.txt`
- `resources/dictionary/expansion/batch510_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch511_draft.txt`
- `resources/dictionary/expansion/batch511_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch512_draft.txt`
- `resources/dictionary/expansion/batch512_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch513_draft.txt`
- `resources/dictionary/expansion/batch513_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_508.md`
- `resources/dictionary/expansion/batch_509.md`
- `resources/dictionary/expansion/batch_510.md`
- `resources/dictionary/expansion/batch_511.md`
- `resources/dictionary/expansion/batch_512.md`
- `resources/dictionary/expansion/batch_513.md`
- `resources/dictionary/expansion/lexicon_batch_508.applied.json`
- `resources/dictionary/expansion/lexicon_batch_508.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_508.json`
- `resources/dictionary/expansion/lexicon_batch_508.tsv`
- `resources/dictionary/expansion/lexicon_batch_508.validation.json`
- `resources/dictionary/expansion/lexicon_batch_509.applied.json`
- `resources/dictionary/expansion/lexicon_batch_509.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_509.json`
- `resources/dictionary/expansion/lexicon_batch_509.tsv`
- `resources/dictionary/expansion/lexicon_batch_509.validation.json`
- `resources/dictionary/expansion/lexicon_batch_510.applied.json`
- `resources/dictionary/expansion/lexicon_batch_510.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_510.json`
- `resources/dictionary/expansion/lexicon_batch_510.tsv`
- `resources/dictionary/expansion/lexicon_batch_510.validation.json`
- `resources/dictionary/expansion/lexicon_batch_511.json`
- `resources/dictionary/expansion/lexicon_batch_511.tsv`
- `resources/dictionary/expansion/lexicon_batch_512.json`
- `resources/dictionary/expansion/lexicon_batch_512.tsv`
- `resources/dictionary/expansion/lexicon_batch_513.json`
- `resources/dictionary/expansion/lexicon_batch_513.tsv`
- `resources/dictionary/expansion/probes/batch508_probe.tsv`
- `resources/dictionary/expansion/probes/batch509_probe.tsv`
- `resources/dictionary/expansion/probes/batch510_probe.tsv`
- `resources/dictionary/expansion/probes/batch511_probe.tsv`
- `resources/dictionary/expansion/probes/batch512_probe.tsv`
- `resources/dictionary/expansion/probes/batch513_probe.tsv`
- `resources/dictionary/expansion/round48_exclusions.tsv`
- `resources/dictionary/expansion/round48_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
