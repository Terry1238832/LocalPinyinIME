# 词库续加第31轮（批次 404–409）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `14fb0af4af734aef6ef0349a4cc788e6f5cbd8dd`，引擎版本 1.2.0。分支通过合并吸收 main，不 rebase、不 force push。第一阶段第十至二十八轮已合并发布至 1.2.0；第二阶段从第二十九轮开始，本 PR 的总表从第二十九轮起累计。

- `local_core`：236017 → 236431，新增 **414 条**。
- 权重 6000 / 5500 / 5000：0 / 414 / 0 条。
- 最终 SHA-256：`E2166A2FA9C91738344C4B202E3BE784101DD28083CAAF9C16B5AD6EDC552584`。
- CMake 上限 255390，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

本轮方向：A。已打开母婴批次 180、汽车批次 183 和餐饮门店批次 278 的 TSV；本轮分别补喂奶拍嗝与奶具使用的日常状态、轮胎检修充气补胎的车主说法、后厨备料与出餐协作。前两类旧批次更偏用品名和汽车综合部件，后者旧批次更偏经营报表及岗位；这里只依据生活场景补自然表达，并删去专业参数和行政字段式组合。短词每批 100–400 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 404 | 婴儿喂奶拍嗝与奶具使用 | curate | 119 | 0 | 119 | 0 | 89.08% | 100.00% | `339471f4a8821746229ffab95520fe28019f5fb8` |
| 405 | 汽车轮胎检修与充气补胎 | curate | 148 | 0 | 148 | 0 | 83.78% | 100.00% | `4eff2dec7543b96196532c8c4ca0b096e642d3e5` |
| 406 | 餐饮后厨备料与出餐协作 | curate | 117 | 0 | 117 | 0 | 84.62% | 100.00% | `ebe4420915befc699031f94c4347e3118f281ef5` |
| 407 | 婴儿喂奶拍嗝与奶具使用较长说法 | merge | 10 | 0 | 10 | 0 | 80.00% | 100.00% | `38aa9f54f386690dadc9629a72108037f8be9d64` |
| 408 | 汽车轮胎检修与充气补胎较长说法 | merge | 10 | 0 | 10 | 0 | 40.00% | 100.00% | `74bf621e806786f6e655175c75ba85b2a4ffcd26` |
| 409 | 餐饮后厨备料与出餐协作较长说法 | merge | 10 | 0 | 10 | 0 | 90.00% | 100.00% | `14cae23ecd6ba718d88f81c63794307d4929a954` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round31_exclusions.tsv`，共 198 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：49 条。
- `quality: repetitive field label, less natural spoken fragment or narrow component combination`：101 条。
- `quality: overly technical label, unnatural fragment or repetitive operational field`：48 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

291 个含多音字的词在各批复核表逐字记录；5 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

母乳解冻的解取 jie、背奶取 bei，奶还没喝完的还取 hai、没取 mei；轮胎扎钉的扎取 zha，气不足的气取 qi；后厨调料取 tiao liao，重做和菜票打重了取 chong，单据的单取 dan。每条全拼逐条审阅，含多音字按字记录。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 277/277 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 255390` 通过，236431 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round31_validation.json`；短词批次另存工具生成的 `.validation.json`。
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
- `docs/lexicon-keep-adding-round31.md`
- `resources/dictionary/expansion/batch404_authored_draft.txt`
- `resources/dictionary/expansion/batch404_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch405_authored_draft.txt`
- `resources/dictionary/expansion/batch405_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch406_authored_draft.txt`
- `resources/dictionary/expansion/batch406_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch407_draft.txt`
- `resources/dictionary/expansion/batch407_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch408_draft.txt`
- `resources/dictionary/expansion/batch408_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch409_draft.txt`
- `resources/dictionary/expansion/batch409_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_404.md`
- `resources/dictionary/expansion/batch_405.md`
- `resources/dictionary/expansion/batch_406.md`
- `resources/dictionary/expansion/batch_407.md`
- `resources/dictionary/expansion/batch_408.md`
- `resources/dictionary/expansion/batch_409.md`
- `resources/dictionary/expansion/lexicon_batch_404.applied.json`
- `resources/dictionary/expansion/lexicon_batch_404.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_404.json`
- `resources/dictionary/expansion/lexicon_batch_404.tsv`
- `resources/dictionary/expansion/lexicon_batch_404.validation.json`
- `resources/dictionary/expansion/lexicon_batch_405.applied.json`
- `resources/dictionary/expansion/lexicon_batch_405.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_405.json`
- `resources/dictionary/expansion/lexicon_batch_405.tsv`
- `resources/dictionary/expansion/lexicon_batch_405.validation.json`
- `resources/dictionary/expansion/lexicon_batch_406.applied.json`
- `resources/dictionary/expansion/lexicon_batch_406.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_406.json`
- `resources/dictionary/expansion/lexicon_batch_406.tsv`
- `resources/dictionary/expansion/lexicon_batch_406.validation.json`
- `resources/dictionary/expansion/lexicon_batch_407.json`
- `resources/dictionary/expansion/lexicon_batch_407.tsv`
- `resources/dictionary/expansion/lexicon_batch_408.json`
- `resources/dictionary/expansion/lexicon_batch_408.tsv`
- `resources/dictionary/expansion/lexicon_batch_409.json`
- `resources/dictionary/expansion/lexicon_batch_409.tsv`
- `resources/dictionary/expansion/probes/batch404_probe.tsv`
- `resources/dictionary/expansion/probes/batch405_probe.tsv`
- `resources/dictionary/expansion/probes/batch406_probe.tsv`
- `resources/dictionary/expansion/probes/batch407_probe.tsv`
- `resources/dictionary/expansion/probes/batch408_probe.tsv`
- `resources/dictionary/expansion/probes/batch409_probe.tsv`
- `resources/dictionary/expansion/round31_exclusions.tsv`
- `resources/dictionary/expansion/round31_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`

## 后续选题评估与停止位置

第 30 轮方向 B 新增 483 条，本轮方向 A 新增条数见上表，仍低于 500 条，触发“连续两轮各自新增都不到 500 条（自查轮不算）”的停止条件。完成全部验收与提交后推送，并写 docs/lexicon-stage2-summary.md 总结两个阶段的覆盖、尚未进入的自查轮及需要维护者决定的后续方向。本次按数量阈值停止，不声称所有细分领域和表达类型已经穷尽。
