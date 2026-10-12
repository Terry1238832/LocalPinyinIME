# 当前交付与后续

当前版本0.6.0，设置程序已迁移到Electron、React、TypeScript、Vite。五个页面保持挂载，主题启动快照在首帧前加载，旧Win32设置和截图代码已移除。32项原生测试、12个真实ASAR桌面流程及34张截图检查通过；267句输入结果与0.5.1一致。

下一轮按用户反馈处理界面或输入质量。词库batch_007的1,419条原创词已冻结并验证全部可选，但尚未进入0.6.0。它在resources/dictionary/expansion中保留，SHA为877eb4cd90f8f0fde4b8a371c1e235df9ae9b883285d6d8f77cdf16516477344。需要扩词时再合并、更新来源及元数据、运行回归、另发新版本。已发布版本资产不得覆盖。

三组267句均已公开用作回归，后续不能称盲测；不要把完整验证答案加入词库或训练。完整输入质量与兼容范围在TYPING_EVALUATION.md和COMPATIBILITY.md。node_modules、.build缓存可重建；Git历史、冻结数据与校验值需保留。
