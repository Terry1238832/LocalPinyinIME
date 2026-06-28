#include "..\dist\LocalPinyinIME-InnoVersion.iss"

#define MyAppName "LocalPinyinIME"
#define MyAppPublisher "LocalPinyinIME contributors"

[Setup]
AppId={{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\LocalPinyinIME\releases\{#MyAppVersion}\x64
DefaultGroupName=LocalPinyinIME
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputBaseFilename=LocalPinyinIMESetup-{#MyAppVersion}-x64
Compression=lzma
SolidCompression=yes

[Files]
Source: "{#MyAppPackageDir}\bin\LocalPinyinIME.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\LocalPinyinImeSetup.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\LocalPinyinImeAudit.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\LocalPinyinSettings.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\dictionary\core_zh_pinyin.tsv"; DestDir: "{app}\dictionary"; Flags: ignoreversion

[Icons]
Name: "{group}\LocalPinyinIME Settings"; Filename: "{app}\LocalPinyinSettings.exe"

[Run]
Filename: "{app}\LocalPinyinImeSetup.exe"; Parameters: "--register-system --dll ""{app}\LocalPinyinIME.dll"""; StatusMsg: "Registering LocalPinyinIME..."

[UninstallRun]
Filename: "{app}\LocalPinyinImeSetup.exe"; Parameters: "--unregister-system --dll ""{app}\LocalPinyinIME.dll"""; RunOnceId: "UnregisterLocalPinyinIME"
