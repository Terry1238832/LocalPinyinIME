[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string]$InstallRoot = "$env:ProgramFiles\LocalPinyinIME",
    [switch]$DisableCurrentUser,
    [switch]$RemoveFiles
)

$ErrorActionPreference = "Stop"

$Clsid = "{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
$ProfileGuid = "{84D58E7C-481E-4D20-A951-4ED39F01D8D5}"
$LanguageTag = "zh-Hans-CN"
$ExpectedTip = "0804:$Clsid$ProfileGuid"

function Assert-Admin {
    $principal = [Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Administrator PowerShell is required for system unregister. Switch to another IME before uninstalling."
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
        throw "Step failed: $Step. ExitCode=$($process.ExitCode). Keep the install directory for diagnostics."
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

$currentDll = Get-CurrentInprocServer
if (!$currentDll) {
    Write-Host "No LocalPinyinIME COM registration was found. No unregister action was executed."
    return
}

Assert-PathUnderRoot -Path $currentDll -Root $InstallRoot
$VersionDir = Split-Path -Parent $currentDll
$SetupPath = Join-Path $VersionDir "LocalPinyinImeSetup.exe"
if (!(Test-Path -LiteralPath $SetupPath)) {
    throw "Setup tool is missing next to the registered DLL: $SetupPath"
}

if ($DisableCurrentUser) {
    if ($PSCmdlet.ShouldProcess("current user", "disable LocalPinyinIME without changing the default input method")) {
        Invoke-Setup -SetupPath $SetupPath -Arguments @("--disable-current-user") -Step "disable current user"
        if (Test-LocalPinyinImeInCurrentUserList) {
            throw "Current user disable verification failed: InputMethodTips still contains LocalPinyinIME."
        }
    }
}

if ($PSCmdlet.ShouldProcess($currentDll, "unregister LocalPinyinIME system registration")) {
    Invoke-Setup -SetupPath $SetupPath -Arguments @("--unregister-system", "--dll", $currentDll) -Step "unregister system"
}

if ($RemoveFiles) {
    Assert-PathUnderRoot -Path $VersionDir -Root $InstallRoot
    if ($PSCmdlet.ShouldProcess($VersionDir, "delete exact version directory")) {
        Remove-Item -LiteralPath $VersionDir -Recurse -Force
    }
}

Write-Host "Uninstall flow finished. Restart Windows before deleting any retained empty parent directories."
