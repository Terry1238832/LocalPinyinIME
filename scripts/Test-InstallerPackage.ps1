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

function Get-InnoFilesSectionLines {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path)) {
        throw "Inno Setup script is missing: $Path"
    }

    $inFilesSection = $false
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
        $lineNumber++
        $trimmed = ([string]$line).Trim()
        if ($trimmed -match '^\[(.+)\]$') {
            $inFilesSection = ($Matches[1] -ieq "Files")
            continue
        }
        if (!$inFilesSection -or [string]::IsNullOrWhiteSpace($trimmed) -or $trimmed.StartsWith(";")) {
            continue
        }
        [PSCustomObject]@{
            LineNumber = $lineNumber
            Text = [string]$line
        }
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
        "DisableDirPage=yes",
        "DirExistsWarning=no",
        "UsePreviousAppDir=no",
        "function SetupToolPath(): String",
        "ExpandConstant('{app}\{#MySetupToolName}')",
        "function SetupWorkingDir(): String",
        "ExpandConstant('{app}')",
        "function InstalledDllPath(): String",
        "ExpandConstant('{app}\{#MyAppExeName}')",
        "Exec(SetupToolPath(), Params, SetupWorkingDir()",
        "ResultCode <> 0",
        "ShowStepFailure",
        "RegisteredDllMatches(NewDll)",
        "VerifySystemRegistrationAfterRegister(NewDll, RegisterCode, PreviousDll)",
        "Native --verify also queries current-user enabled state",
        "diagnostic verify after register-system",
        "RunRegisterAndVerify(NewDll, CurrentDll)",
        "RunEnableAndVerify()"
    )
    foreach ($text in $requiredText) {
        if ($scriptText.IndexOf($text, [StringComparison]::OrdinalIgnoreCase) -lt 0) {
            throw "Inno script is missing required installer safety text: $text"
        }
    }

    if ($scriptText -match '(?i)cmd\.exe') {
        throw "Inno script must not invoke cmd.exe."
    }
    if ($scriptText -match '(?i)(\{localappdata\}|%LOCALAPPDATA%|LocalAppData|user_lexicon|user_learning|DelTree|RemoveDir)') {
        throw "Inno script appears to touch local user data or delete directories."
    }
    if ($scriptText -match '(?im)^\s*\[Run\]') {
        throw "Inno script should use explicit [Code] setup flow instead of [Run]."
    }

    $idxRegister = $scriptText.IndexOf("RegisterParams := '--register-system --dll '", [StringComparison]::OrdinalIgnoreCase)
    $idxRegisterExec = $scriptText.IndexOf("RunSetupTool(RegisterParams, 'register system', RegisterCode)", [StringComparison]::OrdinalIgnoreCase)
    $idxSystemVerifyAfterRegister = $scriptText.IndexOf("VerifySystemRegistrationAfterRegister(NewDll, RegisterCode, PreviousDll)", [StringComparison]::OrdinalIgnoreCase)
    $idxDiagnosticVerifyAfterRegister = $scriptText.IndexOf("'diagnostic verify after register-system'", [StringComparison]::OrdinalIgnoreCase)
    $idxEnable = $scriptText.IndexOf("'--enable-current-user', 'enable current user'", [StringComparison]::OrdinalIgnoreCase)
    $idxVerifyAfterEnable = $scriptText.IndexOf("'verify after enable-current-user'", [StringComparison]::OrdinalIgnoreCase)
    $idxRunRegister = $scriptText.IndexOf("RunRegisterAndVerify(NewDll, CurrentDll)", [StringComparison]::OrdinalIgnoreCase)
    $idxRunEnable = $scriptText.IndexOf("RunEnableAndVerify()", [StringComparison]::OrdinalIgnoreCase)
    if ($idxRegister -lt 0 -or $idxSystemVerifyAfterRegister -lt 0 -or
        $idxRegisterExec -lt 0 -or $idxDiagnosticVerifyAfterRegister -lt 0 -or
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
    if ($scriptText -match "(?s)'diagnostic verify after register-system'.{0,400}AbortWithDiagnostics") {
        throw "Diagnostic verify after register-system must not abort before enable-current-user."
    }

    $exitCodeVariables = @("RegisterCode", "DiagnosticVerifyCode", "EnableCode", "VerifyCode")
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
Assert-AbsentPattern -Pattern (Join-Path $StagingRoot "*.pdb") -Description "PDB"
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
