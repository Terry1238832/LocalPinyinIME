---
name: product-launch-page
description: 制作具有 Apple 新品发布页质感的产品官网 / 发布页 / Landing Page（极简排版、电影感滚动叙事、产品特写、深浅色、响应式）。当用户要求“做官网”“做产品发布页”“做 landing page”“做 GitHub Pages 主页”“像 Apple 那样的产品页”时使用。它编排本项目已安装的 web-design、frontend-design、web-design-guidelines、deploy-to-vercel 四个 Skill，并规定它们冲突时以哪条为准。
---

# Premium Product Launch Page

一份工作约定：把 Apple iPhone / MacBook / Vision Pro 产品页的**设计原则**（不是它的商标、文案、图片或具体页面）用到我们自己的产品上。

## 用到的 Skill 和分工

| Skill | 位置 | 用在哪一步 |
| --- | --- | --- |
| `web-design` | `.claude/skills/web-design` | 第 2–3 步：产出 `DESIGN.md`；交互档位按 **L3**；查 `references/scroll-story-patterns.md`、`interaction-patterns.md`、`motion-library.md`、`quality-checklist.md`；`references/design-systems/apple.md` 只作原则参考 |
| `frontend-design` | `.claude/skills/frontend-design` | 第 2 步：先写 token 计划并对照 brief 自查“是不是模板默认值”；第 5 步：截图自评、克制、“出门前摘掉一件饰品” |
| `web-design-guidelines` | `.claude/skills/web-design-guidelines` | 第 6 步：按 Vercel Web Interface Guidelines 审计（拉取最新规则，输出 `file:line`） |
| `deploy-to-vercel` | `.claude/skills/deploy-to-vercel` | 第 7 步，**仅在用户明确确认后**；默认 preview，不部署 production |

### 冲突时的裁决（本约定优先）

- `web-design` 要求 L3 页面必须有 WebGL、6 类签名动效、Aurora 一类氛围背景：**不强制**。电影感来自产品本身（真实界面、真实素材的特写和转场），不来自装饰性背景。只有产品本身是 3D 物件时才用 WebGL，全页最多 1 处。
- `web-design` 的“3 个爆点 + 1 个巧思”保留其精神：每 1–2 屏一个由产品驱动的 signature moment，全页 5–8 个，不堆。
- `frontend-design` 的克制原则 > 一切“为了炫”而加的效果。磁吸按钮、自定义光标、Glitch/Shiny 文字、鼠标光斑默认不用。
- `web-design` 说中文页用 Google Fonts 的 Noto Sans SC：若产品强调隐私/离线，或已有品牌字体栈，优先系统字体（PingFang SC / Microsoft YaHei UI / Noto Sans CJK），页面不向第三方发请求。
- 用户在当次请求里写明的要求高于以上全部。

## 视觉方向

- 极简、现代、精致、有科技感：一个主角一屏，超大标题（`clamp()` 流式字号，桌面 64–96px，字重 600，字距 −0.02 到 −0.03em），正文 17–21px，大面积留白，段落短。
- 以**真实**产品截图、渲染图、真实界面为中心。能用产品自己的 CSS/组件现场渲染界面就不用位图（清晰、可主题化、可做动画）。没有素材时先问用户或从仓库找，不用纯色块或 stock 图冒充产品。
- 深色与浅色主题都要做：跟随 `prefers-color-scheme`，并提供手动切换（记住选择，`localStorage` 读写包在 try/catch 里）。深色用真黑 `#000` 做舞台，浅色用白和极浅灰交替分段。
- 不要：廉价渐变（彩虹/紫粉渐变、渐变按钮、渐变文字）、一排排等大的圆角卡片、SaaS 模板（logo 墙、三列 feature 卡、tracked-out 全大写 eyebrow、`A · B · C` 元信息串、每个链接后加 →）、霓虹光晕、玻璃拟态堆叠。
- 结构即信息：序号只给真正有顺序的内容（安装步骤）；分隔线、标签只在承载信息时出现。
- 文案：短句、具体、可验证；数字给出处或测量方法；不夸大（隐私、性能声明必须和代码一致，例外要写出来）。

## 动画与交互

可用的招式（按需选，不全上）：

- **滚动叙事**：一节一个论点，顺序讲完产品故事。
- **Sticky 场景**：外层高度 = N 屏，内层 `position: sticky; top: 0; height: 100svh`，用滚动进度驱动场景内的状态。单页 ≤ 3 处。
- **视差**：只移动 `transform`，幅度小（≤ 10–15% 视口），只在大图/产品特写上。
- **滚动渐显**：IntersectionObserver 触发一次，`opacity + translate/scale`，不用 `filter: blur()` 做动画。
- **电影感转场**：明暗切换、产品从远到近的 scale、遮罩 `clip-path` 揭示。
- **产品缩放展示**：产品界面随滚动从 0.8→1 放大、或局部特写。
- **逐词点亮**：大段宣言文字随滚动逐词变亮（sticky + 进度）。

实现规则：

- 依赖最少：先用原生 CSS + IntersectionObserver + 一个 rAF 节流的滚动循环；确实需要复杂时间线时再用 GSAP + ScrollTrigger（CDN 固定版本）；React 项目可用 Motion。不为一个效果引入一个库。
- 只动 `transform` / `opacity` / `clip-path`；`will-change` 只在动画期间加。
- 离开视口的动画暂停；`requestAnimationFrame` 统一调度，不在 scroll 事件里直接写样式。
- **降级**：`prefers-reduced-motion: reduce` → 关闭滚动驱动和自动播放，直接显示终态，sticky 场景改为普通纵向排列；低性能设备（`navigator.hardwareConcurrency <= 4`、`deviceMemory <= 4`、`saveData`）减少视差与连续动画；无 JS 时内容完整可读。
- 触屏与小屏：sticky 场景在手机上缩短滚动长度或改为静态序列；触摸目标 ≥ 44×44。

## 开发质量底线

- 桌面（≥1280）、平板（768–1024）、手机（360–430）三档实测，无横向滚动；`100svh` 而不是 `100vh`。
- 性能：首屏不依赖大图；图片 `loading="lazy"`、给出宽高、用 WebP/AVIF；字体不阻塞渲染；无布局抖动（CLS ≈ 0）。
- 可访问性：语义标签和标题层级、跳到内容链接、可见的键盘焦点、对比度 ≥ 4.5:1（大字 3:1）、装饰性动画 `aria-hidden`、交互演示可用键盘操作并有文字等价内容。
- 优先复用项目已有技术栈与设计 token（例如本仓库的 `assets/branding/theme/design_tokens.json`），不随意重构项目。

## 工作流程

1. **分析产品**：读 README、文档、发布说明和代码，列出真实功能、目标用户、可用素材（截图、图标、可现场渲染的界面）。不确定的事实不写进页面。
2. **提出结构与视觉方向**：按 `web-design` 写 `DESIGN.md`（放在网站目录里），包含页面结构、每节的核心信息与动画、token、Do/Don't；按 `frontend-design` 自查模板感并修改。
3. **逐节确定内容与动画**：每节一句论点 + 一个证据（数字、演示或界面）+ 一个动画表现。
4. **实现真实网页**：严格按 `DESIGN.md`。
5. **浏览器检查并迭代**：用 Playwright（`/opt/pw-browsers` 已有 Chromium，不要 `playwright install`）截桌面/平板/手机、浅色/深色、reduced-motion 的图，看图改。
6. **构建与质量检查**：HTML/链接检查、控制台零报错、`web-design-guidelines` 审计、`web-design/references/quality-checklist.md` 自检。
7. **部署**：GitHub Pages 用 Actions 工作流（`actions/upload-pages-artifact` + `actions/deploy-pages`）；Vercel 只在用户确认后用 `deploy-to-vercel`。

## 本仓库的现成例子

本地拼音的官网在私有仓库 `Terry1238832/LocalPinyinIME-site`（GitHub Pages），它的 `DESIGN.md` 是规范，可作为下一个页面的起点。官网的配色和截图脚本需要本仓库的检出路径作参数。
