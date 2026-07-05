# LocalPinyinIME 内置自维护词库编辑指南

`resources/dictionary/local_core_zh_pinyin.tsv` 是项目第一方自维护词库，只接受可公开审阅、可分发的词条。不要把个人私有词库、聊天记录、网页抓取结果或第三方输入法词库合入这里。

## 格式

每行一个词条，使用 UTF-8 TSV：

```text
pinyin<TAB>word<TAB>frequency
```

拼音必须小写、无声调、无空格、无数字、无符号。频率是项目维护的相对排序权重，常用档位为 `9000 / 7000 / 5000 / 3000 / 1000`，不是外部语料统计。

## 补丁流程

1. 新建一个补丁 TSV，例如 `my_patch.tsv`。
2. 先 dry-run：

```powershell
python .\tools\merge_local_core_lexicon_patch.py --patch .\my_patch.tsv
```

3. 审核输出中的 added / skipped / conflicts / invalid_rows。
4. 确认后再应用：

```powershell
python .\tools\merge_local_core_lexicon_patch.py --patch .\my_patch.tsv --apply
```

如果要修改已有 `pinyin + word` 的频率，必须显式加 `--allow-update`，避免无意覆盖排序。

## 边界

- 不导入 rime-ice、搜狗、百度、微软、Google 或其他第三方大词库。
- 不联网，不下载数据，不抓取网页。
- 不读取、不写入 `%LOCALAPPDATA%\LocalPinyinIME\user_lexicon.tsv`。
- 不读取、不写入用户学习数据库。
- 不把用户私有词条提交到 Git 或打进 Release。

每次修改后都应运行 local core 词库校验和完整 CTest。
