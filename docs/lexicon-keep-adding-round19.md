# 词库续加第19轮（批次 310–317）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `289746eecd510f358b9fb68dc8bfa8e087967a95`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：220213 → 221998，新增 **1785 条**。
- 权重 6000 / 5500 / 5000：0 / 1784 / 1 条。
- 最终 SHA-256：`9B7622B33AF3041E8BCF4DAE020EDE2E50F3B7F3CC6A6B619E95674469820D6B`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 310 | 留学与外语学习 | curate | 414 | 0 | 414 | 0 | 93.48% | 100.00% | `4aa3a193342db71dddcb40407ea0c0262bba91ad` |
| 311 | 驾考与交通违章 | curate | 471 | 0 | 470 | 1 | 88.54% | 100.00% | `d5ce4e69c25b913c6c92dd2d4686b943e6f38b08` |
| 312 | 二手交易与闲置 | curate | 405 | 0 | 405 | 0 | 96.05% | 100.00% | `ae891cc08d8c3ed7422095ef9ce5687e0b152a2b` |
| 313 | 消费维权与投诉 | curate | 416 | 0 | 416 | 0 | 93.99% | 100.00% | `a8739546976ce4869ac461e210bf3e310eaddb30` |
| 314 | 留学与外语学习较长说法 | merge | 20 | 0 | 20 | 0 | 55.00% | 100.00% | `0a42f4292b6a53d1503703ab15434a73772e6726` |
| 315 | 驾考与交通违章较长说法 | merge | 20 | 0 | 20 | 0 | 75.00% | 100.00% | `5f5798ada99043e16019d4553166fa8c1effc6b8` |
| 316 | 二手交易与闲置较长说法 | merge | 19 | 0 | 19 | 0 | 68.42% | 100.00% | `36d3b9c6c76d6eadf011a59a0fca3726686a22f8` |
| 317 | 消费维权与投诉较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `a6b74ae5b3eacba62d7dcb2d5c4829e91984a79a` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round19_exclusions.tsv`，共 783 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `quality:incomplete_or_unnatural_expression`：97 条。
- `duplicate:local_core_zh_pinyin.tsv`：486 条。
- `duplicate:standard_zh_pinyin.tsv`：34 条。
- `outside_batch_length_scope`：2 条。
- `quality:incomplete_or_uncommon_label`：109 条。
- `batch_size_limit`：11 条。
- `quality:omit_over_specific_product_combinations`：39 条。
- `duplicate:batch312`：5 条。

候选检查没有发现需要撤回整批的情况。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1264 个含多音字的词在各批复核表逐字记录；3 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 留学外语：行前、行李的行 xing；重音的重 zhong，重复的重 chong。重音是音节强调，不是“重新”的重。单语、单项的单 dan；语伴、语调按完整语境复核。
- 驾考：调节、调整的调 tiao，往前调一点的调 tiao；重新考试、重考的重 chong，倒库、倒车的倒 dao。驾驶证、处置、处罚逐字记录。
- 二手：行情的行 hang，转让、转卖的转 zhuan；拆封、折旧、褪色、磕碰按物品语境。轻声按本字写，配件齐全与未拆封不采用异读。
- 投诉：调解的调 tiao，重复投诉的重 chong，处理、处置、处罚的处 chu。只收生活消费用语，不把未经确认的请求写成法律结论。
- 驾校场地、驾校转校中的校 xiao；熄火重启的重 chong。学历校验中的校 jiao，与学校区分。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1426/1426 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，221998 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round19_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.96% | +0.00 | 98.10% → 98.10% |
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
- `docs/lexicon-keep-adding-round19.md`
- `resources/dictionary/expansion/batch310_authored_draft.txt`
- `resources/dictionary/expansion/batch310_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch311_authored_draft.txt`
- `resources/dictionary/expansion/batch311_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch312_authored_draft.txt`
- `resources/dictionary/expansion/batch312_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch313_authored_draft.txt`
- `resources/dictionary/expansion/batch313_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch314_draft.txt`
- `resources/dictionary/expansion/batch314_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch315_draft.txt`
- `resources/dictionary/expansion/batch315_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch316_draft.txt`
- `resources/dictionary/expansion/batch316_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch317_draft.txt`
- `resources/dictionary/expansion/batch317_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_310.md`
- `resources/dictionary/expansion/batch_311.md`
- `resources/dictionary/expansion/batch_312.md`
- `resources/dictionary/expansion/batch_313.md`
- `resources/dictionary/expansion/batch_314.md`
- `resources/dictionary/expansion/batch_315.md`
- `resources/dictionary/expansion/batch_316.md`
- `resources/dictionary/expansion/batch_317.md`
- `resources/dictionary/expansion/lexicon_batch_310.applied.json`
- `resources/dictionary/expansion/lexicon_batch_310.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_310.json`
- `resources/dictionary/expansion/lexicon_batch_310.tsv`
- `resources/dictionary/expansion/lexicon_batch_310.validation.json`
- `resources/dictionary/expansion/lexicon_batch_311.applied.json`
- `resources/dictionary/expansion/lexicon_batch_311.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_311.json`
- `resources/dictionary/expansion/lexicon_batch_311.tsv`
- `resources/dictionary/expansion/lexicon_batch_311.validation.json`
- `resources/dictionary/expansion/lexicon_batch_312.applied.json`
- `resources/dictionary/expansion/lexicon_batch_312.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_312.json`
- `resources/dictionary/expansion/lexicon_batch_312.tsv`
- `resources/dictionary/expansion/lexicon_batch_312.validation.json`
- `resources/dictionary/expansion/lexicon_batch_313.applied.json`
- `resources/dictionary/expansion/lexicon_batch_313.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_313.json`
- `resources/dictionary/expansion/lexicon_batch_313.tsv`
- `resources/dictionary/expansion/lexicon_batch_313.validation.json`
- `resources/dictionary/expansion/lexicon_batch_314.json`
- `resources/dictionary/expansion/lexicon_batch_314.tsv`
- `resources/dictionary/expansion/lexicon_batch_315.json`
- `resources/dictionary/expansion/lexicon_batch_315.tsv`
- `resources/dictionary/expansion/lexicon_batch_316.json`
- `resources/dictionary/expansion/lexicon_batch_316.tsv`
- `resources/dictionary/expansion/lexicon_batch_317.json`
- `resources/dictionary/expansion/lexicon_batch_317.tsv`
- `resources/dictionary/expansion/probes/batch310_probe.tsv`
- `resources/dictionary/expansion/probes/batch311_probe.tsv`
- `resources/dictionary/expansion/probes/batch312_probe.tsv`
- `resources/dictionary/expansion/probes/batch313_probe.tsv`
- `resources/dictionary/expansion/probes/batch314_probe.tsv`
- `resources/dictionary/expansion/probes/batch315_probe.tsv`
- `resources/dictionary/expansion/probes/batch316_probe.tsv`
- `resources/dictionary/expansion/probes/batch317_probe.tsv`
- `resources/dictionary/expansion/round19_exclusions.tsv`
- `resources/dictionary/expansion/round19_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
