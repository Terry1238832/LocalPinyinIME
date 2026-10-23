# 词库续加第34轮（批次 414–419）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `e2f129dcf5668c05170320c43871693eca3cb1a7`，引擎版本 1.2.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：236787 → 237251，新增 **464 条**。
- 权重 6000 / 5500 / 5000：0 / 464 / 0 条。
- 最终 SHA-256：`E01B3291A4CE997F1DA7D56D31B98B22E3A33A52CC86D5236DCB61F5BA19D050`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：A。已打开饮食批次 165、家政批次 225、公共交通票务批次 209 的 TSV；分别补家常面点和面擀皮蒸煮、锅具餐具清洁养护、铁路车厢座席与在途沟通。与第二阶段已补的凉菜腌拌、后厨出餐和轮胎检修不同，侧重家庭操作及车内具体交流。逐条自拟，不看评测缺词；仅按既有词面去重，删去参数字段式组合和不自然残句。短词每批 100–400 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 414 | 家常面点和面擀皮与蒸煮 | curate | 135 | 0 | 135 | 0 | 82.22% | 100.00% | `97ad62a27d159f93b6b479a59063672efdaa8610` |
| 415 | 锅具餐具清洁与日常养护 | curate | 143 | 0 | 143 | 0 | 84.62% | 100.00% | `9200dee80c399844e2ce3dcff73ba113c3237384` |
| 416 | 铁路车厢座席与在途沟通 | curate | 156 | 0 | 156 | 0 | 78.21% | 100.00% | `00f9b8c38acea4f7e0570bdce6390cb0626c4735` |
| 417 | 家常面点和面擀皮与蒸煮较长说法 | merge | 10 | 0 | 10 | 0 | 60.00% | 100.00% | `7af8f83eadd601c160e56ac2c9bc87bfc58e2e6a` |
| 418 | 锅具餐具清洁与日常养护较长说法 | merge | 10 | 0 | 10 | 0 | 100.00% | 100.00% | `96c1d82cd42c9c5056e84ed0a2e210b524eab18c` |
| 419 | 铁路车厢座席与在途沟通较长说法 | merge | 10 | 0 | 10 | 0 | 80.00% | 100.00% | `dcca71fc994a1a0898735cf56b6042fb95a2cbe0` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round34_exclusions.tsv`，共 179 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：69 条。
- `quality: repetitive parameter label, less natural whole-input wording or narrow hardware detail`：44 条。
- `quality: less idiomatic craft wording, repetitive cleaning specification or incomplete travel reply`：66 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

376 个含多音字的词在各批复核表逐字记录；15 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

和面中的和取 huo，醒面取 xing，面团发酵取 fa jiao，擀成长条的长取 chang；锅具不粘取 zhan，薄的被子取 bao；铁路行李取 xing li，铺位的铺取 pu，别落东西的落取 la，调节座椅取 tiao。含多音字的词逐字复核。 不粘锅相关词先逐条自拟，复核时仅核对“粘”作动词读 zhan 的规则，参考[教育部答复](https://www.moe.gov.cn/moe_879/moe_1252/s8447/201412/t20141204_179422.html)；没有从该页面或第三方词典导入候选词。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 232/232 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，237251 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round34_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.97% → 85.97% | +0.00 | 98.11% → 98.11% |
| 逐词输入·无前文 | 20626 | 77.98% → 77.98% | +0.00 | 97.19% → 97.19% |
| 留出整句 | 4126 | 58.07% → 58.07% | +0.00 | 73.44% → 73.44% |
| 口语对话·整句 | 3368 | 80.40% → 80.40% | +0.00 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round34.md`
- `resources/dictionary/expansion/batch414_authored_draft.txt`
- `resources/dictionary/expansion/batch414_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch415_authored_draft.txt`
- `resources/dictionary/expansion/batch415_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch416_authored_draft.txt`
- `resources/dictionary/expansion/batch416_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch417_draft.txt`
- `resources/dictionary/expansion/batch417_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch418_draft.txt`
- `resources/dictionary/expansion/batch418_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch419_draft.txt`
- `resources/dictionary/expansion/batch419_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_414.md`
- `resources/dictionary/expansion/batch_415.md`
- `resources/dictionary/expansion/batch_416.md`
- `resources/dictionary/expansion/batch_417.md`
- `resources/dictionary/expansion/batch_418.md`
- `resources/dictionary/expansion/batch_419.md`
- `resources/dictionary/expansion/lexicon_batch_414.applied.json`
- `resources/dictionary/expansion/lexicon_batch_414.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_414.json`
- `resources/dictionary/expansion/lexicon_batch_414.tsv`
- `resources/dictionary/expansion/lexicon_batch_414.validation.json`
- `resources/dictionary/expansion/lexicon_batch_415.applied.json`
- `resources/dictionary/expansion/lexicon_batch_415.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_415.json`
- `resources/dictionary/expansion/lexicon_batch_415.tsv`
- `resources/dictionary/expansion/lexicon_batch_415.validation.json`
- `resources/dictionary/expansion/lexicon_batch_416.applied.json`
- `resources/dictionary/expansion/lexicon_batch_416.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_416.json`
- `resources/dictionary/expansion/lexicon_batch_416.tsv`
- `resources/dictionary/expansion/lexicon_batch_416.validation.json`
- `resources/dictionary/expansion/lexicon_batch_417.json`
- `resources/dictionary/expansion/lexicon_batch_417.tsv`
- `resources/dictionary/expansion/lexicon_batch_418.json`
- `resources/dictionary/expansion/lexicon_batch_418.tsv`
- `resources/dictionary/expansion/lexicon_batch_419.json`
- `resources/dictionary/expansion/lexicon_batch_419.tsv`
- `resources/dictionary/expansion/probes/batch414_probe.tsv`
- `resources/dictionary/expansion/probes/batch415_probe.tsv`
- `resources/dictionary/expansion/probes/batch416_probe.tsv`
- `resources/dictionary/expansion/probes/batch417_probe.tsv`
- `resources/dictionary/expansion/probes/batch418_probe.tsv`
- `resources/dictionary/expansion/probes/batch419_probe.tsv`
- `resources/dictionary/expansion/round34_exclusions.tsv`
- `resources/dictionary/expansion/round34_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
