# 任务：补全“本地拼音”的零碎日常词（第二轮补词）

仓库：https://github.com/Terry1238832/LocalPinyinIME （默认分支 `main`；下文路径都相对仓库根目录）。

这是一个 Windows 离线中文拼音输入法（C++20 / TSF，设置界面为 Electron）。本次任务**只扩充词库数据**，包括主词库和“常用场景”词表。不要改引擎代码、版本号、安装程序或界面。

第一轮（批次 135–147，0.8.4）补的是规范汉字和 jieba 前 3 万词的缺口。本轮补的是**每天都会打、却打不出来的零碎词**。

**开始前先确认版本**：`desktop/package.json` 的版本应为 0.15.0 或更高，并且存在 `resources/dictionary/domains/` 和 `resources/dictionary/expansion/gaps/accuracy_baseline_0.15.0.json`。不满足说明拿到的是旧代码，停下来告诉维护者，不要在旧代码上做。

## 一、问题是什么

用户开着“数学理科”场景打 shizi，九个候选里没有“式子”。它不在任何一层词库里，也不在数学场景词表里。

维护者随手列了 158 个日常词做抽查，19 个汉字词不在任何一层词库里（约 12%）。用 0.15.0 单独输入这些词（不带前文、不选场景）：

- **7 个根本不在候选里**：式子、算式、竖式、演草纸、甩锅、背锅、氪金。其中氪金只有选了“游戏娱乐”才出现。
- **其余 12 个靠逐字组合才排进前九**，位置不稳：内卷、躺平第 1，根号第 4，三角板第 5，锅子第 7。

这 19 个词在 `resources/dictionary/expansion/probes/spot_check_0.15.0.tsv`，可以直接用第七节第 3 步的命令重测。

还有一些词只在场景词表里，不选那个场景就打不出来或排得靠后：“氪金”只在游戏场景，“根号”只在数学场景（不选时排第 4）。

缺口的来源：

- 标准词库（Rime pinyin-simp）偏小；
- 第一轮只查到 jieba 前 3 万词；
- jieba 词表本身较旧，没有近十年的新词。

## 二、先读这些文件

- `resources/dictionary/local_core_zh_pinyin.metadata.json`：其中 `source_policy`、`frequency_policy` 两段是必须遵守的来源与权重规则。
- `tools/curate_lexicon.py`、`tools/merge_lexicon_batch.py`：分批补词的流程。
- 最近一批的完整样例：`resources/dictionary/expansion/` 下的 `lexicon_batch_147.*`、`batch147_authored_daily_draft.txt`、`batch147_pronunciation_review.tsv`、`batch_147.md`。新批次照这个格式做。
- `docs/INPUT_FEATURES.md` 的“常用场景”一节、`tools/build_domain_packs.py`、`resources/dictionary/authoring/domains/*.txt`：场景词表的规则和来源。
- `docs/LEXICON_COVERAGE.md`：第一轮后的覆盖率。

## 三、三层词库与场景词表

| 文件 | 内容 | 本轮是否改动 |
| --- | --- | --- |
| `standard_zh_pinyin.tsv` | 第三方标准词库（Apache-2.0） | 不改 |
| `core_zh_pinyin.tsv` | 项目核心常用词 | 不改 |
| `local_core_zh_pinyin.tsv` | 项目自维护词库，现有 164,330 条，按批次追加 | **追加新批次** |
| `authoring/domains/<场景>.txt` → `domains/<场景>.tsv` | 六个“常用场景”词表，选中场景时才生效 | **可以追加** |

**放哪一层？**

- 通用的日常词进主词库（`local_core`），不选任何场景也要能打出来。
  - 例如：式子、算式、根号、三角板、躺平、刷怪。
- 场景词表只放“选了这个场景才值得排到前面”的词。
- 一个词可以既在主词库、又在场景词表里：主词库保证能打出来，场景词表负责在该场景里把它排到前面。

## 四、要补什么（按优先级）

### A. 按生活主题自拟（first-party authored，最重要）

每个主题一批，一批 300–1,000 条。只收常用、无争议、现在大家确实在打的词：

1. **学校与数学**：小学到高中的算术与几何说法、课堂用语、作业与考试、文具。
   - 例如：式子、算式、竖式、横式、加数、被减数、余数、约分、通分、三角板、量角器、演草纸、错题本、口算、应用题。
2. **家居与日用品**：家具、电器、厨房、清洁、收纳、五金。
3. **饮食**：家常菜、小吃、饮品、调料、烹饪动作。
4. **身体与健康**：部位、常见症状、科室、常用药与器具。
5. **购物与生活服务**：网购、外卖、快递、支付、退换货、物业、维修。
6. **出行与天气**：交通方式、站点与票务说法、导航、天气与节气用语。
7. **职场口语**：例如甩锅、背锅、复盘、对齐、拉通、摸鱼、画饼。
8. **网络用语与口语**：近十年广泛使用、已经稳定下来的词，例如内卷、躺平、社死、破防、上头、种草、拔草。
   - 不收低俗词、攻击性词、只流行几周的梗。
9. **游戏与娱乐**：例如刷怪、坑队友、开黑、上分、抽卡、副本、追剧、弹幕。
10. **零碎常用结构**：
    - “X子”名词：锅子、式子、样子……
    - 常用的动宾短语和补语短语：打草稿、对答案、拉肚子、睡过头……
    - 量词搭配、拟声词、口语里的固定说法。

### B. 场景词表里有、主词库没有的 101 个词

这些词只有选中对应场景才能打出。逐个判断：

- 通用的（例如根号、正弦、切线、实数、开黑、复盘、追剧、取快递）补进主词库；
- 只对该场景有意义的保持原样，并在报告里列出理由。

| 场景 | 主词库没有的词 |
| --- | --- |
| 编程开发 | 浮点数 多态 迭代器 回调函数 协程 内存溢出 时间复杂度 空间复杂度 哈希 二叉树 跨域 反序列化 正则表达式 代码风格 泛型 越界 死循环 依赖注入 单例 复现 |
| 日常聊天 | 不用了 咋整 忙啥 吃啥 取快递 追剧 饿死了 卡了 |
| 游戏娱乐 | 开黑 上分 掉分 段位 抽卡 氪金 读档 下路 团战 英雄联盟 和平精英 星穹铁道 绝地求生 塞尔达 宝可梦 番剧 追番 二次元 手办 |
| 数学理科 | 行列式 特征向量 引理 三角函数 正弦 余弦 正切 幂函数 反函数 复合函数 并集 补集 单调性 奇函数 偶函数 双曲线 圆锥曲线 全等 勾股定理 圆周率 根号 实数 虚数 原函数 不定积分 微分方程 中值定理 间断点 切线 法线 对称轴 判别式 韦达定理 二项式 数学归纳法 反证法 充分条件 充要条件 逆命题 数量积 夹角 空间向量 解析几何 化学方程式 整道题 |
| 学习考试 | 模拟考 考公 直播课 |
| 办公职场 | 年终总结 复盘 远程办公 外勤 转化率 复购 |

（“整道题”“饿死了”“卡了”这类短语未必适合进主词库，按实际判断。）

### C. jieba 3–10 万名之间的缺词（reference-derived，作检查表）

用 `tools/export_lexicon_gaps.py --top 100000` 重新生成缺词清单，只看排名 30,001–100,000 的部分。

- 生成需要 Unicode Unihan 数据目录（unicode.org 的 Unihan.zip）和装有 jieba、pypinyin 的 Python。
- 输出写到 `resources/dictionary/expansion/gaps/`，覆盖旧的 0.8.3 清单。
- 取舍规则：
  - **收**：普通词语、成语、固定说法；
  - **不收**：小说人物名、生僻古官职、繁体或异体重复、分词碎片。
- 这部分词在批次 json 里如实标为 reference-derived，来源写 jieba `dict.txt`（MIT）。

### D. 扩充场景词表

- 给 `authoring/domains/math.txt` 补上中小学算术和课堂说法，例如：式子、算式、竖式、横式、口算、加法、减法、乘法、除法、余数、约分、通分、应用题、解方程、列式。
- 其他场景按同样思路补充本场景高频、而日常不常用的词。
- 遵守场景词表的现有规则：
  - 不收与极常用词同音、对场景又不关键的词（如“同事/同时”“离子/例子”）；
  - 不收与常用字同音的单字语气词。
- 改完运行 `python tools/build_domain_packs.py`，并核对它列出的新读音。

**严禁**从 `tests/data/` 下任何文件取词，包括 heldout_*、typing_* 等测试集。这些是评测答案，取了会让准确率评估失真。`merge_lexicon_batch.py` 也会拒绝测试整句。

**拿不准的词宁可不收**，记进报告的“未收录及原因”。

## 五、格式与规则

- **行格式**：`拼音<TAB>词<TAB>权重`，UTF-8，LF 换行。
  - 拼音全小写 a–z，不带声调和分隔符。
  - ü 写 v（如 nv、lv），但 üe 写 ue（lue、nue），与现有词条一致。
- **多音字**：凡是含多音字的词，都要在 `batchNNN_pronunciation_review.tsv` 中逐字记录读音选择，格式照批次 147。
  - 不能直接相信 pypinyin 的默认读音，只看不带声调的拼写是否正确。例如：调休 tiaoxiu（不是 diaoxiu）、差评 chaping、薄荷 bohe、量角器 liangjiaoqi、角色 juese、重庆 chongqing。
  - 轻声按本字读音写：式子 shizi、锅子 guozi。
- **权重**：只用项目现有的粗档。
  - 普通词：6000 / 5500 / 5000，按常用程度选。
  - 不得高于同音的更常用词，避免挤掉它们。
- **暂不收扩展区汉字**：不收 U+20000 及以上的字，即 CJK 扩展 B 区及以后。
- **来源与许可**：
  - 自拟词标为 first-party authored；
  - jieba 来的词标为 reference-derived；
  - 不得整批导入任何第三方词库（搜狗、百度、Rime、公开 GitHub 词库、维基词典等）。
- **不改已有内容**：已有词条和权重一律不动，只追加新批次。

## 六、流程

1. **编号与规模**：批次号从 148 开始顺延，每批一个主题，不超过约 1,000 条。
2. **2–6 字的词**走 `tools/curate_lexicon.py`：
   1. 写 `lexicon_batch_NNN.authoring.json` 和草稿 txt；
   2. 运行 `prepare`；
   3. 写读音复核 tsv；
   4. 依次运行 `freeze`、`apply`、`evaluate`。

   该工具要求 pypinyin 0.55.0。
3. **7 字以上的长词**：curate 工具不收。另建批次，手写 `lexicon_batch_NNN.tsv`、批次 json 和读音复核 tsv（字段照批次 147），再用下面的命令合并：

   ```
   python tools/merge_lexicon_batch.py --dictionary-dir resources/dictionary --batch <批次tsv> --holdout tests/data/heldout_sentences.tsv
   ```
4. **说明文件**：每批写一份 `batch_NNN.md`，内容包括主题、条数、来源、复核方式。
5. **可打出清单**：每批另写 `resources/dictionary/expansion/probes/batchNNN_probe.tsv`，列出这一批的全部新词。
   - 格式为 `拼音<TAB>词<TAB>来源标记`，三列都要有，第三列写批次号即可，例如 `shizi\t式子\tbatch148`。
   - 用它检查新词能否真的打出来，见第七节第 3 步。
6. **词条数上限**：`CMakeLists.txt` 里 `test_local_core_lexicon` 的 `--max-entries 170000` 是上限，现有 164,330 条，本轮一定会超出。补完后把它调到新总数加 10% 左右，并在报告里说明。

## 七、验收（全部通过才算完成）

1. **构建并运行全部测试**，38 项都要通过：

   ```
   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build.ps1
   ```

   只想更快地构建和测试时，可以只做 CMake 配置、构建，再用 ctest 跑测试。
2. **准确率不能下降**。运行下面的命令，与 `resources/dictionary/expansion/gaps/accuracy_baseline_0.15.0.json` 对比：

   ```
   python tools/accuracy_snapshot.py --output after.json
   ```

   - 每个测试集的首选率下降都不能超过 0.2 个百分点。
   - 如果下降，用 `.build/bin/LocalPinyinImeEval.exe .build/bin tests/data/<文件> --scores` 找出被新词挤下去的用例。处理办法是降低那个新词的权重或删掉它，**不能**改测试集。

   当前基线（0.15.0）：

   | 测试集 | 用例 | 首选 | 前九 |
   | --- | ---: | ---: | ---: |
   | 逐词输入·参考前文 | 20,626 | 85.95% | 98.04% |
   | 逐词输入·无前文 | 20,626 | 77.96% | 97.13% |
   | 留出整句 | 4,126 | 57.71% | 73.10% |
   | 口语对话·整句 | 3,368 | 80.31% | 91.75% |
   | 口语对话·逐词 | 19,504 | 94.10% | 99.49% |
   | 项目留出 | 138 | 79.71% | 86.23% |
   | 新增用例 | 67 | 62.69% | 77.61% |
   | 独立用例 | 62 | 43.55% | 62.90% |
3. **新词要能打出来**。这是本轮的目标：词进了词库却排不进前九，用户照样打不出来。对每批的可打出清单运行：

   ```
   .build/bin/LocalPinyinImeEval.exe .build/bin resources/dictionary/expansion/probes/batchNNN_probe.tsv --no-context --scores
   ```

   - 每个词输出一行 JSON，`expected_rank` 是它在候选中的位置，0 表示不在候选里。
   - 2–4 字的新词，单独输入时都应在前九。
   - 不在前九的，在允许的权重档内调高，或在报告里说明原因（例如同音的极常用词太多）。
   - 每批报告首选率和前九率。
4. **场景不能变差**。改过场景词表的，对六个场景各运行一次：

   ```
   .build/bin/LocalPinyinImeEval.exe .build/bin tests/data/heldout_context.tsv --domains=<场景>
   .build/bin/LocalPinyinImeEval.exe .build/bin tests/data/heldout_sentences.tsv --domains=<场景>
   ```

   与 `docs/INPUT_FEATURES.md` 中“选中的场景”表对比，下降不超过 0.2 个百分点，并用新结果更新那张表。

   用场景的可打出清单确认：选中场景后，场景新词都在前三。
5. **重跑覆盖率**，更新 `docs/LEXICON_COVERAGE.md` 和 `docs/lexicon-coverage.json`：

   ```
   python tools/report_lexicon_coverage.py --unihan <Unihan目录> --eval-exe .build/bin/LocalPinyinImeEval.exe --resources .build/bin --text tests/data/heldout_sentences.tsv --output docs/lexicon-coverage.json
   ```

   - 不在仓库中的文本（Tatoeba 日常句子、中文维基新闻）注明“未重测”即可。
   - `--text` 的结果只用来测量，**不要**根据它列出的缺词去补词。

## 八、提交方式

- 从最新的 `main` 新建分支 `lexicon/gap-fill-2`，每个批次一个提交，提交信息格式为 `feat(lexicon): add batch NNN <主题>`。
  - 场景词表的改动单独一个提交：`feat(lexicon): extend usage-scene word lists`。
- 推送这个分支后，向 `main` 开一个 Pull Request，PR 描述就是下面的交付报告。**不要直接推送到 `main`**。
- 不要修改版本号，不要打 tag，不要打包安装程序。这些由维护者在合并后完成。
- `.build/`、`build-*/`、`dist/` 等构建产物已在 `.gitignore` 中，不要提交。

**如果你的环境不是 Windows**，或者无法构建、运行 `LocalPinyinImeEval.exe`：

- 照常完成批次文件，包括草稿、读音复核、批次 tsv/json、说明和可打出清单。
- `curate_lexicon.py` 的 `freeze`/`apply` 和 `build_domain_packs.py` 只依赖 Python 和 pypinyin 0.55.0，在任何系统上都能跑；第七节的验收需要 Windows。
- 在 PR 描述里写明“原生验证未运行”，并列出跑过和没跑的步骤。维护者会在 Windows 上补跑验收；验收不通过的词会退回调整。

## 九、交付报告

完成后在 PR 描述中给出一份简短报告：

- 新增了哪些批次，每批的主题和条数；
- 第一节的 19 个抽查词补词后单独输入排第几（不选场景和选中对应场景各测一次）；
- 每批可打出清单的首选率和前九率；
- `accuracy_snapshot.py` 各组的准确率，补词前后对比；
- 场景词表的改动，以及六个场景的通用测试集结果；
- B 节 101 个场景词的处理结果（补进主词库的、保持只在场景里的及理由）；
- 未收录的词及原因（可以按类别汇总）；
- 所有改动过的文件清单；
- 验收步骤中哪些已运行、哪些未运行。
