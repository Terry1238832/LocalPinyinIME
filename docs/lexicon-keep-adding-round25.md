# 词库续加第25轮（批次 358–365）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `4fa2936af9fd6655bfc7fcbd840c1cd28890de03`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：229511 → 230848，新增 **1337 条**。
- 权重 6000 / 5500 / 5000：0 / 1330 / 7 条。
- 最终 SHA-256：`B546AD394B8CEE3C549C2F93B9124B5598F012081CBFAAE8F710B8BD846A12FF`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

原主题清单已完成，本轮自拟陶艺黏土、录音配音、儿童益智玩具、食品保鲜与包装标签四个日常专题。分别服务手作体验、家庭录音与配音沟通、亲子玩具购买与玩法、厨房存放与看标签等实际输入；与既有文具手作、音乐、母婴、健康饮食有关，但没有同名专批，逐条与全部历史词面去重。少收工厂参数、罕见技法及不自然的组合说法。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 358 | 陶艺黏土与手工塑形 | curate | 301 | 0 | 295 | 6 | 86.38% | 100.00% | `b8abece1dc0706c01b6bb3cf8255041bc591e7db` |
| 359 | 录音配音与声音处理 | curate | 305 | 0 | 305 | 0 | 94.43% | 100.00% | `a2ca4ee9c431dacae7a592ec886985a318ac739e` |
| 360 | 儿童玩具与益智玩具 | curate | 321 | 0 | 320 | 1 | 95.02% | 100.00% | `952519e14f2c5ece7178e55cfa92b626dae5184f` |
| 361 | 食品保鲜储藏与包装标签 | curate | 330 | 0 | 330 | 0 | 93.03% | 100.00% | `334bb98749861225c64055bb74b2415eacf87500` |
| 362 | 陶艺黏土与手工塑形较长说法 | merge | 20 | 0 | 20 | 0 | 50.00% | 100.00% | `88570ba231144668f96d487f9b6794f05366b511` |
| 363 | 录音配音与声音处理较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `51ff192e4892233acb20fe0374cd0565f509f534` |
| 364 | 儿童玩具与益智玩具较长说法 | merge | 20 | 0 | 20 | 0 | 85.00% | 100.00% | `19c771e15864a1a8e783cc9eca2b16bcb5f2dc52` |
| 365 | 食品保鲜储藏与包装标签较长说法 | merge | 20 | 0 | 20 | 0 | 90.00% | 100.00% | `7db7ea378e5bd4a92ec81c7b8557034963632078` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round25_exclusions.tsv`，共 1367 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：490 条。
- `quality: overly technical, redundant description or unnatural everyday expression`：619 条。
- `quality: narrow technical or name-derived label; unsuitable for everyday vocabulary`：20 条。
- `duplicate:standard_zh_pinyin.tsv`：32 条。
- `probe_top9: shiyou already has 5000-band homophones; new term capped at 5000 and absent from top nine`：1 条。
- `batch_size_limit`：204 条。
- `outside_batch_length_scope`：1 条。

首次探针中试釉 shiyou 权重受已有同拼 5000 档限制，未进前九；不提权，删除该词后从本轮基线重建未提交批次，并重新完成双架构构建、全部探针、curate evaluate、准确率快照和结构校验。未撤回整批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1018 个含多音字的词在各批复核表逐字记录；9 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

逐词按语境复核：陶艺拉坯为 la pi；软陶的模具读 mu；釉色读 you se；录音降调、变调读 diao，调音、调节读 tiao；玩具回力车读 che，食物解冻读 jie dong。其余选择逐字记录于各批 pronunciation_review。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1121/1121 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，230848 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round25_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.96% → 85.95% | -0.01 | 98.09% → 98.09% |
| 逐词输入·无前文 | 20626 | 77.97% → 77.97% | +0.00 | 97.17% → 97.17% |
| 留出整句 | 4126 | 58.05% → 58.05% | +0.00 | 73.44% → 73.44% |
| 口语对话·整句 | 3368 | 80.43% → 80.40% | -0.03 | 91.81% → 91.81% |
| 口语对话·逐词 | 19504 | 94.12% → 94.12% | +0.00 | 99.50% → 99.50% |
| 项目留出 | 138 | 79.71% → 79.71% | +0.00 | 86.23% → 86.23% |
| 新增用例 | 67 | 62.69% → 62.69% | +0.00 | 77.61% → 77.61% |
| 独立用例 | 62 | 41.94% → 41.94% | +0.00 | 62.90% → 62.90% |
| 日常口语·开发 | 149 | 96.64% → 96.64% | +0.00 | 100.00% → 100.00% |
| 日常口语·留出 | 150 | 87.33% → 87.33% | +0.00 | 97.33% → 97.33% |

## 改动文件

- `docs/LEXICON_COVERAGE.md`
- `docs/lexicon-coverage.json`
- `docs/lexicon-keep-adding-round25.md`
- `resources/dictionary/expansion/batch358_authored_draft.txt`
- `resources/dictionary/expansion/batch358_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch359_authored_draft.txt`
- `resources/dictionary/expansion/batch359_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch360_authored_draft.txt`
- `resources/dictionary/expansion/batch360_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch361_authored_draft.txt`
- `resources/dictionary/expansion/batch361_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch362_draft.txt`
- `resources/dictionary/expansion/batch362_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch363_draft.txt`
- `resources/dictionary/expansion/batch363_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch364_draft.txt`
- `resources/dictionary/expansion/batch364_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch365_draft.txt`
- `resources/dictionary/expansion/batch365_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_358.md`
- `resources/dictionary/expansion/batch_359.md`
- `resources/dictionary/expansion/batch_360.md`
- `resources/dictionary/expansion/batch_361.md`
- `resources/dictionary/expansion/batch_362.md`
- `resources/dictionary/expansion/batch_363.md`
- `resources/dictionary/expansion/batch_364.md`
- `resources/dictionary/expansion/batch_365.md`
- `resources/dictionary/expansion/lexicon_batch_358.applied.json`
- `resources/dictionary/expansion/lexicon_batch_358.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_358.json`
- `resources/dictionary/expansion/lexicon_batch_358.tsv`
- `resources/dictionary/expansion/lexicon_batch_358.validation.json`
- `resources/dictionary/expansion/lexicon_batch_359.applied.json`
- `resources/dictionary/expansion/lexicon_batch_359.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_359.json`
- `resources/dictionary/expansion/lexicon_batch_359.tsv`
- `resources/dictionary/expansion/lexicon_batch_359.validation.json`
- `resources/dictionary/expansion/lexicon_batch_360.applied.json`
- `resources/dictionary/expansion/lexicon_batch_360.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_360.json`
- `resources/dictionary/expansion/lexicon_batch_360.tsv`
- `resources/dictionary/expansion/lexicon_batch_360.validation.json`
- `resources/dictionary/expansion/lexicon_batch_361.applied.json`
- `resources/dictionary/expansion/lexicon_batch_361.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_361.json`
- `resources/dictionary/expansion/lexicon_batch_361.tsv`
- `resources/dictionary/expansion/lexicon_batch_361.validation.json`
- `resources/dictionary/expansion/lexicon_batch_362.json`
- `resources/dictionary/expansion/lexicon_batch_362.tsv`
- `resources/dictionary/expansion/lexicon_batch_363.json`
- `resources/dictionary/expansion/lexicon_batch_363.tsv`
- `resources/dictionary/expansion/lexicon_batch_364.json`
- `resources/dictionary/expansion/lexicon_batch_364.tsv`
- `resources/dictionary/expansion/lexicon_batch_365.json`
- `resources/dictionary/expansion/lexicon_batch_365.tsv`
- `resources/dictionary/expansion/probes/batch358_probe.tsv`
- `resources/dictionary/expansion/probes/batch359_probe.tsv`
- `resources/dictionary/expansion/probes/batch360_probe.tsv`
- `resources/dictionary/expansion/probes/batch361_probe.tsv`
- `resources/dictionary/expansion/probes/batch362_probe.tsv`
- `resources/dictionary/expansion/probes/batch363_probe.tsv`
- `resources/dictionary/expansion/probes/batch364_probe.tsv`
- `resources/dictionary/expansion/probes/batch365_probe.tsv`
- `resources/dictionary/expansion/round25_exclusions.tsv`
- `resources/dictionary/expansion/round25_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
