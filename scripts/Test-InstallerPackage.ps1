[CmdletBinding()]
param(
    [string]$InstallerPath = "",
    [string]$StagingRoot = "",
    [string]$DistDir = "dist",
    [switch]$StaticOnly
)

$ErrorActionPreference = "Stop"

$SourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
. (Join-Path $PSScriptRoot "LocalPinyinRelease.ps1")

$ReleaseVersion = Get-LocalPinyinReleaseVersion -SourceRoot $SourceRoot
$PackageName = "LocalPinyinIME-$($ReleaseVersion.Package)-win-x64"
$InstallerBaseName = "LocalPinyinIME-Setup-$($ReleaseVersion.Package)-x64.exe"
$DistRoot = Join-Path $SourceRoot $DistDir
$InnoScript = Join-Path $SourceRoot "installer\LocalPinyinIME.iss"
if ([string]::IsNullOrWhiteSpace($StagingRoot)) {
    $StagingRoot = Join-Path $DistRoot $PackageName
}
if ([string]::IsNullOrWhiteSpace($InstallerPath)) {
    $InstallerPath = Join-Path (Join-Path $DistRoot "installers") $InstallerBaseName
}

function Assert-File {
    param([string]$Path, [string]$Description)
    if (!(Test-Path -LiteralPath $Path)) {
        throw "Missing $Description`: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.Length -le 0) {
        throw "$Description is empty: $Path"
    }
}

function Assert-AbsentPattern {
    param([string]$Pattern, [string]$Description)
    $matches = @(Get-ChildItem -LiteralPath $StagingRoot -Recurse -Force -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -like $Pattern })
    if ($matches.Count -gt 0) {
        $sample = ($matches | Select-Object -First 5 | ForEach-Object { $_.FullName }) -join "; "
        throw "Forbidden $Description found in staging. sample=$sample"
    }
}

function Assert-NoFileName {
    param([string]$Name, [string]$Description)
    $matches = @(Get-ChildItem -LiteralPath $StagingRoot -Recurse -Force -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq $Name })
    if ($matches.Count -gt 0) {
        $sample = ($matches | Select-Object -First 5 | ForEach-Object { $_.FullName }) -join "; "
        throw "Forbidden $Description found in staging. sample=$sample"
    }
}

function Assert-NoUnexpectedPdb {
    $allowedRoot = [IO.Path]::GetFullPath((Join-Path $StagingRoot "evidence\symbols")).TrimEnd('\') + '\'
    $matches = @(Get-ChildItem -LiteralPath $StagingRoot -Recurse -Force -File -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Extension -ieq ".pdb" -and
            !([IO.Path]::GetFullPath($_.FullName).StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase))
        })
    if ($matches.Count -gt 0) {
        $sample = ($matches | Select-Object -First 5 | ForEach-Object { $_.FullName }) -join "; "
        throw "Forbidden PDB outside private evidence symbols found in staging. sample=$sample"
    }
}

function Get-InnoFilesSectionLines {
    param([string]$Path)
    return Get-InnoSectionLines -Path $Path -SectionName "Files"
}

function Get-InnoSectionLines {
    param([string]$Path, [string]$SectionName)
    if (!(Test-Path -LiteralPath $Path)) {
        throw "Inno Setup script is missing: $Path"
    }

    $inSection = $false
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
        $lineNumber++
        $trimmed = ([string]$line).Trim()
        if ($trimmed -match '^\[(.+)\]$') {
            $inSection = ($Matches[1] -ieq $SectionName)
            continue
        }
        if (!$inSection -or [string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith(";")) {
            continue
        }
        [PSCustomObject]@{
            LineNumber = $lineNumber
            Text = [string]$line
        }
    }
}

function Get-InnoDefines {
    param([string]$ScriptText)

    $defines = @{}
    foreach ($match in [regex]::Matches($ScriptText, '(?im)^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+"([^"]*)"\s*$')) {
        $defines[$match.Groups[1].Value] = $match.Groups[2].Value
    }
    return $defines
}

function Resolve-InnoPreprocessorValue {
    param([string]$Value, [hashtable]$Defines)

    $resolved = $Value
    foreach ($name in $Defines.Keys) {
        $resolved = $resolved.Replace("{#$name}", [string]$Defines[$name])
    }
    return $resolved
}

function Split-InnoParameters {
    param([string]$Text)

    $parts = New-Object System.Collections.Generic.List[string]
    $current = New-Object System.Text.StringBuilder
    $inQuote = $false
    for ($i = 0; $i -lt $Text.Length; $i++) {
        $ch = $Text[$i]
        if ($ch -eq '"') {
            $inQuote = !$inQuote
            [void]$current.Append($ch)
            continue
        }
        if ($ch -eq ';' -and !$inQuote) {
            $part = $current.ToString().Trim()
            if ($part.Length -gt 0) {
                $parts.Add($part)
            }
            [void]$current.Clear()
            continue
        }
        [void]$current.Append($ch)
    }
    $last = $current.ToString().Trim()
    if ($last.Length -gt 0) {
        $parts.Add($last)
    }
    return $parts
}

function Convert-InnoDirective {
    param([string]$Text, [hashtable]$Defines)

    $fields = @{}
    foreach ($part in Split-InnoParameters -Text $Text) {
        if ($part -notmatch '^\s*([A-Za-z0-9_]+)\s*:\s*(.*)\s*$') {
            continue
        }
        $key = $Matches[1]
        $value = $Matches[2].Trim()
        if ($value.Length -ge 2 -and $value.StartsWith('"') -and $value.EndsWith('"')) {
            $value = $value.Substring(1, $value.Length - 2)
        }
        $fields[$key] = Resolve-InnoPreprocessorValue -Value $value -Defines $Defines
    }
    return $fields
}

function Assert-InnoIconShortcuts {
    param([string]$Path, [string]$ScriptText)

    $iconLines = @(Get-InnoSectionLines -Path $Path -SectionName "Icons")
    if ($iconLines.Count -eq 0) {
        throw "Inno Setup script has no [Icons] entries: $Path"
    }

    $defines = Get-InnoDefines -ScriptText $ScriptText
    $settingsShortcutName = "{group}\LocalPinyinIME " + [string][char]0x8BBE + [string][char]0x7F6E
    $uninstallShortcutName = "{group}\" + [string][char]0x5378 + [string][char]0x8F7D + " LocalPinyinIME"
    $settingEntries = @()
    $uninstallEntries = @()
    foreach ($entry in $iconLines) {
        $fields = Convert-InnoDirective -Text ([string]$entry.Text) -Defines $defines
        $name = [string]$fields["Name"]
        $filename = [string]$fields["Filename"]
        $workingDir = [string]$fields["WorkingDir"]
        $iconFilename = [string]$fields["IconFilename"]

        foreach ($value in @($name, $filename, $workingDir, $iconFilename)) {
            if ($value -match '(?i)(^|\\)(build|dist)(\\|$)|LocalPinyinIME\.sln|user_lexicon\.tsv|user_learning\.sqlite|\.pdb|test_') {
                throw "Inno [Icons] line $($entry.LineNumber) references forbidden content: $($entry.Text)"
            }
            if ($value -match '(?i)[A-Z]:\\.*LocalPinyinIME') {
                throw "Inno [Icons] line $($entry.LineNumber) uses a hard-coded LocalPinyinIME path instead of an Inno constant: $($entry.Text)"
            }
        }

        if (($name -ieq $settingsShortcutName) -and
            ($filename -ieq "{app}\LocalPinyinSettings.exe")) {
            $settingEntries += [PSCustomObject]@{
                LineNumber = $entry.LineNumber
                WorkingDir = $workingDir
                IconFilename = $iconFilename
                Text = [string]$entry.Text
            }
        }
        if (($name -ieq $uninstallShortcutName) -and
            ($filename -ieq "{uninstallexe}")) {
            $uninstallEntries += [PSCustomObject]@{
                LineNumber = $entry.LineNumber
                IconFilename = $iconFilename
                Text = [string]$entry.Text
            }
        }
    }

    if ($settingEntries.Count -eq 0) {
        throw "Inno script is missing a Start Menu Settings shortcut with Name {group}\LocalPinyinIME <settings> and Filename {app}\LocalPinyinSettings.exe."
    }
    $settingsWithWorkingDir = @($settingEntries | Where-Object { $_.WorkingDir -ieq "{app}" })
    if ($settingsWithWorkingDir.Count -eq 0) {
        $sample = ($settingEntries | Select-Object -First 1).Text
        throw "Inno script Settings shortcut must use WorkingDir {app}. entry=$sample"
    }
    $settingsWithIcon = @($settingEntries | Where-Object { $_.IconFilename -ieq "{app}\LocalPinyinSettings.exe" })
    if ($settingsWithIcon.Count -eq 0) {
        $sample = ($settingEntries | Select-Object -First 1).Text
        throw "Inno script Settings shortcut must use IconFilename {app}\LocalPinyinSettings.exe. entry=$sample"
    }
    if ($uninstallEntries.Count -eq 0) {
        throw "Inno script is missing a Start Menu uninstall shortcut with Name {group}\<uninstall> LocalPinyinIME and Filename {uninstallexe}."
    }
    $uninstallWithIcon = @($uninstallEntries | Where-Object { $_.IconFilename -ieq "{app}\LocalPinyinSettings.exe" })
    if ($uninstallWithIcon.Count -eq 0) {
        $sample = ($uninstallEntries | Select-Object -First 1).Text
        throw "Inno script uninstall shortcut must use IconFilename {app}\LocalPinyinSettings.exe. entry=$sample"
    }

    $iconsText = ($iconLines | ForEach-Object { $_.Text }) -join "`n"
    if ($iconsText -match '(?i)(\{commondesktop\}|\{userdesktop\}|Desktop)') {
        throw "Inno script must not create a desktop shortcut."
    }
}

function Assert-InnoInstallerScript {
    param([string]$Path)

    $scriptText = Get-Content -LiteralPath $Path -Encoding UTF8 -Raw
    $fileLines = @(Get-InnoFilesSectionLines -Path $Path)
    if ($fileLines.Count -eq 0) {
        throw "Inno Setup script has no [Files] entries: $Path"
    }

    foreach ($entry in $fileLines) {
        $text = [string]$entry.Text
        if ($text -notmatch 'Flags\s*:\s*(.+)$') {
            continue
        }
        $flags = $Matches[1]
        $hasIgnoreVersion = ($flags -match '(^|\s)ignoreversion(\s|$)')
        $hasReplaceSameVersion = ($flags -match '(^|\s)replacesameversion(\s|$)')
        if ($hasIgnoreVersion -and $hasReplaceSameVersion) {
            throw "Inno [Files] line $($entry.LineNumber) has mutually exclusive flags ignoreversion and replacesameversion: $text"
        }
    }

    $requiredSources = @(
        "bin\LocalPinyinIME.dll",
        "bin\LocalPinyinImeSetup.exe",
        "bin\LocalPinyinImeAudit.exe",
        "bin\LocalPinyinSettings.exe",
        "bin\dictionary\core_zh_pinyin.tsv",
        "bin\dictionary\local_core_zh_pinyin.tsv",
        "release-manifest.json",
        "SHA256SUMS.txt"
    )
    foreach ($source in $requiredSources) {
        $found = @($fileLines | Where-Object {
            ([string]$_.Text).IndexOf($source, [StringComparison]::OrdinalIgnoreCase) -ge 0
        })
        if ($found.Count -eq 0) {
            throw "Inno [Files] is missing required source: $source"
        }
    }

    $forbiddenPatterns = @(
        "user_lexicon\.tsv",
        "user_learning\.sqlite",
        "\\logs\\",
        "\\build\\",
        "\.zip",
        "\.pdb",
        "\\test_[^\\]*\.exe"
    )
    foreach ($entry in $fileLines) {
        foreach ($pattern in $forbiddenPatterns) {
            if ([string]$entry.Text -match $pattern) {
                throw "Inno [Files] line $($entry.LineNumber) includes forbidden installer content: $($entry.Text)"
            }
        }
    }

    $requiredText = @(
        "PrivilegesRequired=admin",
        "SetupIconFile=..\assets\branding\icon\LocalPinyinIME.ico",
        "UninstallDisplayIcon={app}\LocalPinyinSettings.exe",
        "DisableDirPage=yes",
        "DirExistsWarning=no",
        "UsePreviousAppDir=no",
        "function SetupToolPath(): String",
        "ExpandConstant('{app}\{#MySetupToolName}')",
        "function SetupWorkingDir(): String",
        "ExpandConstant('{app}')",
        "function InstalledDllPath(): String",
        "ExpandConstant('{app}\{#MyAppExeName}')",
        "function SetupDiagnosticLogPath(): String",
        "setup-diagnostics.log",
        "function RegistrationStatusLogPath(): String",
        "LocalPinyinIME\logs\status.log",
        "ParamsWithDiagnostics(Params)",
        "--diagnostic-log",
        "LoadStringFromFile(SetupDiagnosticLogPath(), Contents)",
        "Exec(SetupToolPath(), ExecParams, SetupWorkingDir()",
        "RegisterCode <> 0",
        "TargetVerifyCode <> 0",
        "EnableCode <> 0",
        "VerifyCode <> 0",
        "ShowStepFailure",
        "RegisteredDllMatches(NewDll)",
        "VerifySystemRegistrationAfterRegister(NewDll, RegisterCode, PreviousDll)",
        "--verify --expected-dll",
        "verify target DLL after register-system",
        "--require-current-user-enabled",
        "RunRegisterAndVerify(NewDll, CurrentDll)",
        "RunEnableAndVerify(NewDll)",
        "preserving previous registration until new target verifies"
    )
    foreach ($text in $requiredText) {
        if ($scriptText.IndexOf($text, [StringComparison]::OrdinalIgnoreCase) -lt 0) {
            throw "Inno script is missing required installer safety text: $text"
        }
    }

    if ($scriptText -match '(?i)cmd\.exe') {
        throw "Inno script must not invoke cmd.exe."
    }
    $scriptTextWithoutAllowedDiagnostics = $scriptText.Replace(
        "ExpandConstant('{localappdata}\LocalPinyinIME\logs\status.log')",
        "")
    if ($scriptTextWithoutAllowedDiagnostics -match '(?i)(\{localappdata\}|%LOCALAPPDATA%|LocalAppData|user_lexicon|user_learning|DelTree|RemoveDir)') {
        throw "Inno script appears to touch local user data or delete directories."
    }
    if ($scriptText -match '(?im)^\s*\[Run\]') {
        throw "Inno script should use explicit [Code] setup flow instead of [Run]."
    }
    Assert-InnoIconShortcuts -Path $Path -ScriptText $scriptText

    $idxRegister = $scriptText.IndexOf("RegisterParams := '--register-system --dll '", [StringComparison]::OrdinalIgnoreCase)
    $idxRegisterExec = $scriptText.IndexOf("RunSetupTool(RegisterParams, 'register system', RegisterCode)", [StringComparison]::OrdinalIgnoreCase)
    $idxSystemVerifyAfterRegister = $scriptText.IndexOf("VerifySystemRegistrationAfterRegister(NewDll, RegisterCode, PreviousDll)", [StringComparison]::OrdinalIgnoreCase)
    $idxTargetVerifyAfterRegister = $scriptText.IndexOf("'verify target DLL after register-system'", [StringComparison]::OrdinalIgnoreCase)
    $idxEnable = $scriptText.IndexOf("'--enable-current-user', 'enable current user'", [StringComparison]::OrdinalIgnoreCase)
    $idxVerifyAfterEnable = $scriptText.IndexOf("'verify after enable-current-user'", [StringComparison]::OrdinalIgnoreCase)
    $idxRunRegister = $scriptText.IndexOf("RunRegisterAndVerify(NewDll, CurrentDll)", [StringComparison]::OrdinalIgnoreCase)
    $idxRunEnable = $scriptText.IndexOf("RunEnableAndVerify(NewDll)", [StringComparison]::OrdinalIgnoreCase)
    if ($idxRegister -lt 0 -or $idxSystemVerifyAfterRegister -lt 0 -or
        $idxRegisterExec -lt 0 -or $idxTargetVerifyAfterRegister -lt 0 -or
        $idxEnable -lt 0 -or $idxVerifyAfterEnable -lt 0 -or
        $idxRunRegister -lt 0 -or $idxRunEnable -lt 0) {
        throw "Inno script is missing the expected register/verify/enable/verify sequence."
    }
    if (!($idxRegister -lt $idxRegisterExec -and
          $idxRegisterExec -lt $idxSystemVerifyAfterRegister -and
          $idxEnable -lt $idxVerifyAfterEnable -and
          $idxRunRegister -lt $idxRunEnable)) {
        throw "Inno script register/verify/enable/verify sequence is out of order."
    }
    $curStepMatch = [regex]::Match(
        $scriptText,
        '(?is)procedure\s+CurStepChanged\b.*?(?=\r?\nprocedure\s+|\r?\nfunction\s+|\r?\n\[[A-Za-z]+\]|\z)')
    if (!$curStepMatch.Success) {
        throw "Inno script is missing CurStepChanged."
    }
    if ($curStepMatch.Value -match '(?i)--unregister-system') {
        throw "Inno CurStepChanged must not unregister the previous DLL before the new DLL verifies."
    }
    if ($scriptText -notmatch "(?s)--verify --expected-dll.{0,200}verify target DLL after register-system") {
        throw "Target DLL verification after register-system must use --verify --expected-dll."
    }
    if ($scriptText -notmatch "(?s)--verify --expected-dll.{0,200}--require-current-user-enabled") {
        throw "Final verification after enable-current-user must require current-user enabled state."
    }

    $exitCodeVariables = @("RegisterCode", "TargetVerifyCode", "EnableCode", "VerifyCode")
    foreach ($name in $exitCodeVariables) {
        if ($scriptText.IndexOf($name + ": Integer", [StringComparison]::OrdinalIgnoreCase) -lt 0) {
            throw "Inno script is missing independent exit code variable: $name"
        }
    }
}

if (!(Test-Path -LiteralPath $StagingRoot)) {
    throw "Release staging directory is missing: $StagingRoot"
}

Assert-InnoInstallerScript -Path $InnoScript

Assert-File -Path (Join-Path $StagingRoot "bin\LocalPinyinIME.dll") -Description "LocalPinyinIME.dll"
Assert-File -Path (Join-Path $StagingRoot "bin\LocalPinyinImeSetup.exe") -Description "LocalPinyinImeSetup.exe"
Assert-File -Path (Join-Path $StagingRoot "bin\LocalPinyinImeAudit.exe") -Description "LocalPinyinImeAudit.exe"
Assert-File -Path (Join-Path $StagingRoot "bin\LocalPinyinSettings.exe") -Description "LocalPinyinSettings.exe"
Assert-File -Path (Join-Path $StagingRoot "bin\dictionary\core_zh_pinyin.tsv") -Description "core dictionary"
Assert-File -Path (Join-Path $StagingRoot "bin\dictionary\local_core_zh_pinyin.tsv") -Description "local core dictionary"
Assert-File -Path (Join-Path $StagingRoot "release-manifest.json") -Description "release manifest"
Assert-File -Path (Join-Path $StagingRoot "SHA256SUMS.txt") -Description "release SHA256SUMS"

Assert-NoFileName -Name "user_lexicon.tsv" -Description "user lexicon"
Assert-NoFileName -Name "user_learning.sqlite" -Description "user learning database"
Assert-AbsentPattern -Pattern (Join-Path $StagingRoot "*\logs\*") -Description "logs directory"
Assert-AbsentPattern -Pattern (Join-Path $StagingRoot "*\build\*") -Description "build directory"
Assert-AbsentPattern -Pattern (Join-Path $StagingRoot "*.zip") -Description "dist ZIP"
Assert-NoUnexpectedPdb
Assert-AbsentPattern -Pattern (Join-Path $StagingRoot "*\test_*.exe") -Description "test EXE"

if ($StaticOnly) {
    Write-Host "Inno script: $InnoScript"
    Write-Host "Staging root: $StagingRoot"
    Write-Host "Version: $($ReleaseVersion.Package)"
    Write-Host "Installer package static checks passed."
    return
}

Assert-File -Path $InstallerPath -Description "installer EXE"
if ((Split-Path -Leaf $InstallerPath) -ne $InstallerBaseName) {
    throw "Installer file name mismatch. expected=$InstallerBaseName actual=$(Split-Path -Leaf $InstallerPath)"
}
if ((Split-Path -Leaf $InstallerPath) -notlike "*$($ReleaseVersion.Package)*") {
    throw "Installer file name does not contain version $($ReleaseVersion.Package): $InstallerPath"
}

$hash = Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256
$hashPath = "$InstallerPath.sha256"
if (Test-Path -LiteralPath $hashPath) {
    $hashText = Get-Content -LiteralPath $hashPath -Encoding ASCII -Raw
    if ($hashText -notmatch [regex]::Escape($hash.Hash)) {
        throw "SHA256 sidecar does not contain the installer hash: $hashPath"
    }
}

Write-Host "Installer: $InstallerPath"
Write-Host "Installer bytes: $((Get-Item -LiteralPath $InstallerPath).Length)"
Write-Host "Installer SHA256: $($hash.Hash)"
Write-Host "Staging root: $StagingRoot"
Write-Host "Version: $($ReleaseVersion.Package)"
Write-Host "Installer package checks passed."
