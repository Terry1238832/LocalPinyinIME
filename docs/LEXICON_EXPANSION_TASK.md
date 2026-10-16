# 任务：为“本地拼音”输入法补全词库缺口

仓库：https://github.com/Terry1238832/LocalPinyinIME （默认分支 `main`；下文路径都相对仓库根目录）。

这是一个 Windows 离线中文拼音输入法（C++20 / TSF，设置界面为 Electron）。本次任务**只扩充词库数据**：不要改引擎代码、版本号、安装程序或界面。

## 一、先读这些文件

- `docs/LEXICON_COVERAGE.md`：覆盖率评估，说明缺口在哪里。
- `resources/dictionary/local_core_zh_pinyin.metadata.json`：其中 `source_policy`、`frequency_policy` 两段是必须遵守的来源与权重规则。
- `tools/curate_lexicon.py`、`tools/merge_lexicon_batch.py`：现行的分批补词流程。
- 最近一批的完整样例：`resources/dictionary/expansion/` 下的 `lexicon_batch_134.*`、`batch134_authored_daily_draft.txt`、`batch134_pronunciation_review.tsv`、`batch_134.md`。新批次照这个格式做。

## 二、工作底稿（已生成）

目录：`resources/dictionary/expansion/gaps/`

1. **`missing_characters.tsv`**：《通用规范汉字表》中不能按标准读音单独打出的字，列出了级别、序号、字、标准读音、已有的单字读音。
   - 一级 15 个（如 蚣、蚪、蜻、蝙、豌）
   - 二级 653 个
   - 三级 1,380 个
2. **`missing_common_words.tsv`**：jieba 词频表（MIT 许可）前 30,000 词中，三层词库都没有收录的 7,798 个。
   - 列出了排名、词、jieba 词频、词性、专名类型，以及 pypinyin 给出的默认拼音（多音字需要人工确认）。
   - 其中约 2,900 个是专名：人名 1,735、地名 624、机构 223、其他专名 247、音译人名 100。
3. **`accuracy_baseline_0.8.3.json`**：当前准确率基线，补词后不能下降。

这两份清单只是检查表，不是可以直接导入的数据。每个词都要逐个判断值不值得收、读音对不对。

## 三、要补什么（按优先级）

### A. 单字

1. 补一级缺的 15 个字和二级缺的 653 个字，读音用表中的标准读音。
2. 多读音字给每个标准读音各写一条。
3. 三级字只补其中确实常用的：常见姓氏、地名用字和现代科技用字。其余跳过，并在报告里说明。

### B. 常用词（来自 `missing_common_words.tsv`）

**要收：**
- 普通词语和成语、固定说法；
- 广为人知的现实专名，例如国家机构名、国家与地区名、著名地标、知名历史人物；
- 常见长专名：中华人民共和国、全国人民代表大会、天安门广场、马克思主义。

**不收：**
- jieba 自带的小说人物名，例如武侠小说角色：袁承志、李莫愁；
- 古代官职和生僻历史名词，例如礼部、刑部，除非它们确属常用；
- 繁体或异体写法的重复词，例如“甚么”已有“什么”；
- 分词碎片、不成词的组合。

拿不准的词宁可不收，记进报告的“未收录及原因”。

### C. 自拟词（清单里没有，但覆盖率报告指出的短板）

缺口在三类：时事人名与新词（如新冠、冠状病毒）、港台常用语（如立法会、捷运、新台币、港府）、科技和互联网新词。每类自己按主题写一批常用、无争议的词。

**严禁**从 `tests/data/` 下任何文件取词，包括 heldout_*、typing_* 等测试集。这些是评测答案，取了会让准确率评估失真。`merge_lexicon_batch.py` 也会拒绝测试整句。

## 四、格式与规则

- **行格式**：`拼音<TAB>词<TAB>权重`，UTF-8，LF 换行。
  - 拼音全小写 a–z，不带声调和分隔符。
  - ü 写 v（如 nv、lv），但 üe 写 ue（lue、nue），与现有词条一致。
- **多音字**：凡是含多音字的词，都要在 `batchNNN_pronunciation_review.tsv` 中逐字记录读音选择，格式照批次 134。不能直接相信 pypinyin 的默认读音，例如：银行 yinhang、重庆 chongqing、长沙 changsha、单于 chanyu、厦门 xiamen。
- **暂不收扩展区汉字**：不收 U+20000 及以上的字，即 CJK 扩展 B 区及以后，例如 𠳐、𨱏。这类字在 Windows 里占两个 UTF-16 单元，前文语言模型目前会把它当成两个陌生字打分，会在整句里挤掉常用字（批次 135–147 验收时发现，“不大”被“不𨱏”顶替）。
- **权重**：只用项目现有的粗档。
  - 普通词：6000 / 5500 / 5000，按常用程度选。
  - 新补的生僻单字：3000，不能高于常用字，避免挤到常用候选前面。
- **来源与许可**：
  - 从 jieba 清单挑出的词，在批次 json 中如实标为 reference-derived，来源写 jieba `dict.txt`（MIT），并在 `resources/THIRD_PARTY_NOTICES.txt` 补上 jieba 的 MIT 署名。
  - 单字读音来自 Unicode Unihan `kTGHZ2013`，同样补上 Unicode 数据许可署名。
  - 自拟词标为 first-party authored。
  - 不得整批导入任何第三方词库（搜狗、百度、Rime、公开 GitHub 词库等）。
- **不改已有内容**：已有词条和权重一律不动，只追加新批次。

## 五、流程

1. **编号与规模**：批次号从 135 开始顺延，每批一个主题，不超过约 1,000 条。
2. **2–6 字的词**走 `tools/curate_lexicon.py`：
   1. 写 `lexicon_batch_NNN.authoring.json` 和草稿 txt；
   2. 运行 `prepare`；
   3. 写读音复核 tsv；
   4. 依次运行 `freeze`、`apply`、`evaluate`。

   该工具要求 pypinyin 0.55.0，且只接受 2–6 个汉字的词。
3. **单字和 7 字以上的长专名**：curate 工具不收。另建批次，手写 `lexicon_batch_NNN.tsv`、批次 json 和读音复核 tsv（字段照批次 134），再用下面的命令合并：

   ```
   python tools/merge_lexicon_batch.py --dictionary-dir resources/dictionary --batch <批次tsv> --holdout tests/data/heldout_sentences.tsv
   ```
4. **说明文件**：每批写一份 `batch_NNN.md`，内容包括主题、条数、来源、复核方式。
5. **词条数上限**：`CMakeLists.txt` 里 `test_local_core_lexicon` 的 `--max-entries 170000` 是词条数上限。当前已有 162,428 条，补词后若超出，把上限调到新总数加 10% 左右，并在报告里说明。

## 六、验收（全部通过才算完成）

1. 构建并运行全部测试，36 项都要通过：

   ```
   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
   ```

   只想更快地构建和测试时，可以只做 CMake 配置、构建，再用 ctest 跑测试。
2. **准确率不能下降**。运行下面的命令，与 `gaps/accuracy_baseline_0.8.3.json` 对比：

   ```
   python tools/accuracy_snapshot.py --output after.json
   ```

   - 每个测试集的首选率下降都不能超过 0.2 个百分点。
   - 如果下降，用 `.build/bin/LocalPinyinImeEval.exe .build/bin tests/data/<文件> --scores` 找出被新词挤下去的用例。处理办法是降低那个新词的权重或删掉它，**不能**改测试集。

   当前基线：

   | 测试集 | 首选 | 前九 |
   | --- | ---: | ---: |
   | 逐词输入·参考前文 | 85.95% | 97.98% |
   | 留出整句 | 57.22% | 73.00% |
   | 项目留出 | 81.16% | 86.96% |
   | 新增用例 | 61.19% | 74.63% |
   | 独立用例 | 41.94% | 66.13% |
3. **重跑覆盖率**，更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`，并重新生成工作底稿。需要 Unicode Unihan 数据目录（unicode.org 的 Unihan.zip）和已安装 jieba、pypinyin 的 Python：

   ```
   python tools/report_lexicon_coverage.py --unihan <Unihan目录> --eval-exe .build/bin/LocalPinyinImeEval.exe --resources .build/bin --text tests/data/heldout_sentences.tsv --output docs/lexicon-coverage.json
   python tools/export_lexicon_gaps.py --unihan <Unihan目录>
   ```

   - 原报告里的“Tatoeba 日常句子”和“中文维基新闻”两份文本不在仓库中，这两行不用更新，在文档中注明“未重测”即可。
   - `--text` 的结果只用来测量，**不要**根据它列出的缺词去补词。

## 七、提交方式

- 从 `main` 新建分支 `lexicon/gap-fill`，每个批次一个提交，提交信息格式为 `feat(lexicon): add batch NNN <主题>`。
- 推送这个分支后，向 `main` 开一个 Pull Request，PR 描述就是下面的交付报告。**不要直接推送到 `main`**。
- 不要修改版本号，不要打 tag，不要打包安装程序。这些由维护者在合并后完成。
- `.build/`、`build-*/`、`dist/` 等构建产物已在 `.gitignore` 中，不要提交。

**如果你的环境不是 Windows**，或者无法构建、运行 `LocalPinyinImeEval.exe`：

- 照常完成批次文件，包括草稿、读音复核、批次 tsv/json 和说明。
- `curate_lexicon.py` 的 `freeze`/`apply` 只依赖 Python 和 pypinyin 0.55.0，在任何系统上都能跑；只有 `evaluate` 和第六节的验收需要 Windows。
- 在 PR 描述里写明“原生验证未运行”，并列出跑过和没跑的步骤。维护者会在 Windows 上补跑验收；验收不通过的词会退回调整。

## 八、交付报告

完成后在 PR 描述中给出一份简短报告：

- 新增了哪些批次，每批的主题和条数；
- 单字、常用词前 1 万 / 3 万的覆盖率，补词前后对比；
- 五个测试集的准确率，补词前后对比；
- 未收录的词及原因（可以按类别汇总）；
- 所有改动过的文件清单；
- 验收步骤中哪些已运行、哪些未运行。
