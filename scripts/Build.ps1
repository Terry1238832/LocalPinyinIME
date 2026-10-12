[CmdletBinding()]
param(
    [string]$CMakePath = '',
    [string]$IsccPath = '',
    [string]$PythonPath = '',
    [int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$buildRoot = Join-Path $sourceRoot '.build'
$outputRoot = Join-Path $sourceRoot 'dist'
$stageRoot = Join-Path $buildRoot 'package'

function Find-Tool([string]$Explicit, [string]$Name, [string[]]$Candidates) {
    if ($Explicit) {
        if (!(Test-Path -LiteralPath $Explicit -PathType Leaf)) { throw "Tool missing: $Explicit" }
        return (Resolve-Path -LiteralPath $Explicit).Path
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    }
    throw "Tool not found: $Name. Pass its full path to this script."
}
function Invoke-Checked([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Tool exited with code $LASTEXITCODE" }
}

$vsRoots = @()
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path -LiteralPath $vswhere) {
    $vsRoots = @(& $vswhere -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
}
$cmakeCandidates = @($vsRoots | ForEach-Object { Join-Path $_ 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' })
$CMakePath = Find-Tool $CMakePath 'cmake.exe' $cmakeCandidates
$ctestPath = Join-Path (Split-Path -Parent $CMakePath) 'ctest.exe'
$PythonPath = Find-Tool $PythonPath 'python.exe' @(
    (Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe')
)
$isccCandidates = @()
foreach ($key in @('HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall', 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall', 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall')) {
    $apps = Get-ChildItem -LiteralPath $key -ErrorAction SilentlyContinue | Get-ItemProperty -ErrorAction SilentlyContinue
    foreach ($app in $apps) {
        if ($app.DisplayName -like 'Inno Setup*' -and $app.InstallLocation) { $isccCandidates += Join-Path $app.InstallLocation 'ISCC.exe' }
    }
}
$IsccPath = Find-Tool $IsccPath 'ISCC.exe' $isccCandidates

if (!$vsRoots.Count) { throw 'Visual Studio C++ build tools are required' }
New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
$devScript = Join-Path $buildRoot 'dev-env.cmd'
@(
    '@echo off',
    "call `"$(Join-Path $vsRoots[0] 'Common7\Tools\VsDevCmd.bat')`" -arch=x64 -host_arch=x64 >nul",
    'if errorlevel 1 exit /b 1',
    'set'
) | Set-Content -LiteralPath $devScript -Encoding ASCII
& cmd.exe /d /c $devScript | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}
if ($LASTEXITCODE -ne 0) { throw 'Failed to initialize C++ build tools' }
$env:VSLANG = '1033'
$ninjaPath = Find-Tool '' 'ninja.exe' @((Join-Path $vsRoots[0] 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'))
Invoke-Checked $CMakePath @('-S', $sourceRoot, '-B', $buildRoot, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER=cl.exe', '-DCMAKE_CXX_FLAGS_DEBUG=/Z7 /Ob0 /Od /RTC1', "-DCMAKE_MAKE_PROGRAM=$ninjaPath", "-DLOCALPINYINIME_PYTHON_EXECUTABLE=$PythonPath")
Invoke-Checked $CMakePath @('--build', $buildRoot, '--config', 'Release', '--parallel', "$Jobs")
Invoke-Checked $ctestPath @('--test-dir', $buildRoot, '-C', 'Release', '--output-on-failure')

# Staging always lives inside the checked build directory.
if (Test-Path -LiteralPath $stageRoot) {
    $resolvedStage = (Resolve-Path -LiteralPath $stageRoot).Path
    if (!$resolvedStage.StartsWith($buildRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe staging path' }
    if ((Get-Item -LiteralPath $resolvedStage).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing a linked staging directory' }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $stageRoot 'bin\dictionary') -Force | Out-Null
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
foreach ($binary in @('LocalPinyinIME.dll', 'LocalPinyinSettings.exe', 'LocalPinyinImeSetup.exe')) {
    Copy-Item -LiteralPath (Join-Path $buildRoot "bin\$binary") -Destination (Join-Path $stageRoot 'bin')
}
Copy-Item -LiteralPath (Join-Path $sourceRoot 'resources\dictionary\core_zh_pinyin.tsv'), (Join-Path $sourceRoot 'resources\dictionary\local_core_zh_pinyin.tsv') -Destination (Join-Path $stageRoot 'bin\dictionary')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'LICENSE') -Destination $stageRoot
$versionHeader = Get-Content -LiteralPath (Join-Path $buildRoot 'generated\localpinyin_version.h') -Raw
$version = [regex]::Match($versionHeader, '#define LOCALPINYINIME_VERSION_STRING_UTF8 "([^"]+)"').Groups[1].Value
if (!$version) { throw 'Cannot read generated release version' }
$installerName = "LocalPinyinIME-$version-x64"
$configPath = Join-Path $buildRoot 'InstallerVersion.iss'
@(
    "#define MyAppVersion `"$version`"",
    "#define MyAppPackageDir `"$stageRoot`"",
    "#define MyAppOutputDir `"$outputRoot`"",
    "#define MyAppSetupBaseFilename `"$installerName`""
) | Set-Content -LiteralPath $configPath -Encoding UTF8
Invoke-Checked $IsccPath @((Join-Path $sourceRoot 'installer\LocalPinyinIME.iss'))
$installerPath = Join-Path $outputRoot "$installerName.exe"
$hash = (Get-FileHash -LiteralPath $installerPath -Algorithm SHA256).Hash
Set-Content -LiteralPath "$installerPath.sha256" -Value "$hash  $installerName.exe" -Encoding ASCII
Write-Host "Installer: $installerPath"
Write-Host "Size: $([math]::Round((Get-Item -LiteralPath $installerPath).Length / 1MB, 2)) MB"
