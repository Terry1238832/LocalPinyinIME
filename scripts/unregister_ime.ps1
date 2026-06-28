param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [string]$BuildDir = "build",
    [string]$DllPath = ""
)

$ErrorActionPreference = "Stop"

function Assert-Admin {
    $principal = [Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "Run this script from an elevated PowerShell session."
    }
}

function Invoke-Setup {
    param([string]$SetupPath, [string[]]$Arguments)

    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $SetupPath
    $psi.Arguments = ($Arguments | ForEach-Object { '"' + ($_ -replace '"', '\"') + '"' }) -join ' '
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
        throw "LocalPinyinImeSetup unregister failed with exit code $($process.ExitCode)."
    }
}

$dll = if ($DllPath) {
    (Resolve-Path -LiteralPath $DllPath).Path
} else {
    Join-Path (Resolve-Path -LiteralPath $BuildDir) "bin\$Configuration\LocalPinyinIME.dll"
}
if (!(Test-Path -LiteralPath $dll)) {
    throw "LocalPinyinIME.dll not found: $dll"
}

$setup = Join-Path (Split-Path -Parent $dll) "LocalPinyinImeSetup.exe"
if (!(Test-Path -LiteralPath $setup)) {
    throw "LocalPinyinImeSetup.exe not found next to DLL. Build the LocalPinyinImeSetup target first: $setup"
}

Assert-Admin
Invoke-Setup -SetupPath $setup -Arguments @("--unregister-system", "--dll", $dll)
