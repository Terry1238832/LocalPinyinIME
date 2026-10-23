# 词库续加第17轮（批次 294–301）

在独立检出 `localpinyinime-lexicon` 的 `lexicon/continuous` 上继续。开始前已获取最新 `origin/main`；本轮起点 `dff930a2871678af894f0065fb1123e6361a651e`，引擎版本 1.1.0。分支通过合并吸收 main，不 rebase、不 force push。第十至十四轮已随 PR #6 合并；当前持续 PR 的总表从第十五轮起累计。

- `local_core`：216403 → 218303，新增 **1900 条**。
- 权重 6000 / 5500 / 5000：0 / 1897 / 3 条。
- 最终 SHA-256：`ACB15AF395D554F93B853823B58230037551A50D9A07399A32D95888F26C569A`。
- CMake 上限 231676，本轮无需改动上限。
- 来源均为逐条自拟 `first_party_individually_authored`，未导入第三方词库；未从 `tests/data/` 取词；不含人名、商标、具体机构名条目。
- 已有行和权重逐行保留。未改引擎、界面、安装程序、版本号、场景词表或测试集，未打 tag、未打包。

## 批次

按主题清单顺序选取尚未专门完成的四个主题，并对历史全部批次按词面去重。短词每批 300–600 条；长词每批 10–30 条。短词运行 prepare → 逐字读音复核 → freeze → apply → evaluate（pypinyin 0.55.0）；长词独立使用 merge。每批一个提交，最后单独提交本记录和验收摘要。

| 批次 | 主题 | 流程 | 条数 | 6000 | 5500 | 5000 | 首选率 | 前九率 | 提交 |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 294 | 书法与绘画 | curate | 494 | 0 | 491 | 3 | 94.74% | 100.00% | `57060f76373aafa2d18e1fa738a344c40220ef27` |
| 295 | 口腔与牙科 | curate | 438 | 0 | 438 | 0 | 84.02% | 100.00% | `8ff9096b78518aba26db8903770886272ac69728` |
| 296 | 眼镜与视力 | curate | 444 | 0 | 444 | 0 | 86.94% | 100.00% | `592d5ca1b2f8dfac7627f0abf000be919aebc699` |
| 297 | 美甲与美睫 | curate | 445 | 0 | 445 | 0 | 80.00% | 100.00% | `cbb809002a904de5fa22057f5b8c4219b97d7797` |
| 298 | 书法与绘画较长说法 | merge | 20 | 0 | 20 | 0 | 70.00% | 100.00% | `c6747a8c2d60bbd91565219a163d06daae2ffb69` |
| 299 | 口腔与牙科较长说法 | merge | 20 | 0 | 20 | 0 | 35.00% | 100.00% | `a139c8bb08d9414d6f35921de5b89662a4d1c8f0` |
| 300 | 眼镜与视力较长说法 | merge | 20 | 0 | 20 | 0 | 45.00% | 100.00% | `4f43c6cdcd80011c015172cea96b4c19b1d781d5` |
| 301 | 美甲与美睫较长说法 | merge | 19 | 0 | 19 | 0 | 31.58% | 100.00% | `8fd45a278305f89753440316a7f586b25bba277d` |

## 去掉的词与修复

全部排除记录：`resources/dictionary/expansion/round17_exclusions.tsv`，共 1270 条；原因包括重复词面、语义质量筛选、批次长度范围及验收中删词。

- `duplicate:local_core_zh_pinyin.tsv`：263 条。
- `quality:incomplete_or_uncommon_label`：161 条。
- `quality:incomplete_or_unnatural_expression`：17 条。
- `duplicate:standard_zh_pinyin.tsv`：16 条。
- `candidate_top9:homophone_cap_5500_prevents_raise`：1 条。
- `batch_size_limit`：727 条。
- `pronunciation:contextual_reading_uncertain_omit`：11 条。
- `quality:name_derived_technical_label_omit`：1 条。
- `outside_batch_length_scope`：8 条。
- `quality:retain_familiar_styles_omit_over_specific_color_combinations`：65 条。

首次无上下文 probe 中“笺纸 jianzhi”未进入前九；同音已有“剪枝”权重 5500，因此新词已在允许上限，不能调至 6000。删除这一个新词，不改任何旧词、权重或测试集，并在临时副本重建本轮冻结文件和下游哈希链后重新验收。未撤回整批，也未因准确率撤批。

同全拼已有不低于 5000 的词时，新词不高于其中最低档；已有词条和权重未调整。所有新词均为 BMP 汉字，无 U+20000 及以上字符。除已有词去重和测试隔离外，没有用外部词表或评测答案选词。

## 读音决定

1353 个含多音字的词在各批复核表逐字记录；30 个词按语境修正默认全拼。一名 AI 作者逐条审阅，不声称独立人工语言学审校。

- 书画：书法行气贯通、行气连贯中的行 `hang`；行书、行草、行笔的行 `xing`。拓片、拓本、拓印的拓 `ta`；临摹、摹帖的摹 `mo`，藏锋的藏 `cang`，露锋的露 `lu`。色调、铺调子的调 `diao`，调色与调整的调 `tiao`。
- 牙科：龈缘、龈沟、龈上、龈下等均选龈 `yin`；出血、充血按临床复合词选血 `xue`。食物嵌塞的塞 `se`，塞牙的塞 `sai`；颌面的颌 `he`。黏膜的黏 `nian`，粘接的粘 `zhan`；咬合重建的重 `chong`。牙模、取模等读音未有把握的候选整词剔除。
- 眼镜视力：眼镜、镜框、镜架调校均为调 `tiao`、校 `jiao`；褪色、膜色等用色 `se`。眼轴增长的长 `zhang`，长度的长 `chang`。重影、重新的重 `chong`，重量的重 `zhong`；眼镜戴着的着 `zhe`。
- 美甲美睫：美甲重做、美睫重做、卸甲重做的重 `chong`；睫毛加长、延长、时长的长 `chang`。水钻、贴钻的钻 `zuan`；美甲调色的调 `tiao`；贝壳的壳 `ke`。轻声按本字写，如睫毛 `jiemao`。

ü 写 v，üe 写 lve / nve；轻声按本字写。每个最终全拼均与逐字选择重建一致，冻结文件及作者文件哈希已核对。

## 验收

- x64 Release 原生构建与 ctest：44/44 通过。
- x86 Release 原生构建与 ctest：44/44 通过。
- 每批 probe 均运行 `LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores`。2–4 字 1553/1553 在前九；5 字以上结果见批次表与验证 JSON。
- `accuracy_snapshot.py` 补词前后分别运行，所有组首选率下降不超过 0.2 个百分点，未更改测试集。
- `validate_local_core_lexicon.py --max-entries 231676` 通过，218303 条。
- 全层与 domains 词面去重、旧行保留、权重上限、拼音格式、UTF-8 无 BOM / LF、来源和权重策略、读音复核、冻结哈希与 probe 一致性全部通过。
- 机器可读摘要：`resources/dictionary/expansion/round17_validation.json`；短词批次另存工具生成的 `.validation.json`。
- 按任务文档重跑覆盖率（Unihan 17.0.0、jieba 0.42.1），更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`；测量在冻结后运行，未据此取词。

未运行：完整 Build.ps1 的 Electron 与安装包打包段（本任务禁止打包）；未改场景词表，未重跑六场景回归；仓库外 Tatoeba、中文维基新闻未重测。已有 19 词抽查及第二轮 101 场景词不作为本轮加词来源，不再迁移场景词。

## 准确率（补词前 → 后）

| 测试集 | 用例 | 首选率 | 首选变化（百分点） | 前九率 |
| --- | ---: | ---: | ---: | ---: |
| 逐词输入·参考前文 | 20626 | 85.95% → 85.96% | +0.01 | 98.10% → 98.10% |
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
- `docs/lexicon-keep-adding-round17.md`
- `resources/dictionary/expansion/batch294_authored_draft.txt`
- `resources/dictionary/expansion/batch294_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch295_authored_draft.txt`
- `resources/dictionary/expansion/batch295_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch296_authored_draft.txt`
- `resources/dictionary/expansion/batch296_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch297_authored_draft.txt`
- `resources/dictionary/expansion/batch297_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch298_draft.txt`
- `resources/dictionary/expansion/batch298_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch299_draft.txt`
- `resources/dictionary/expansion/batch299_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch300_draft.txt`
- `resources/dictionary/expansion/batch300_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch301_draft.txt`
- `resources/dictionary/expansion/batch301_pronunciation_review.tsv`
- `resources/dictionary/expansion/batch_294.md`
- `resources/dictionary/expansion/batch_295.md`
- `resources/dictionary/expansion/batch_296.md`
- `resources/dictionary/expansion/batch_297.md`
- `resources/dictionary/expansion/batch_298.md`
- `resources/dictionary/expansion/batch_299.md`
- `resources/dictionary/expansion/batch_300.md`
- `resources/dictionary/expansion/batch_301.md`
- `resources/dictionary/expansion/lexicon_batch_294.applied.json`
- `resources/dictionary/expansion/lexicon_batch_294.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_294.json`
- `resources/dictionary/expansion/lexicon_batch_294.tsv`
- `resources/dictionary/expansion/lexicon_batch_294.validation.json`
- `resources/dictionary/expansion/lexicon_batch_295.applied.json`
- `resources/dictionary/expansion/lexicon_batch_295.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_295.json`
- `resources/dictionary/expansion/lexicon_batch_295.tsv`
- `resources/dictionary/expansion/lexicon_batch_295.validation.json`
- `resources/dictionary/expansion/lexicon_batch_296.applied.json`
- `resources/dictionary/expansion/lexicon_batch_296.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_296.json`
- `resources/dictionary/expansion/lexicon_batch_296.tsv`
- `resources/dictionary/expansion/lexicon_batch_296.validation.json`
- `resources/dictionary/expansion/lexicon_batch_297.applied.json`
- `resources/dictionary/expansion/lexicon_batch_297.authoring.json`
- `resources/dictionary/expansion/lexicon_batch_297.json`
- `resources/dictionary/expansion/lexicon_batch_297.tsv`
- `resources/dictionary/expansion/lexicon_batch_297.validation.json`
- `resources/dictionary/expansion/lexicon_batch_298.json`
- `resources/dictionary/expansion/lexicon_batch_298.tsv`
- `resources/dictionary/expansion/lexicon_batch_299.json`
- `resources/dictionary/expansion/lexicon_batch_299.tsv`
- `resources/dictionary/expansion/lexicon_batch_300.json`
- `resources/dictionary/expansion/lexicon_batch_300.tsv`
- `resources/dictionary/expansion/lexicon_batch_301.json`
- `resources/dictionary/expansion/lexicon_batch_301.tsv`
- `resources/dictionary/expansion/probes/batch294_probe.tsv`
- `resources/dictionary/expansion/probes/batch295_probe.tsv`
- `resources/dictionary/expansion/probes/batch296_probe.tsv`
- `resources/dictionary/expansion/probes/batch297_probe.tsv`
- `resources/dictionary/expansion/probes/batch298_probe.tsv`
- `resources/dictionary/expansion/probes/batch299_probe.tsv`
- `resources/dictionary/expansion/probes/batch300_probe.tsv`
- `resources/dictionary/expansion/probes/batch301_probe.tsv`
- `resources/dictionary/expansion/round17_exclusions.tsv`
- `resources/dictionary/expansion/round17_validation.json`
- `resources/dictionary/local_core_zh_pinyin.metadata.json`
- `resources/dictionary/local_core_zh_pinyin.tsv`
- `resources/dictionary/sources/lexicon_sources.json`
