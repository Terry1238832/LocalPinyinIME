[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$Version = "",
    [string]$InstallRoot = "$env:ProgramFiles\LocalPinyinIME",
    [switch]$EnableCurrentUser
)

$ErrorActionPreference = "Stop"

$Clsid = "{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
$ProfileGuid = "{84D58E7C-481E-4D20-A951-4ED39F01D8D5}"
$LanguageTag = "zh-Hans-CN"
$ExpectedTip = "0804:$Clsid$ProfileGuid"
$CanonicalTip = "0x0804:$Clsid$ProfileGuid;"
$PackageRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")
. (Join-Path $PSScriptRoot "LocalPinyinRelease.ps1")
$PackageBin = Join-Path $PackageRoot "bin"
$ManifestPath = Join-Path $PackageRoot "release-manifest.json"
if ([string]::IsNullOrWhiteSpace($Version)) {
    if (!(Test-Path -LiteralPath $ManifestPath)) {
        throw "Version was not supplied and release-manifest.json is missing: $ManifestPath"
    }
    $manifest = Get-Content -LiteralPath $ManifestPath -Encoding UTF8 -Raw | ConvertFrom-Json
    $Version = [string]$manifest.version
    if ([string]::IsNullOrWhiteSpace($Version)) {
        throw "Version was not supplied and release-manifest.json has no version field: $ManifestPath"
    }
}
$SetupSource = Join-Path $PackageBin "LocalPinyinImeSetup.exe"
$DllSource = Join-Path $PackageBin "LocalPinyinIME.dll"
$DictionarySource = Join-Path $PackageBin "dictionary\core_zh_pinyin.tsv"
$LocalCoreDictionarySource = Join-Path $PackageBin "dictionary\local_core_zh_pinyin.tsv"
$VersionDir = Join-Path $InstallRoot "releases\$Version\x64"
$DllTarget = Join-Path $VersionDir "LocalPinyinIME.dll"
$DictionaryTargetDir = Join-Path $VersionDir "dictionary"
$DictionaryTarget = Join-Path $DictionaryTargetDir "core_zh_pinyin.tsv"
$LocalCoreDictionaryTarget = Join-Path $DictionaryTargetDir "local_core_zh_pinyin.tsv"

function Assert-Admin {
    $principal = [Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Administrator PowerShell is required for system registration."
    }
}

function Quote-Argument {
    param([string]$Value)
    return ([string]([char]34)) + ($Value -replace '"', '\"') + ([string]([char]34))
}

function Format-CommandLine {
    param([string]$FileName, [string[]]$Arguments)
    $quoted = @($Arguments | ForEach-Object { Quote-Argument $_ })
    return "$FileName $($quoted -join ' ')"
}

function Invoke-Setup {
    param([string]$SetupPath, [string[]]$Arguments, [string]$Step)

    $command = Format-CommandLine -FileName $SetupPath -Arguments $Arguments
    Write-Host "Step: $Step"
    Write-Host "Command: $command"

    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $SetupPath
    $psi.Arguments = ($Arguments | ForEach-Object { Quote-Argument $_ }) -join ' '
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($psi)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($stdout) { Write-Host $stdout.TrimEnd() }
    if ($stderr) { Write-Error $stderr.TrimEnd() -ErrorAction Continue }
    Write-Host "ExitCode: $($process.ExitCode)"

    if ($process.ExitCode -ne 0) {
        throw "Step failed: $Step. ExitCode=$($process.ExitCode). Review HRESULT and Win32 diagnostics above."
    }
}

function Get-CurrentInprocServer {
    $path = "Registry::HKEY_CLASSES_ROOT\CLSID\$Clsid\InprocServer32"
    if (!(Test-Path -LiteralPath $path)) { return "" }
    $item = Get-Item -LiteralPath $path
    return [string]$item.GetValue("")
}

function Assert-PathUnderRoot {
    param([string]$Path, [string]$Root)
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    $fullPath = [IO.Path]::GetFullPath($Path)
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    if (!$fullPath.StartsWith($fullRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Current registered DLL is outside $Root; stopping to avoid loading an unknown DLL: $fullPath"
    }
}

function Normalize-LocalPinyinTip {
    param([string]$Tip)
    $value = ([string]$Tip).Trim().TrimEnd(';')
    $value = $value -replace '(?i)^0x', ''
    return $value.ToUpperInvariant()
}

function Format-Bool {
    param([bool]$Value)
    if ($Value) { return "TRUE" }
    return "FALSE"
}

function Test-LocalPinyinImeInCurrentUserList {
    Write-Host "Current user language: $LanguageTag"
    Write-Host "Expected TIP: $ExpectedTip"
    if (!(Get-Command Get-WinUserLanguageList -ErrorAction SilentlyContinue)) {
        throw "Get-WinUserLanguageList is unavailable; cannot verify current user InputMethodTips."
    }

    $target = Normalize-LocalPinyinTip $ExpectedTip
    $languages = Get-WinUserLanguageList
    $language = @($languages | Where-Object { $_.LanguageTag -eq $LanguageTag })
    $contains = $false
    foreach ($entry in $language) {
        foreach ($tip in @($entry.InputMethodTips)) {
            if ((Normalize-LocalPinyinTip ([string]$tip)) -eq $target) {
                $contains = $true
                Write-Host "Matched TIP: $tip"
            }
        }
    }
    Write-Host "InputMethodTips contains LocalPinyinIME: $(Format-Bool $contains)"
    return $contains
}

Assert-Admin
if (!(Test-Path -LiteralPath $SetupSource)) { throw "Missing setup tool: $SetupSource" }
if (!(Test-Path -LiteralPath $DllSource)) { throw "Missing DLL: $DllSource" }
$sourceDictionaryStats = Assert-LocalPinyinDictionaryFile -Path $DictionarySource -MinimumEntries 300
$sourceLocalCoreDictionaryStats = Get-LocalPinyinDictionaryStats -Path $LocalCoreDictionarySource
$sourceDictionaryHash = Get-FileHash -LiteralPath $DictionarySource -Algorithm SHA256
$sourceLocalCoreDictionaryHash = Get-FileHash -LiteralPath $LocalCoreDictionarySource -Algorithm SHA256
Write-Host "Package dictionary: $DictionarySource"
Write-Host "Package dictionary SHA256: $($sourceDictionaryHash.Hash)"
Write-Host "Package dictionary sourceRows: $($sourceDictionaryStats.sourceRows)"
Write-Host "Package dictionary duplicateRows: $($sourceDictionaryStats.duplicateRows)"
Write-Host "Package dictionary invalidRows: $($sourceDictionaryStats.invalidRows)"
Write-Host "Package dictionary validEntries: $($sourceDictionaryStats.validEntries)"
Write-Host "Package local core dictionary: $LocalCoreDictionarySource"
Write-Host "Package local core dictionary SHA256: $($sourceLocalCoreDictionaryHash.Hash)"
Write-Host "Package local core dictionary duplicateRows: $($sourceLocalCoreDictionaryStats.duplicateRows)"
Write-Host "Package local core dictionary invalidRows: $($sourceLocalCoreDictionaryStats.invalidRows)"
Write-Host "Package local core dictionary validEntries: $($sourceLocalCoreDictionaryStats.validEntries)"

$currentDll = Get-CurrentInprocServer
if ($currentDll) {
    Write-Host "Current InprocServer32: $currentDll"
    Assert-PathUnderRoot -Path $currentDll -Root $InstallRoot
    if ([IO.Path]::GetFullPath($currentDll).Equals([IO.Path]::GetFullPath($DllTarget), [StringComparison]::OrdinalIgnoreCase)) {
        throw "The same version path is already registered. Refusing to overwrite a DLL that TSF may still have loaded: $DllTarget"
    }
}

if (Test-Path -LiteralPath $DllTarget) {
    throw "Target version DLL already exists. Use a new version directory or uninstall first: $DllTarget"
}

if ($PSCmdlet.ShouldProcess($VersionDir, "copy release files into versioned install directory")) {
    New-Item -ItemType Directory -Force -Path $VersionDir | Out-Null
    New-Item -ItemType Directory -Force -Path $DictionaryTargetDir | Out-Null
    Copy-Item -LiteralPath (Join-Path $PackageBin "LocalPinyinIME.dll") -Destination $VersionDir -Force
    Copy-Item -LiteralPath (Join-Path $PackageBin "LocalPinyinImeSetup.exe") -Destination $VersionDir -Force
    Copy-Item -LiteralPath (Join-Path $PackageBin "LocalPinyinImeAudit.exe") -Destination $VersionDir -Force
    Copy-Item -LiteralPath (Join-Path $PackageBin "LocalPinyinSettings.exe") -Destination $VersionDir -Force
    Copy-Item -LiteralPath $DictionarySource -Destination $DictionaryTarget -Force
    Copy-Item -LiteralPath $LocalCoreDictionarySource -Destination $LocalCoreDictionaryTarget -Force
}

if (!(Test-Path -LiteralPath $DllTarget)) {
    throw "Post-copy verification failed: DLL is missing from version directory: $DllTarget"
}
if (!(Test-Path -LiteralPath $DictionaryTarget)) {
    throw "Post-copy verification failed: dictionary is missing from version directory: $DictionaryTarget"
}
if (!(Test-Path -LiteralPath $LocalCoreDictionaryTarget)) {
    throw "Post-copy verification failed: local core dictionary is missing from version directory: $LocalCoreDictionaryTarget"
}
$targetDictionaryStats = Assert-LocalPinyinDictionaryFile -Path $DictionaryTarget -MinimumEntries 300
$targetLocalCoreDictionaryStats = Get-LocalPinyinDictionaryStats -Path $LocalCoreDictionaryTarget
$targetDictionaryHash = Get-FileHash -LiteralPath $DictionaryTarget -Algorithm SHA256
$targetLocalCoreDictionaryHash = Get-FileHash -LiteralPath $LocalCoreDictionaryTarget -Algorithm SHA256
if ($targetDictionaryHash.Hash -ne $sourceDictionaryHash.Hash) {
    throw "Post-copy verification failed: dictionary SHA256 mismatch. source=$($sourceDictionaryHash.Hash) target=$($targetDictionaryHash.Hash)"
}
if ($targetLocalCoreDictionaryHash.Hash -ne $sourceLocalCoreDictionaryHash.Hash) {
    throw "Post-copy verification failed: local core dictionary SHA256 mismatch. source=$($sourceLocalCoreDictionaryHash.Hash) target=$($targetLocalCoreDictionaryHash.Hash)"
}
Write-Host "Installed dictionary target: $DictionaryTarget"
Write-Host "Installed dictionary SHA256: $($targetDictionaryHash.Hash)"
Write-Host "Installed dictionary sourceRows: $($targetDictionaryStats.sourceRows)"
Write-Host "Installed dictionary duplicateRows: $($targetDictionaryStats.duplicateRows)"
Write-Host "Installed dictionary invalidRows: $($targetDictionaryStats.invalidRows)"
Write-Host "Installed dictionary validEntries: $($targetDictionaryStats.validEntries)"
Write-Host "Installed local core dictionary target: $LocalCoreDictionaryTarget"
Write-Host "Installed local core dictionary SHA256: $($targetLocalCoreDictionaryHash.Hash)"
Write-Host "Installed local core dictionary duplicateRows: $($targetLocalCoreDictionaryStats.duplicateRows)"
Write-Host "Installed local core dictionary invalidRows: $($targetLocalCoreDictionaryStats.invalidRows)"
Write-Host "Installed local core dictionary validEntries: $($targetLocalCoreDictionaryStats.validEntries)"

$SetupTarget = Join-Path $VersionDir "LocalPinyinImeSetup.exe"

if ($currentDll) {
    if ($PSCmdlet.ShouldProcess($currentDll, "unregister old DLL")) {
        Invoke-Setup -SetupPath $SetupTarget -Arguments @("--unregister-system", "--dll", $currentDll) -Step "unregister old DLL"
    }
}

try {
    if ($PSCmdlet.ShouldProcess($DllTarget, "register new DLL")) {
        Invoke-Setup -SetupPath $SetupTarget -Arguments @("--register-system", "--dll", $DllTarget) -Step "register new DLL"
    }
    Invoke-Setup -SetupPath $SetupTarget -Arguments @("--verify") -Step "verify system registration"
} catch {
    if ($currentDll) {
        Write-Warning "New registration failed. Attempting to restore old registration: $currentDll"
        try {
            Invoke-Setup -SetupPath $SetupTarget -Arguments @("--register-system", "--dll", $currentDll) -Step "restore old DLL"
        } catch {
            Write-Warning "Old registration restore failed: $($_.Exception.Message)"
        }
    }
    throw
}

if ($EnableCurrentUser) {
    if ($PSCmdlet.ShouldProcess("current user", "enable LocalPinyinIME without changing the default input method")) {
        Invoke-Setup -SetupPath $SetupTarget -Arguments @("--enable-current-user") -Step "enable current user"
        Invoke-Setup -SetupPath $SetupTarget -Arguments @("--verify") -Step "verify TSF enabled state"
        if (!(Test-LocalPinyinImeInCurrentUserList)) {
            throw "Current user enable verification failed: InputMethodTips does not contain LocalPinyinIME."
        }
    }
}

Write-Host "Install flow finished. Run Verify-LocalPinyinIME.ps1 for read-only verification, then use Win+Space manually."
