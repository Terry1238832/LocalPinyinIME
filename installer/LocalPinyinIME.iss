#include "..\.build\InstallerVersion.iss"

#define MyAppName "本地拼音"
#define MyAppPublisher "LocalPinyinIME contributors"
#define MyAppExeName "LocalPinyinIME.dll"
#define MySetupToolName "LocalPinyinImeSetup.exe"

[Setup]
AppId={{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\LocalPinyinIME\releases\{#MyAppVersion}\x64
UsePreviousAppDir=no
DisableDirPage=yes
DisableProgramGroupPage=yes
DirExistsWarning=no
DefaultGroupName=本地拼音
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
PrivilegesRequired=admin
CloseApplications=no
RestartApplications=no
OutputDir={#MyAppOutputDir}
OutputBaseFilename={#MyAppSetupBaseFilename}
SetupIconFile=..\assets\branding\icon\LocalPinyinIME.ico
Compression=lzma2/ultra64
SolidCompression=yes
; A short, quiet flow: welcome (install) -> progress -> finished. The style
; follows the Windows light/dark setting like the input method itself.
WizardStyle=modern dynamic windows11 hidebevels
WizardSizePercent=110
WizardImageFile=art\wizard-202.png,art\wizard-269.png,art\wizard-336.png,art\wizard-403.png,art\wizard-498.png
WizardSmallImageFile=art\small-55.png,art\small-69.png,art\small-83.png,art\small-110.png,art\small-138.png
WizardImageFileDynamicDark=art\wizard-202.png,art\wizard-269.png,art\wizard-336.png,art\wizard-403.png,art\wizard-498.png
WizardSmallImageFileDynamicDark=art\small-55.png,art\small-69.png,art\small-83.png,art\small-110.png,art\small-138.png
DisableWelcomePage=no
DisableReadyPage=yes
SetupLogging=yes
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\LocalPinyinSettings.exe

[Files]
Source: "{#MyAppPackageDir}\bin\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyAppPackageDir}\THIRD_PARTY_NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
; localpinyin: links start the settings app. The input method uses them in
; store apps, whose sandbox may not start programs itself; Windows asks once
; which program opens them there, so the app is registered by name for that
; question and for Default apps.
Root: HKLM; Subkey: "Software\Classes\localpinyin"; ValueType: string; ValueName: ""; ValueData: "URL:本地拼音"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\localpinyin"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKLM; Subkey: "Software\Classes\localpinyin\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\LocalPinyinSettings.exe"",0"
Root: HKLM; Subkey: "Software\Classes\localpinyin\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\LocalPinyinSettings.exe"" ""%1"""
Root: HKLM; Subkey: "Software\Classes\LocalPinyinIME.Link"; ValueType: string; ValueName: ""; ValueData: "本地拼音"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\LocalPinyinIME.Link\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\LocalPinyinSettings.exe"",0"
Root: HKLM; Subkey: "Software\Classes\LocalPinyinIME.Link\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\LocalPinyinSettings.exe"" ""%1"""
Root: HKLM; Subkey: "Software\LocalPinyinIME\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "本地拼音设置"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\LocalPinyinIME\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "本地拼音输入法的设置和工具面板"
Root: HKLM; Subkey: "Software\LocalPinyinIME\Capabilities\URLAssociations"; ValueType: string; ValueName: "localpinyin"; ValueData: "LocalPinyinIME.Link"
Root: HKLM; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "LocalPinyinIME"; ValueData: "Software\LocalPinyinIME\Capabilities"; Flags: uninsdeletevalue

[Run]
; The stickers folder, readable by store apps, where the small panel shows them.
Filename: "{app}\LocalPinyinSettingsBridge.exe"; Parameters: "share-stickers"; WorkingDir: "{app}"; Flags: runhidden runasoriginaluser waituntilterminated
Filename: "{app}\LocalPinyinSettings.exe"; Description: "打开本地拼音设置"; WorkingDir: "{app}"; Flags: postinstall nowait skipifsilent runasoriginaluser

[UninstallRun]
; The clipboard recorder leaves and stops starting at sign-in; its saved
; history stays with the rest of the user's data.
Filename: "{app}\LocalPinyinClipboard.exe"; Parameters: "--uninstall"; Flags: runhidden waituntilterminated; RunOnceId: "LocalPinyinClipboardQuit"

[Icons]
Name: "{group}\本地拼音设置"; Filename: "{app}\LocalPinyinSettings.exe"; WorkingDir: "{app}"; IconFilename: "{app}\LocalPinyinSettings.exe"
Name: "{group}\卸载本地拼音"; Filename: "{uninstallexe}"; IconFilename: "{app}\LocalPinyinSettings.exe"

[Messages]
SetupAppTitle=安装本地拼音
SetupWindowTitle=本地拼音安装向导
UninstallAppTitle=卸载本地拼音
UninstallAppFullTitle=卸载 %1
ButtonBack=< 上一步
ButtonNext=下一步 >
ButtonInstall=立即安装
ButtonCancel=取消
ButtonFinish=完成
ButtonOK=确定
ButtonYes=是
ButtonNo=否
WizardLicense=许可协议
LicenseLabel=请阅读许可协议。
LicenseLabel3=继续安装前，请阅读并接受以下 MIT 许可协议。
LicenseAccepted=我接受此协议
LicenseNotAccepted=我不接受此协议
WizardReady=准备安装
ReadyLabel1=即将安装本地拼音。
ReadyLabel2a=点击“安装”开始。
WizardInstalling=正在安装
InstallingLabel=正在安装并注册输入法，通常只需几秒。
FinishedHeadingLabel=安装完成
FinishedLabelNoIcons=本地拼音已安装。
ConfirmUninstall=确定卸载本地拼音吗？个人词库和学习记录将保留。
UninstallStatusLabel=正在卸载，请稍候。

[Code]
const
  LocalPinyinClsid = '{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}';
  GENERIC_WRITE = $40000000;
  OPEN_EXISTING = 3;

function CreateFileW(FileName: String; DesiredAccess, ShareMode, SecurityAttributes,
  CreationDisposition, FlagsAndAttributes, TemplateFile: Cardinal): Longint;
  external 'CreateFileW@kernel32.dll stdcall';
function CloseHandle(Handle: Longint): Boolean;
  external 'CloseHandle@kernel32.dll stdcall';

var
  // Files of an earlier installation of this same version that were in use
  // and moved aside, and the names they were moved to.
  MovedFrom: TArrayOfString;
  MovedTo: TArrayOfString;
  InstallFinished: Boolean;

// Reinstalling the version that is already installed (a repair, or a newer
// build of the same version) replaces files running programs still hold:
// every program with a text box has LocalPinyinIME.dll loaded, and an open
// settings window maps its own files. Windows will not overwrite or delete
// those, but lets them be renamed. So they are moved aside, to be deleted at
// the next restart: running programs keep their copy, programs started
// after this load the new one. Upgrades install into a new version folder
// and never get here.
function FileInUse(Path: String): Boolean;
var
  Handle: Longint;
begin
  Handle := CreateFileW(Path, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
  Result := Handle = -1;
  if not Result then
    CloseHandle(Handle);
end;

function IsMovedAside(Name: String): Boolean;
begin
  Result := (Length(Name) > 4) and (CompareText(Copy(Name, Length(Name) - 3, 4), '.old') = 0);
end;

procedure MoveAsideFilesInUse(Dir: String; Stamp: String);
var
  Found: TFindRec;
  Path, Aside: String;
  Count: Integer;
begin
  if not FindFirst(Dir + '\*', Found) then
    Exit;
  try
    repeat
      Path := Dir + '\' + Found.Name;
      if (Found.Name = '.') or (Found.Name = '..') then
        Continue;
      if (Found.Attributes and FILE_ATTRIBUTE_DIRECTORY) <> 0 then
        MoveAsideFilesInUse(Path, Stamp)
      else if (CompareText(Copy(Found.Name, 1, 5), 'unins') <> 0) and not IsMovedAside(Found.Name) and
              FileInUse(Path) then
      begin
        Aside := Path + '.' + Stamp + '.old';
        if RenameFile(Path, Aside) then
        begin
          Count := GetArrayLength(MovedFrom);
          SetArrayLength(MovedFrom, Count + 1);
          SetArrayLength(MovedTo, Count + 1);
          MovedFrom[Count] := Path;
          MovedTo[Count] := Aside;
          Log('LocalPinyinIME moved a file in use aside: ' + Path + ' -> ' + Aside);
          try
            RestartReplace(Aside, '');
          except
            Log('LocalPinyinIME could not schedule deleting it at restart: ' + GetExceptionMessage);
          end;
        end
        else
          Log('LocalPinyinIME could not move a file in use aside: ' + Path);
      end;
    until not FindNext(Found);
  finally
    FindClose(Found);
  end;
end;

// An installation that did not finish puts the moved files back.
procedure RestoreMovedFiles();
var
  Index: Integer;
begin
  for Index := 0 to GetArrayLength(MovedFrom) - 1 do
    if not FileExists(MovedFrom[Index]) and RenameFile(MovedTo[Index], MovedFrom[Index]) then
      Log('LocalPinyinIME restored a moved file: ' + MovedFrom[Index]);
end;

function QuoteArg(Value: String): String;
begin
  Result := '"' + Value + '"';
end;

function BoolText(Value: Boolean): String;
begin
  if Value then
    Result := 'TRUE'
  else
    Result := 'FALSE';
end;

function SetupToolPath(): String;
begin
  Result := ExpandConstant('{app}\{#MySetupToolName}');
end;

function SetupWorkingDir(): String;
begin
  Result := ExpandConstant('{app}');
end;

function InstalledDllPath(): String;
begin
  Result := ExpandConstant('{app}\{#MyAppExeName}');
end;

function SetupDiagnosticLogPath(): String;
begin
  Result := ExpandConstant('{app}\setup-diagnostics.log');
end;

function RegistrationStatusLogPath(): String;
begin
  Result := ExpandConstant('{localappdata}\LocalPinyinIME\logs\status.log');
end;

function ParamsWithDiagnostics(Params: String): String;
begin
  Result := Params + ' --diagnostic-log ' + QuoteArg(SetupDiagnosticLogPath());
end;

function DiagnosticSummary(): String;
var
  Contents: AnsiString;
begin
  Result := 'Diagnostic log: ' + SetupDiagnosticLogPath() + #13#10 +
            'Registration status log: ' + RegistrationStatusLogPath();
  if LoadStringFromFile(SetupDiagnosticLogPath(), Contents) then
  begin
    if Length(Contents) > 1800 then
      Contents := Copy(Contents, Length(Contents) - 1799, 1800);
    Result := Result + #13#10 + 'Diagnostic summary:' + #13#10 + Contents;
  end;
end;

procedure ShowStepFailure(StepName: String; Params: String; Started: Boolean;
  ResultCode: Integer; Detail: String);
begin
  MsgBox('LocalPinyinIME setup step failed: ' + StepName + #13#10 +
         'Program: ' + SetupToolPath() + #13#10 +
         'Parameters: ' + Params + #13#10 +
         'Working directory: ' + SetupWorkingDir() + #13#10 +
         'Started: ' + BoolText(Started) + #13#10 +
         'Exit code: ' + IntToStr(ResultCode) + #13#10 +
         Detail + #13#10 +
         DiagnosticSummary(), mbError, MB_OK);
end;

function RunSetupTool(Params: String; StepName: String; var ResultCode: Integer): Boolean;
var
  Started: Boolean;
  ExecParams: String;
begin
  ResultCode := -1;
  ExecParams := ParamsWithDiagnostics(Params);
  Log('LocalPinyinIME step: ' + StepName);
  Log('LocalPinyinIME program: ' + SetupToolPath());
  Log('LocalPinyinIME params: ' + Params);
  Log('LocalPinyinIME exec params: ' + ExecParams);
  Log('LocalPinyinIME workdir: ' + SetupWorkingDir());
  Started := Exec(SetupToolPath(), ExecParams, SetupWorkingDir(), SW_HIDE,
                  ewWaitUntilTerminated, ResultCode);
  Log('LocalPinyinIME started: ' + BoolText(Started));
  if Started then
    Log('LocalPinyinIME exit code: ' + IntToStr(ResultCode))
  else
    Log('LocalPinyinIME process did not start');
  Result := Started;
end;

procedure RunBestEffort(Params: String; StepName: String);
var
  ResultCode: Integer;
begin
  if RunSetupTool(Params, StepName, ResultCode) then
    Log('LocalPinyinIME best-effort step finished: ' + StepName +
        ', exit code: ' + IntToStr(ResultCode));
end;

function CurrentRegisteredDll(): String;
var
  Value: String;
begin
  Result := '';
  if RegQueryStringValue(HKCR, 'CLSID\' + LocalPinyinClsid + '\InprocServer32', '', Value) then
    Result := Value;
end;

function RegisteredDllMatches(ExpectedDll: String): Boolean;
var
  ActualDll: String;
begin
  ActualDll := CurrentRegisteredDll();
  Log('LocalPinyinIME current InprocServer32: ' + ActualDll);
  Log('LocalPinyinIME expected InprocServer32: ' + ExpectedDll);
  Result := (ActualDll <> '') and (CompareText(ActualDll, ExpectedDll) = 0);
  Log('LocalPinyinIME InprocServer32 matches installed DLL: ' + BoolText(Result));
end;

procedure TryRestorePreviousRegistration(PreviousDll: String);
var
  ResultCode: Integer;
begin
  if PreviousDll = '' then
    Exit;

  Log('LocalPinyinIME attempting to restore previous registration: ' + PreviousDll);
  if RunSetupTool('--register-system --dll ' + QuoteArg(PreviousDll),
                  'restore previous registration', ResultCode) then
  begin
    Log('LocalPinyinIME restore previous registration exit code: ' + IntToStr(ResultCode));
    if ResultCode = 0 then
      RunBestEffort('--verify', 'verify restored previous registration');
  end;
end;

procedure AbortWithDiagnostics(Params: String; StepName: String; Started: Boolean;
  ResultCode: Integer; Detail: String; PreviousDll: String);
begin
  ShowStepFailure(StepName, Params, Started, ResultCode, Detail);
  TryRestorePreviousRegistration(PreviousDll);
  Abort;
end;

procedure VerifySystemRegistrationAfterRegister(NewDll: String; RegisterCode: Integer;
  PreviousDll: String);
var
  TargetVerifyParams: String;
  TargetVerifyCode: Integer;
begin
  if RegisterCode <> 0 then
    AbortWithDiagnostics('--register-system --dll ' + QuoteArg(NewDll), 'register system',
      True, RegisterCode, 'The setup tool returned a non-zero exit code.', PreviousDll);

  if not RegisteredDllMatches(NewDll) then
    AbortWithDiagnostics('--register-system --dll ' + QuoteArg(NewDll),
      'verify system registration after register-system', True, RegisterCode,
      'InprocServer32 does not point to the installed DLL after register-system.', PreviousDll);

  TargetVerifyParams := '--verify --expected-dll ' + QuoteArg(NewDll);
  if not RunSetupTool(TargetVerifyParams, 'verify target DLL after register-system',
                      TargetVerifyCode) then
    AbortWithDiagnostics(TargetVerifyParams, 'verify target DLL after register-system',
      False, TargetVerifyCode, 'The setup tool process could not be started.', PreviousDll);

  if TargetVerifyCode <> 0 then
    AbortWithDiagnostics(TargetVerifyParams, 'verify target DLL after register-system',
      True, TargetVerifyCode, 'System registration verification failed for the target DLL.', PreviousDll);
end;

procedure RunRegisterAndVerify(NewDll: String; PreviousDll: String);
var
  RegisterParams: String;
  RegisterCode: Integer;
begin
  RegisterParams := '--register-system --dll ' + QuoteArg(NewDll);

  if not RunSetupTool(RegisterParams, 'register system', RegisterCode) then
    AbortWithDiagnostics(RegisterParams, 'register system', False, RegisterCode,
      'The setup tool process could not be started.', PreviousDll);

  VerifySystemRegistrationAfterRegister(NewDll, RegisterCode, PreviousDll);
end;

procedure RunEnableAndVerify(NewDll: String; PreviousDll: String);
var
  EnableCode: Integer;
  VerifyCode: Integer;
  FinalVerifyParams: String;
begin
  if not ExecAsOriginalUser(SetupToolPath(), '--enable-current-user', SetupWorkingDir(), SW_HIDE, ewWaitUntilTerminated, EnableCode) then
    AbortWithDiagnostics('--enable-current-user', 'enable current user', False, EnableCode,
      'The setup tool process could not be started.', PreviousDll);

  if EnableCode <> 0 then
    AbortWithDiagnostics('--enable-current-user', 'enable current user', True, EnableCode,
      'The setup tool did not confirm current user enabled state.', PreviousDll);

  FinalVerifyParams := '--verify --expected-dll ' + QuoteArg(NewDll) + ' --require-current-user-enabled';
  if not ExecAsOriginalUser(SetupToolPath(), FinalVerifyParams, SetupWorkingDir(), SW_HIDE, ewWaitUntilTerminated, VerifyCode) then
    AbortWithDiagnostics(FinalVerifyParams, 'verify after enable-current-user', False, VerifyCode,
      'The setup tool process could not be started.', PreviousDll);

  if VerifyCode <> 0 then
    AbortWithDiagnostics(FinalVerifyParams, 'verify after enable-current-user', True, VerifyCode,
      'Final verification failed after enable-current-user.', PreviousDll);
end;

// The 32-bit input method: 32-bit applications (QQ Music and many other
// desktop programs) load input methods from the 32-bit registry view only.
// It lives in {app}\x86 and reads the dictionary in {app}. Its own 32-bit
// setup tool registers it, since a 64-bit process cannot load a 32-bit DLL.
function SetupToolX86Path(): String;
begin
  Result := ExpandConstant('{app}\x86\{#MySetupToolName}');
end;

function InstalledDllX86Path(): String;
begin
  Result := ExpandConstant('{app}\x86\{#MyAppExeName}');
end;

function RunSetupToolX86(Params: String; StepName: String; var ResultCode: Integer): Boolean;
begin
  ResultCode := -1;
  Log('LocalPinyinIME 32-bit step: ' + StepName);
  Log('LocalPinyinIME 32-bit params: ' + Params);
  Result := Exec(SetupToolX86Path(), Params + ' --diagnostic-log ' + QuoteArg(ExpandConstant('{app}\setup-diagnostics-x86.log')),
                 SetupWorkingDir(), SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Log('LocalPinyinIME 32-bit started: ' + BoolText(Result) + ', exit code: ' + IntToStr(ResultCode));
end;

function CurrentRegisteredDllX86(): String;
var
  Value: String;
begin
  Result := '';
  if RegQueryStringValue(HKCR32, 'CLSID\' + LocalPinyinClsid + '\InprocServer32', '', Value) then
    Result := Value;
end;

// Registered before the 64-bit input method, which then owns the shared
// profile (and its icon path). A failure here leaves 64-bit applications
// working, so it is logged rather than undoing the whole installation.
procedure RunRegisterX86AndVerify();
var
  Code: Integer;
  PreviousX86Dll: String;
begin
  PreviousX86Dll := CurrentRegisteredDllX86();
  if RunSetupToolX86('--register-system --dll ' + QuoteArg(InstalledDllX86Path()), 'register 32-bit', Code) and (Code = 0) and
     RunSetupToolX86('--verify --expected-dll ' + QuoteArg(InstalledDllX86Path()), 'verify 32-bit', Code) and (Code = 0) then
    Exit;
  Log('LocalPinyinIME 32-bit registration failed; 32-bit applications cannot use this version.');
  if (PreviousX86Dll <> '') and (CompareText(PreviousX86Dll, InstalledDllX86Path()) <> 0) and FileExists(PreviousX86Dll) then
    RunSetupToolX86('--register-system --dll ' + QuoteArg(PreviousX86Dll), 'restore previous 32-bit registration', Code);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  CurrentDll: String;
  NewDll: String;
begin
  if (CurStep = ssInstall) and DirExists(ExpandConstant('{app}')) then
    MoveAsideFilesInUse(ExpandConstant('{app}'), GetDateTimeString('yyyymmddhhnnss', #0, #0));
  if CurStep = ssDone then
    InstallFinished := True;
  if CurStep = ssPostInstall then
  begin
    NewDll := InstalledDllPath();
    CurrentDll := CurrentRegisteredDll();
    if (CurrentDll <> '') and (CompareText(CurrentDll, NewDll) <> 0) then
      Log('LocalPinyinIME preserving previous registration until new target verifies: ' + CurrentDll);

    RunRegisterX86AndVerify();
    RunRegisterAndVerify(NewDll, CurrentDll);
    RunEnableAndVerify(NewDll, CurrentDll);
  end;
end;

procedure DeinitializeSetup();
begin
  if not InstallFinished then
    RestoreMovedFiles();
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Code: Integer;
begin
  if (CurUninstallStep = usUninstall) and RegisteredDllMatches(InstalledDllPath()) then
  begin
    RunBestEffort('--disable-current-user', 'disable current user');
    RunBestEffort('--unregister-system --dll ' + QuoteArg(InstalledDllPath()),
                  'unregister system');
  end;
  if (CurUninstallStep = usUninstall) and (CompareText(CurrentRegisteredDllX86(), InstalledDllX86Path()) = 0) then
    RunSetupToolX86('--unregister-system --dll ' + QuoteArg(InstalledDllX86Path()), 'unregister 32-bit', Code);
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  // The welcome page leads straight to installation.
  if CurPageID = wpWelcome then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall);
end;

procedure InitializeWizard();
begin
  WizardForm.WelcomeLabel1.Caption := '本地拼音 {#MyAppVersion}';
  WizardForm.WelcomeLabel2.Caption := '离线的中文拼音输入法。' + #13#10 + #13#10 +
    '根据前文排列候选，越用越懂你的选词习惯。' + #13#10 +
    '输入内容和学习记录只保存在这台电脑。';
  WizardForm.FinishedHeadingLabel.Caption := '安装完成';
  WizardForm.FinishedLabel.Caption := '按 Win + 空格切换到「本地拼音」即可开始输入。' + #13#10 + #13#10 +
    '已经打开的程序，需要重新打开后才能使用新版本。';
end;
