本地拼音使用原生 Windows TSF：COM DLL 接收输入事件，通过系统编辑会话更新和提交组合文字；候选使用自绘窗口或宿主管理的 TSF UIElement。

engine 负责离线词库、有限宽度整句解码、简拼/补全/容错与本地学习；settings 负责现代 Win32 设置页；setup 负责系统注册、用户启用与验证。CMake 共享静态库在产品和测试之间复用，无联网模块。
