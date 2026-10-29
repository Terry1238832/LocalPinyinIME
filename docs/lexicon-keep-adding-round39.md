# 词库续加第39轮（批次 452–459）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `023ead6930a2144e831f38b2a8d9c479e4d40c37`，引擎版本 1.6.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：242593 → 244018，新增 **1425 条**。
- 权重 6000 / 5500 / 5000：0 / 1425 / 0 条。
- 最终 SHA-256：`F69674CCCC1049B211D7BC77B15156938732627486BCFB9F2B9FFDAE3CA43C93`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：日常聊天口语。逐条拟写换洗晾晒与穿衣、冷热风雨与出门准备、电话听说与屏幕操作、清扫与物件归置四个方向；选取生活中会整段输入的自然问答和提醒。全部词面按现有层、domains及本轮前批去重，不从测试数据或评测答案选词，不机械替换人物、数字或句尾。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 452 | 换洗晾晒与穿衣聊天 | curate | 351 | 0 | 351 | 0 | 61.82% | 100.00% | `2856633a6aec61f35019421fec16d30a74fd8093` |
| 453 | 冷热风雨与出门准备聊天 | curate | 346 | 0 | 346 | 0 | 65.03% | 100.00% | `5f768dbb47838886a48671a11a64c1368901d691` |
| 454 | 电话听说与屏幕操作聊天 | curate | 318 | 0 | 318 | 0 | 72.01% | 100.00% | `098dffe7cceb46599da0090e69bca494c40986f9` |
| 455 | 收拾打扫与物品归置聊天 | curate | 330 | 0 | 330 | 0 | 60.61% | 100.00% | `9d0a319d56549fac256083d371b44ca2e54084c3` |
| 456 | 换洗晾晒与穿衣较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `27dd0984f65151ea15caa2efcb76f8ce737d4d71` |
| 457 | 冷热风雨与出门准备较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `a115129d3cad2ac19776f56d45070150213752ac` |
| 458 | 电话听说与屏幕操作较长说法 | merge | 20 | 0 | 20 | 0 | 80.00% | 100.00% | `63ff70715b1a1a190e0f9b31b5f3a0f891a43b03` |
| 459 | 收拾打扫与物品归置较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `e781b70838d2ccbe1b7447317b5fb4da425f18d8` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round39_exclusions.tsv`，共 376 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：44 条。
- `quality: clipped explanation, weak naturalness or imprecise object wording`：16 条。
- `pronunciation: defer mechanical 卡 context rather than guess its reading`：5 条。
- `quality: clipped or unnatural particle stacking in the first authored draft`：7 条。
- `outside_batch_length_scope`：301 条。
- `orthography: author the clothing noun 裆 rather than 档`：1 条。
- `quality: replace the clipped 收到 with complete 收拾 wording`：1 条。
- `orthography: choose 黏 for the sticky floor surface`：1 条。

按全部既有层、domains及本轮先前批次去重；剔除残缺或不常见口语、凑合的物件名、图像操作不准确说法和语气叠加。不机械替换人物、日期、数字或句尾。对裆、黏以及收拾一半三处重新拟定表达，旧稿记入排除表；暂不收读音把握不足的机械卡字候选。各项读音决定在冻结前完成。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1327 个含多音字的词在各批复核表逐字记录；48 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词逐字复核：必须义的得取dei，补语及记得的得取de；袖子和电线长度、长按的长取chang；系鞋带、系结的系取ji；调温调风口及音量取tiao，空调取kong tiao；重复发言的说重了和重新连接的重取chong；忘带或遗漏的落取la，灰落一层、雪落头发上的落取luo；薄冰复合词取bo，衣物薄了点取bao；弹出窗口取tan，文本那行取hang；磕着、绊着、烫着、夹着及够不着的着取zhao，持续穿着晾着等取zhe；皮筋捆扎的扎取za，衣物扎人取zha；抹布取ma bu，普通擦抹动作不混用此读音；熨衣服取yun。褪色两条选择tui se；谁字选择shui的两条逐词覆盖，不增异读变体。衣物进水了的了为le。两处草稿字形掉档、粘脚分别改拟掉裆、黏脚，旧草稿词面排除。全部轻声按本字，üe取lve/nve。 口语把手露着选择lou；暴晒选择bao shai。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：46/46 通过。
- x86 Release 原生构建与 ctest：46/46 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。本轮无 2–4 字新词；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，十组首选率和前九率均未下降，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，244018 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round39_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round39.md`
- `resources/dictionary/expansion/batch452_authored_draft.txt`
- `resources/dictionary/expansion/batch452_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch453_authored_draft.txt`
- `resources/dictionary/expansion/batch453_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch454_authored_draft.txt`
- `resources/dictionary/expansion/batch454_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch455_authored_draft.txt`
- `resources/dictionary/expansion/batch455_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch456_draft.txt`
- `resources/dictionary/expansion/batch456_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch457_draft.txt`
- `resources/dictionary/expansion/batch457_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch458_draft.txt`
- `resources/dictionary/expansion/batch458_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch459_draft.txt`
- `resources/dictionary/expansion/batch459_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_452.md`
- `resources/dictionary/expansion/batch_453.md`
- `resources/dictionary/expansion/batch_454.md`
- `resources/dictionary/expansion/batch_455.md`
- `resources/dictionary/expansion/batch_456.md`
- `resources/dictionary/expansion/batch_457.md`
- `resources/dictionary/expansion/batch_458.md`
- `resources/dictionary/expansion/batch_459.md`
- `resources/dictionary/expansion/lexicon_batch_452.applied.json`
- `resources/dictionary/expansion/lexicon_batch_452.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_452.json`
- `resources/dictionary/expansion/lexicon_batch_452.tsv`
- `resources/dictionary/expansion/lexicon_batch_452.validation.json`
- `resources/dictionary/expansion/lexicon_batch_453.applied.json`
- `resources/dictionary/expansion/lexicon_batch_453.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_453.json`
- `resources/dictionary/expansion/lexicon_batch_453.tsv`
- `resources/dictionary/expansion/lexicon_batch_453.validation.json`
- `resources/dictionary/expansion/lexicon_batch_454.applied.json`
- `resources/dictionary/expansion/lexicon_batch_454.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_454.json`
- `resources/dictionary/expansion/lexicon_batch_454.tsv`
- `resources/dictionary/expansion/lexicon_batch_454.validation.json`
- `resources/dictionary/expansion/lexicon_batch_455.applied.json`
- `resources/dictionary/expansion/lexicon_batch_455.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_455.json`
- `resources/dictionary/expansion/lexicon_batch_455.tsv`
- `resources/dictionary/expansion/lexicon_batch_455.validation.json`
- `resources/dictionary/expansion/lexicon_batch_456.json`
- `resources/dictionary/expansion/lexicon_batch_456.tsv`
- `resources/dictionary/expansion/lexicon_batch_457.json`
- `resources/dictionary/expansion/lexicon_batch_457.tsv`
- `resources/dictionary/expansion/lexicon_batch_458.json`
- `resources/dictionary/expansion/lexicon_batch_458.tsv`
- `resources/dictionary/expansion/lexicon_batch_459.json`
- `resources/dictionary/expansion/lexicon_batch_459.tsv`
- `resources/dictionary/expansion/probes/batch452_probe.tsv`
- `resources/dictionary/expansion/probes/batch453_probe.tsv`
- `resources/dictionary/expansion/probes/batch454_probe.tsv`
- `resources/dictionary/expansion/probes/batch455_probe.tsv`
- `resources/dictionary/expansion/probes/batch456_probe.tsv`
- `resources/dictionary/expansion/probes/batch457_probe.tsv`
- `resources/dictionary/expansion/probes/batch458_probe.tsv`
- `resources/dictionary/expansion/probes/batch459_probe.tsv`
- `resources/dictionary/expansion/round39_exclusions.tsv`
- `resources/dictionary/expansion/round39_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
