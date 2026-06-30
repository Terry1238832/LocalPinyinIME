[CmdletBinding()]
param(
    [string]$IsccPath = "",
    [string]$DistDir = "dist"
)

$ErrorActionPreference = "Stop"

$SourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
. (Join-Path $PSScriptRoot "LocalPinyinRelease.ps1")

$ReleaseVersion = Get-LocalPinyinReleaseVersion -SourceRoot $SourceRoot
$PackageName = "LocalPinyinIME-$($ReleaseVersion.Package)-win-x64"
$InstallerBaseName = "LocalPinyinIME-Setup-$($ReleaseVersion.Package)-x64"
$DistRoot = Join-Path $SourceRoot $DistDir
$StageRoot = Join-Path $DistRoot $PackageName
$InstallerDir = Join-Path $DistRoot "installers"
$InstallerPath = Join-Path $InstallerDir "$InstallerBaseName.exe"
$HashPath = "$InstallerPath.sha256"
$InnoScript = Join-Path $SourceRoot "installer\LocalPinyinIME.iss"
$InnoVersionConfig = Join-Path $DistRoot "LocalPinyinIME-InnoVersion.iss"

function Resolve-Iscc {
    param([string]$ExplicitPath)

    if (![string]::IsNullOrWhiteSpace($ExplicitPath)) {
        if (!(Test-Path -LiteralPath $ExplicitPath)) {
            throw "Specified ISCC.exe was not found: $ExplicitPath"
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    $command = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($command -and $command.Source) {
        return $command.Source
    }

    $uninstallRoots = @(
        "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall",
        "Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall"
    )
    foreach ($root in $uninstallRoots) {
        if (!(Test-Path -LiteralPath $root)) {
            continue
        }
        foreach ($key in Get-ChildItem -LiteralPath $root -ErrorAction SilentlyContinue) {
            $item = Get-ItemProperty -LiteralPath $key.PSPath -ErrorAction SilentlyContinue
            if (!$item -or [string]::IsNullOrWhiteSpace([string]$item.DisplayName)) {
                continue
            }
            if ([string]$item.DisplayName -notlike "Inno Setup 7*") {
                continue
            }
            $installLocation = [string]$item.InstallLocation
            if ([string]::IsNullOrWhiteSpace($installLocation)) {
                continue
            }
            $candidate = Join-Path $installLocation "ISCC.exe"
            if (Test-Path -LiteralPath $candidate) {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }
    }

    throw "ISCC.exe was not found. Install Inno Setup 7 locally, or pass -IsccPath with the full path to ISCC.exe. This script will not download or install Inno Setup."
}

function Assert-RequiredFile {
    param([string]$Path, [string]$Description)
    if (!(Test-Path -LiteralPath $Path)) {
        throw "Missing $Description`: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.Length -le 0) {
        throw "$Description is empty: $Path"
    }
}

if (!(Test-Path -LiteralPath $StageRoot)) {
    throw "Release staging directory is missing: $StageRoot. Run scripts\Build-Release.ps1 first."
}
if (!(Test-Path -LiteralPath $InnoScript)) {
    throw "Inno Setup script is missing: $InnoScript"
}

Assert-RequiredFile -Path (Join-Path $StageRoot "bin\LocalPinyinIME.dll") -Description "LocalPinyinIME.dll"
Assert-RequiredFile -Path (Join-Path $StageRoot "bin\LocalPinyinImeSetup.exe") -Description "LocalPinyinImeSetup.exe"
Assert-RequiredFile -Path (Join-Path $StageRoot "bin\LocalPinyinSettings.exe") -Description "LocalPinyinSettings.exe"
Assert-RequiredFile -Path (Join-Path $StageRoot "bin\dictionary\core_zh_pinyin.tsv") -Description "core dictionary"
Assert-RequiredFile -Path (Join-Path $StageRoot "bin\dictionary\local_core_zh_pinyin.tsv") -Description "local core dictionary"
Assert-RequiredFile -Path (Join-Path $StageRoot "release-manifest.json") -Description "release manifest"

$resolvedIscc = Resolve-Iscc -ExplicitPath $IsccPath
New-Item -ItemType Directory -Force -Path $InstallerDir | Out-Null

$innoConfig = @(
    "#define MyAppVersion `"$($ReleaseVersion.Package)`"",
    "#define MyAppPackageName `"$PackageName`"",
    "#define MyAppPackageDir `"..\dist\$PackageName`"",
    "#define MyAppOutputDir `"..\dist\installers`"",
    "#define MyAppSetupBaseFilename `"$InstallerBaseName`""
)
Set-Content -LiteralPath $InnoVersionConfig -Value $innoConfig -Encoding ASCII

if (Test-Path -LiteralPath $InstallerPath) {
    Remove-Item -LiteralPath $InstallerPath -Force
}
if (Test-Path -LiteralPath $HashPath) {
    Remove-Item -LiteralPath $HashPath -Force
}

Write-Host "Version: $($ReleaseVersion.Package)"
Write-Host "Release staging: $StageRoot"
Write-Host "ISCC: $resolvedIscc"
Write-Host "Inno script: $InnoScript"
Write-Host "Installer output: $InstallerPath"

& $resolvedIscc $InnoScript
if ($LASTEXITCODE -ne 0) {
    throw "ISCC.exe failed with exit code $LASTEXITCODE"
}

Assert-RequiredFile -Path $InstallerPath -Description "installer EXE"
$hash = Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256
Set-Content -LiteralPath $HashPath -Value "$($hash.Hash)  $(Split-Path -Leaf $InstallerPath)" -Encoding ASCII

Write-Host "Installer: $InstallerPath"
Write-Host "SHA256: $($hash.Hash)"
Write-Host "SHA256 file: $HashPath"
