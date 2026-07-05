#include "..\dist\LocalPinyinIME-InnoVersion.iss"

#define MyAppName "LocalPinyinIME"
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
DirExistsWarning=no
DefaultGroupName=LocalPinyinIME
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
OutputDir={#MyAppOutputDir}
OutputBaseFilename={#MyAppSetupBaseFilename}
SetupIconFile=..\assets\branding\icon\LocalPinyinIME.ico
Compression=lzma
SolidCompression=yes
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\LocalPinyinSettings.exe

[Files]
Source: "{#MyAppPackageDir}\bin\LocalPinyinIME.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\LocalPinyinImeSetup.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\LocalPinyinImeAudit.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\LocalPinyinSettings.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\dictionary\core_zh_pinyin.tsv"; DestDir: "{app}\dictionary"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\bin\dictionary\local_core_zh_pinyin.tsv"; DestDir: "{app}\dictionary"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\release-manifest.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\SHA256SUMS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPackageDir}\docs\*.md"; DestDir: "{app}\docs"; Flags: ignoreversion

[Icons]
Name: "{group}\LocalPinyinIME 设置"; Filename: "{app}\LocalPinyinSettings.exe"; WorkingDir: "{app}"; IconFilename: "{app}\LocalPinyinSettings.exe"
Name: "{group}\卸载 LocalPinyinIME"; Filename: "{uninstallexe}"; IconFilename: "{app}\LocalPinyinSettings.exe"

[Code]
const
  LocalPinyinClsid = '{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}';

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

procedure RunEnableAndVerify(NewDll: String);
var
  EnableCode: Integer;
  VerifyCode: Integer;
  FinalVerifyParams: String;
begin
  if not RunSetupTool('--enable-current-user', 'enable current user', EnableCode) then
    AbortWithDiagnostics('--enable-current-user', 'enable current user', False, EnableCode,
      'The setup tool process could not be started.', '');

  if EnableCode <> 0 then
    AbortWithDiagnostics('--enable-current-user', 'enable current user', True, EnableCode,
      'The setup tool did not confirm current user enabled state.', '');

  FinalVerifyParams := '--verify --expected-dll ' + QuoteArg(NewDll) + ' --require-current-user-enabled';
  if not RunSetupTool(FinalVerifyParams, 'verify after enable-current-user', VerifyCode) then
    AbortWithDiagnostics(FinalVerifyParams, 'verify after enable-current-user', False, VerifyCode,
      'The setup tool process could not be started.', '');

  if VerifyCode <> 0 then
    AbortWithDiagnostics(FinalVerifyParams, 'verify after enable-current-user', True, VerifyCode,
      'Final verification failed after enable-current-user.', '');
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  CurrentDll: String;
  NewDll: String;
begin
  if CurStep = ssPostInstall then
  begin
    NewDll := InstalledDllPath();
    CurrentDll := CurrentRegisteredDll();
    if (CurrentDll <> '') and (CompareText(CurrentDll, NewDll) <> 0) then
      Log('LocalPinyinIME preserving previous registration until new target verifies: ' + CurrentDll);

    RunRegisterAndVerify(NewDll, CurrentDll);
    RunEnableAndVerify(NewDll);
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    RunBestEffort('--disable-current-user', 'disable current user');
    RunBestEffort('--unregister-system --dll ' + QuoteArg(InstalledDllPath()),
                  'unregister system');
  end;
end;
