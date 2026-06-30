[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ZipPath,
    [string]$ExpectedVersion = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "LocalPinyinRelease.ps1")

function Assert-File {
    param([string]$Path, [string]$Description)
    if (!(Test-Path -LiteralPath $Path)) {
        throw "Missing $Description`: $Path"
    }
}

function Invoke-Smoke {
    param([string]$SmokePath, [string]$ResourceDir, [string]$WorkingDirectory)

    $psi = [Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = $SmokePath
    $psi.Arguments = "--resource-dir `"$ResourceDir`""
    $psi.WorkingDirectory = $WorkingDirectory
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($psi)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($stdout) { Write-Host $stdout.TrimEnd() }
    if ($stderr) { Write-Error $stderr.TrimEnd() -ErrorAction Continue }

    if ($process.ExitCode -ne 0) {
        throw "Dictionary smoke failed with exit code $($process.ExitCode)"
    }

    foreach ($pattern in @(
        'Dictionary loaded:\s*TRUE',
        'Valid entries:\s*([3-9][0-9][0-9]|[1-9][0-9]{3,})',
        'nihao first candidate:\s*\S+',
        'henbang first candidate:\s*\S+',
        'nihaoshijie first candidate:\s*\S+',
        'woxiangqubeijing first candidate:\s*\S+',
        'nihao expected match:\s*TRUE',
        'henbang expected match:\s*TRUE',
        'nihaoshijie expected match:\s*TRUE',
        'woxiangqubeijing expected match:\s*TRUE'
    )) {
        if ($stdout -notmatch $pattern) {
            throw "Dictionary smoke output did not match: $pattern"
        }
    }
}

$resolvedZip = (Resolve-Path -LiteralPath $ZipPath).Path
if (![IO.Path]::IsPathRooted($resolvedZip)) {
    throw "ZipPath must resolve to an absolute path: $ZipPath"
}

$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($ExpectedVersion) -and (Test-Path -LiteralPath (Join-Path $sourceRoot "cmake\LocalPinyinReleaseVersion.cmake"))) {
    $ExpectedVersion = (Get-LocalPinyinReleaseVersion -SourceRoot $sourceRoot).Package
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("LocalPinyinIME-release-test-" + [guid]::NewGuid().ToString("N"))
$unrelatedCwd = Join-Path ([IO.Path]::GetTempPath()) ("LocalPinyinIME-unrelated-cwd-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
New-Item -ItemType Directory -Force -Path $unrelatedCwd | Out-Null

try {
    [System.IO.Compression.ZipFile]::ExtractToDirectory($resolvedZip, $tempRoot)
    $roots = @(Get-ChildItem -LiteralPath $tempRoot -Directory)
    if ($roots.Count -ne 1) {
        throw "Expected exactly one package root in ZIP, found $($roots.Count)."
    }
    $packageRoot = $roots[0].FullName

    $dll = Join-Path $packageRoot "bin\LocalPinyinIME.dll"
    $smoke = Join-Path $packageRoot "bin\LocalPinyinImeDictionarySmoke.exe"
    $dictionary = Join-Path $packageRoot "bin\dictionary\core_zh_pinyin.tsv"
    $localCoreDictionary = Join-Path $packageRoot "bin\dictionary\local_core_zh_pinyin.tsv"
    $manifestPath = Join-Path $packageRoot "release-manifest.json"
    $sha256Path = Join-Path $packageRoot "SHA256SUMS.txt"

    Assert-File -Path $dll -Description "LocalPinyinIME.dll"
    Assert-File -Path $smoke -Description "LocalPinyinImeDictionarySmoke.exe"
    Assert-File -Path $dictionary -Description "core_zh_pinyin.tsv"
    Assert-File -Path $localCoreDictionary -Description "local_core_zh_pinyin.tsv"
    Assert-File -Path $manifestPath -Description "release-manifest.json"
    Assert-File -Path $sha256Path -Description "SHA256SUMS.txt"

    $manifest = Get-Content -LiteralPath $manifestPath -Encoding UTF8 -Raw | ConvertFrom-Json
    if ([string]::IsNullOrWhiteSpace([string]$manifest.version)) { throw "Manifest version is missing." }
    if ($ExpectedVersion -and [string]$manifest.version -ne $ExpectedVersion) {
        throw "Manifest version mismatch. expected=$ExpectedVersion actual=$($manifest.version)"
    }
    if ([string]$manifest.architecture -ne "x64") { throw "Manifest architecture is not x64: $($manifest.architecture)" }
    if ([string]$manifest.dictionary.path -ne "bin/dictionary/core_zh_pinyin.tsv") {
        throw "Manifest dictionary.path mismatch: $($manifest.dictionary.path)"
    }
    if ([string]$manifest.localCoreDictionary.path -ne "bin/dictionary/local_core_zh_pinyin.tsv") {
        throw "Manifest localCoreDictionary.path mismatch: $($manifest.localCoreDictionary.path)"
    }

    $stats = Assert-LocalPinyinDictionaryFile -Path $dictionary -MinimumEntries 300
    if ([int]$manifest.dictionary.validEntries -ne [int]$stats.validEntries) {
        throw "Manifest dictionary.validEntries mismatch. manifest=$($manifest.dictionary.validEntries) actual=$($stats.validEntries)"
    }
    foreach ($name in @("sourceRows", "commentRows", "blankRows", "duplicateRows", "invalidRows")) {
        if ($null -eq $manifest.dictionary.$name -or [int]$manifest.dictionary.$name -ne [int]$stats.$name) {
            throw "Manifest dictionary.$name mismatch. manifest=$($manifest.dictionary.$name) actual=$($stats.$name)"
        }
    }

    $dictionaryHash = Get-FileHash -LiteralPath $dictionary -Algorithm SHA256
    $localCoreDictionaryHash = Get-FileHash -LiteralPath $localCoreDictionary -Algorithm SHA256
    if ([string]$manifest.dictionary.sha256 -ne $dictionaryHash.Hash) {
        throw "Manifest dictionary.sha256 mismatch. manifest=$($manifest.dictionary.sha256) actual=$($dictionaryHash.Hash)"
    }
    if ([string]$manifest.localCoreDictionary.sha256 -ne $localCoreDictionaryHash.Hash) {
        throw "Manifest localCoreDictionary.sha256 mismatch. manifest=$($manifest.localCoreDictionary.sha256) actual=$($localCoreDictionaryHash.Hash)"
    }
    $hashText = Get-Content -LiteralPath $sha256Path -Encoding ASCII -Raw
    if ($hashText -notmatch [regex]::Escape("$($dictionaryHash.Hash)  bin/dictionary/core_zh_pinyin.tsv")) {
        throw "SHA256SUMS.txt does not contain the package dictionary hash."
    }
    if ($hashText -notmatch [regex]::Escape("$($localCoreDictionaryHash.Hash)  bin/dictionary/local_core_zh_pinyin.tsv")) {
        throw "SHA256SUMS.txt does not contain the package local core dictionary hash."
    }

    Write-Host "Package root: $packageRoot"
    Write-Host "Manifest version: $($manifest.version)"
    Write-Host "Dictionary SHA256: $($dictionaryHash.Hash)"
    Write-Host "Local core dictionary SHA256: $($localCoreDictionaryHash.Hash)"
    Write-Host "Dictionary sourceRows: $($stats.sourceRows)"
    Write-Host "Dictionary commentRows: $($stats.commentRows)"
    Write-Host "Dictionary blankRows: $($stats.blankRows)"
    Write-Host "Dictionary duplicateRows: $($stats.duplicateRows)"
    Write-Host "Dictionary invalidRows: $($stats.invalidRows)"
    Write-Host "Dictionary validEntries: $($stats.validEntries)"

    Write-Host "== Smoke from package root =="
    Invoke-Smoke -SmokePath $smoke -ResourceDir (Join-Path $packageRoot "bin") -WorkingDirectory $packageRoot
    Write-Host "== Smoke from unrelated cwd =="
    Invoke-Smoke -SmokePath $smoke -ResourceDir (Join-Path $packageRoot "bin") -WorkingDirectory $unrelatedCwd
    Write-Host "Release package smoke passed: $resolvedZip"
} finally {
    Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $unrelatedCwd -Recurse -Force -ErrorAction SilentlyContinue
}
