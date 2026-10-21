# 新手引导图片

设置程序的新手引导（`desktop/src/Tour.tsx`）用到的图片，全部是原生代码的实际渲染，不是示意图：

| 文件 | 内容 |
| --- | --- |
| `cand-<风格>-<light\|dark>.png` | 候选窗口（`ni'hao'shi'jie`，你好世界、你好、拟好……），8 种风格 × 浅色 / 深色，96 DPI、字号档位 4 |
| `mode-<zh\|en>-<light\|dark>.png` | 任务栏上的“中”“英”模式图标，48 px，放在 64 px 的任务栏底色上 |

在 Windows 上重新生成：

```powershell
LocalPinyinCandidateCapture.exe desktop\src\assets\tour --tour
```

`--tour` 用和输入法相同的 Direct2D 候选窗口渲染器（`CandidateWindowCapture`）和任务栏图标渲染（`render_mode_icon`）。当前这一份是从文档里的原生截图 `docs/images/candidate-styles.png` 和 `docs/images/mode-icons.png`（`--icons` 的输出）裁出来的，内容与 `--tour` 的输出相同。
