# 卸载

卸载前请先切换到 Microsoft Pinyin 或其他输入法，避免正在使用 LocalPinyinIME 时注销 DLL。

## 管理员 PowerShell

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
cd <解压目录>\@LOCALPINYIN_PACKAGE_NAME@
```

## 正式卸载流程

推荐使用：

```powershell
.\scripts\Uninstall-LocalPinyinIME.ps1 -DisableCurrentUser
```

流程为：

```text
disable current user
-> verify InputMethodTips contains LocalPinyinIME: FALSE
-> unregister system DLL
-> verify COM root absent
-> verify TSF TIP root absent
```

`--disable-current-user` 的真实含义是 `Disable LocalPinyinIME for current user`。它只禁用当前用户输入法列表中的 LocalPinyinIME，不等于删除 DLL，不等于删除 COM 注册，也不等于删除 TSF 系统注册。

## 删除版本目录

确认注销成功后，可以加：

```powershell
.\scripts\Uninstall-LocalPinyinIME.ps1 -DisableCurrentUser -RemoveFiles
```

删除范围仅限当前 DLL 所在的精确版本目录。脚本不会使用通配符删除父目录，也不会删除其他输入法。

## 卸载后验证

```powershell
.\scripts\Verify-LocalPinyinIME.ps1
```

卸载后建议重启 Windows。
