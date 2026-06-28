[CmdletBinding()]
param(
    [string]$SetupPath = ""
)

$ErrorActionPreference = "Stop"

$Clsid = "{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
$ProfileGuid = "{84D58E7C-481E-4D20-A951-4ED39F01D8D5}"
$LanguageTag = "zh-Hans-CN"
$ExpectedTip = "0804:$Clsid$ProfileGuid"
$CanonicalTip = "0x0804:$Clsid$ProfileGuid;"

function Format-Bool {
    param([bool]$Value)
    if ($Value) { return "TRUE" }
    return "FALSE"
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

function Invoke-SetupReadOnly {
    param([string]$Path)

    $arguments = @("--verify")
    Write-Host "Command: $(Format-CommandLine -FileName $Path -Arguments $arguments)"

    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $Path
    $psi.Arguments = "--verify"
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($psi)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($stderr) { Write-Error $stderr.TrimEnd() -ErrorAction Continue }
    return [PSCustomObject]@{
        ExitCode = $process.ExitCode
        StdOut = $stdout
        StdErr = $stderr
    }
}

function Get-SetupValue {
    param([string]$Text, [string]$Pattern)
    if ($Text -match $Pattern) {
        return $Matches[1]
    }
    return "<not reported>"
}

function Get-CurrentInprocServer {
    $path = "Registry::HKEY_CLASSES_ROOT\CLSID\$Clsid\InprocServer32"
    if (!(Test-Path -LiteralPath $path)) { return "" }
    $item = Get-Item -LiteralPath $path
    return [string]$item.GetValue("")
}

function Test-ComClsidExists {
    return Test-Path -LiteralPath "Registry::HKEY_CLASSES_ROOT\CLSID\$Clsid"
}

function Normalize-LocalPinyinTip {
    param([string]$Tip)
    $value = ([string]$Tip).Trim().TrimEnd(';')
    $value = $value -replace '(?i)^0x', ''
    return $value.ToUpperInvariant()
}

function Test-LocalPinyinImeInCurrentUserList {
    Write-Host "Current user language: $LanguageTag"
    Write-Host "Expected TIP: $ExpectedTip"
    if (!(Get-Command Get-WinUserLanguageList -ErrorAction SilentlyContinue)) {
        Write-Host "InputMethodTips contains LocalPinyinIME: UNKNOWN"
        Write-Host "Reason: Get-WinUserLanguageList is unavailable."
        return [PSCustomObject]@{ Verified = $false; Contains = $false }
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
    return [PSCustomObject]@{ Verified = $true; Contains = $contains }
}

function Get-PeSummary {
    param([string]$Path)
    if (!(Test-Path -LiteralPath $Path)) {
        return [PSCustomObject]@{ Machine = "<missing>"; OptionalMagic = "<missing>"; IsX64Pe32Plus = $false }
    }

    $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    try {
        $reader = [IO.BinaryReader]::new($stream)
        try {
            if ($reader.ReadUInt16() -ne 0x5A4D) {
                return [PSCustomObject]@{ Machine = "<bad MZ>"; OptionalMagic = "<bad MZ>"; IsX64Pe32Plus = $false }
            }
            $stream.Seek(0x3C, [IO.SeekOrigin]::Begin) | Out-Null
            $peOffset = $reader.ReadInt32()
            $stream.Seek($peOffset, [IO.SeekOrigin]::Begin) | Out-Null
            if ($reader.ReadUInt32() -ne 0x00004550) {
                return [PSCustomObject]@{ Machine = "<bad PE>"; OptionalMagic = "<bad PE>"; IsX64Pe32Plus = $false }
            }
            $machine = $reader.ReadUInt16()
            $stream.Seek(18, [IO.SeekOrigin]::Current) | Out-Null
            $magic = $reader.ReadUInt16()
            return [PSCustomObject]@{
                Machine = ("0x{0:X4}" -f $machine)
                OptionalMagic = ("0x{0:X4}" -f $magic)
                IsX64Pe32Plus = (($machine -eq 0x8664) -and ($magic -eq 0x020B))
            }
        } finally {
            $reader.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
}

if (!$SetupPath) {
    $packageSetup = Join-Path (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")) "bin\LocalPinyinImeSetup.exe"
    if (Test-Path -LiteralPath $packageSetup) {
        $SetupPath = $packageSetup
    } else {
        $currentDll = Get-CurrentInprocServer
        if ($currentDll) {
            $installedSetup = Join-Path (Split-Path -Parent $currentDll) "LocalPinyinImeSetup.exe"
            if (Test-Path -LiteralPath $installedSetup) {
                $SetupPath = $installedSetup
            }
        }
    }
}

if (!$SetupPath -or !(Test-Path -LiteralPath $SetupPath)) {
    throw "LocalPinyinImeSetup.exe was not found. Pass -SetupPath or run this script from the release package scripts directory."
}

$resolvedSetup = (Resolve-Path -LiteralPath $SetupPath).Path
$inproc = Get-CurrentInprocServer
$setupResult = Invoke-SetupReadOnly -Path $resolvedSetup

Write-Host ""
Write-Host "System registration:"
Write-Host "- COM CLSID exists: $(Format-Bool (Test-ComClsidExists))"
Write-Host "- InprocServer32 path: $(if ($inproc) { $inproc } else { '<missing>' })"
Write-Host "- TSF GetProfile: $(Get-SetupValue $setupResult.StdOut 'GetProfile HRESULT:\s*(0x[0-9A-Fa-f]+)')"
Write-Host "- GetProfile valid: $(Get-SetupValue $setupResult.StdOut 'GetProfile LocalPinyinIME valid:\s*(TRUE|FALSE)')"
Write-Host "- EnumProfiles contains profile: $(Get-SetupValue $setupResult.StdOut 'EnumProfiles contains LocalPinyinIME:\s*(TRUE|FALSE)')"
Write-Host "- Keyboard TIP category contains CLSID: $(Get-SetupValue $setupResult.StdOut 'GUID_TFCAT_TIP_KEYBOARD contains LocalPinyinIME CLSID:\s*(TRUE|FALSE)')"
Write-Host "- Setup verify exit code: $($setupResult.ExitCode)"

Write-Host ""
Write-Host "Current user:"
Write-Host "- IsEnabledLanguageProfile HRESULT: $(Get-SetupValue $setupResult.StdOut 'IsEnabledLanguageProfile HRESULT:\s*(0x[0-9A-Fa-f]+)')"
Write-Host "- IsEnabledLanguageProfile: $(Get-SetupValue $setupResult.StdOut 'current user enabled:\s*(TRUE|FALSE)')"
$tipState = Test-LocalPinyinImeInCurrentUserList

Write-Host ""
Write-Host "Binary:"
$dllPath = $inproc
if (!$dllPath -and $resolvedSetup) {
    $candidate = Join-Path (Split-Path -Parent $resolvedSetup) "LocalPinyinIME.dll"
    if (Test-Path -LiteralPath $candidate) {
        $dllPath = $candidate
    }
}
Write-Host "- DLL path: $(if ($dllPath) { $dllPath } else { '<missing>' })"
if ($dllPath -and (Test-Path -LiteralPath $dllPath)) {
    $hash = Get-FileHash -LiteralPath $dllPath -Algorithm SHA256
    $pe = Get-PeSummary -Path $dllPath
    $signature = Get-AuthenticodeSignature -LiteralPath $dllPath
    Write-Host "- SHA256: $($hash.Hash)"
    Write-Host "- architecture: Machine=$($pe.Machine), OptionalMagic=$($pe.OptionalMagic), x64 PE32+=$(Format-Bool $pe.IsX64Pe32Plus)"
    Write-Host "- Authenticode status: $($signature.Status)"
} else {
    Write-Host "- SHA256: <missing>"
    Write-Host "- architecture: <missing>"
    Write-Host "- Authenticode status: <missing>"
}

if (($setupResult.ExitCode -ne 0) -or !$tipState.Verified -or !$tipState.Contains) {
    exit 1
}
exit 0
