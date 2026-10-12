# 构建与桌面验证

Windows x64需要Visual Studio C++/SDK、CMake/Ninja、Python 3、Node.js 22.12+及Inno Setup 6。运行scripts/Build.ps1；可显式传CMakePath、PythonPath、IsccPath与Jobs。

原生CMake构建输出.build/bin；desktop/package-lock.json锁定桌面依赖。npm ci安装作者依赖；npm run build检查TypeScript、生成Vite资源和CJS主进程/preload。npm run package将应用写入.build/electron-dist；Build.ps1添加原生组件与四份词库、保留许可证、生成Inno安装包与SHA。

开发设置程序：先构建原生bridge，再在desktop执行npm ci、npm run dev。首次下载Electron运行时需要网络。生产exe、原生bridge、词库在同一安装目录，React资源在resources/app.asar内。

desktop/tests/electron-smoke.cjs使用Playwright的真实Electron接口。LOCALPINYINIME_PLAYWRIGHT_MODULE可指定现有Playwright模块；--app与--executable指定应用，--bridge指定开发桥接，--output指定工作区输出，--packaged检验最终exe布局。测试拒绝覆盖非空用户数据，始终使用LOCALPINYINIME_LOCALAPPDATA隔离目录和隐藏offscreen窗口。QA不安装或注册输入法。生产renderer启用sandbox/contextIsolation，关闭Node；窗口关闭后退出。

源码无需保留node_modules、.build和dist缓存。首个完整构建会重新安装依赖与下载官方Electron，应用本身离线运行。
