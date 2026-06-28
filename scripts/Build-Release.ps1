[CmdletBinding()]
param(
    [string]$Version = "",
    [string]$BuildDir = "build-release-x64",
    [string]$DistDir = "dist",
    [switch]$UseNinja,
    [string]$CMakePath = "cmake",
    [string]$CTestPath = "ctest",
    [string]$NinjaPath = "",
    [string]$PackageTimestampUtc = "2026-01-01T00:00:00Z"
)

$ErrorActionPreference = "Stop"

$SourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
. (Join-Path $PSScriptRoot "LocalPinyinRelease.ps1")

$ReleaseVersion = Get-LocalPinyinReleaseVersion -SourceRoot $SourceRoot
if (![string]::IsNullOrWhiteSpace($Version) -and $Version -ne $ReleaseVersion.Package) {
    throw "Version override is not allowed. Requested '$Version' but single source declares '$($ReleaseVersion.Package)'."
}
$Version = $ReleaseVersion.Package

$DistRoot = Join-Path $SourceRoot $DistDir
$PackageName = "LocalPinyinIME-$Version-win-x64"
$StageRoot = Join-Path $DistRoot $PackageName
$ZipPath = Join-Path $DistRoot "$PackageName.zip"
$DisplayName = "LocalPinyinIME - " +
    [string]([char]0x79BB) +
    [string]([char]0x7EBF) +
    [string]([char]0x62FC) +
    [string]([char]0x97F3) +
    [string]([char]0x8F93) +
    [string]([char]0x5165) +
    [string]([char]0x6CD5)
$MinimumDictionaryEntries = 300
$ReleaseBinaries = @(
    "LocalPinyinIME.dll",
    "LocalPinyinImeSetup.exe",
    "LocalPinyinImeAudit.exe",
    "LocalPinyinSettings.exe",
    "LocalPinyinImeDictionarySmoke.exe"
)
$BuildTargetsWithDictionary = @(
    $ReleaseBinaries +
    @(
        "LocalPinyinImeEnable.exe",
        "test_dictionary.exe",
        "test_pinyin_engine.exe",
        "test_candidate_ranker.exe",
        "test_user_learning.exe",
        "test_candidate_selection.exe",
        "test_dictionary_resource_layout.exe"
    )
)

function Assert-UnderRoot {
    param([string]$Path, [string]$Root)
    $fullPath = [IO.Path]::GetFullPath($Path)
    $fullRoot = [IO.Path]::GetFullPath($Root).TrimEnd('\') + '\'
    if (!$fullPath.StartsWith($fullRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to operate outside workspace package root: $fullPath"
    }
}

function Invoke-Native {
    param([string]$FileName, [string[]]$Arguments)
    & $FileName @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Native command failed with exit code ${LASTEXITCODE}: $FileName"
    }
}

function Invoke-Step {
    param([string]$Name, [scriptblock]$Action)
    Write-Host "== $Name =="
    & $Action
}

function Resolve-BuiltFile {
    param([string]$Name)
    $candidates = @(
        (Join-Path $SourceRoot "$BuildDir\bin\Release\$Name"),
        (Join-Path $SourceRoot "$BuildDir\bin\$Name"),
        (Join-Path $SourceRoot "$BuildDir\Release\$Name")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    throw "Required Release build output was not found: $Name"
}

function Copy-RequiredFile {
    param([string]$Source, [string]$Destination)
    if (!(Test-Path -LiteralPath $Source)) {
        throw "Required file is missing: $Source"
    }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Copy-ReleaseTextFile {
    param([string]$Source, [string]$Destination)
    if (!(Test-Path -LiteralPath $Source)) {
        throw "Required file is missing: $Source"
    }
    $text = Get-Content -LiteralPath $Source -Encoding UTF8 -Raw
    $expanded = Expand-LocalPinyinReleaseText -Text $text -Version $ReleaseVersion
    Set-Content -LiteralPath $Destination -Value $expanded -Encoding UTF8
}

function Assert-BuiltDictionaryResources {
    $sourceDictionary = Join-Path $SourceRoot "resources\dictionary\core_zh_pinyin.tsv"
    $sourceHash = Get-FileHash -LiteralPath $sourceDictionary -Algorithm SHA256
    foreach ($name in $BuildTargetsWithDictionary) {
        $builtFile = Resolve-BuiltFile $name
        $dictionary = Join-Path (Split-Path -Parent $builtFile) "dictionary\core_zh_pinyin.tsv"
        $stats = Assert-LocalPinyinDictionaryFile -Path $dictionary -MinimumEntries $MinimumDictionaryEntries
        $hash = Get-FileHash -LiteralPath $dictionary -Algorithm SHA256
        if ($hash.Hash -ne $sourceHash.Hash) {
            throw "Built dictionary hash mismatch beside $name. expected=$($sourceHash.Hash) actual=$($hash.Hash)"
        }
        Write-Host "Dictionary beside ${name}: validEntries=$($stats.validEntries), duplicateRows=$($stats.duplicateRows)"
    }
}

function New-DeterministicZip {
    param([string]$Root, [string]$PackageName, [string]$Destination, [string]$TimestampUtc)

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $timestamp = [DateTimeOffset]::Parse($TimestampUtc).ToUniversalTime()
    $archive = [System.IO.Compression.ZipFile]::Open($Destination, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        $files = Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName
        foreach ($file in $files) {
            $relative = $file.FullName.Substring($Root.Length + 1).Replace('\', '/')
            $entryName = "$PackageName/$relative"
            $entry = $archive.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
            $entry.LastWriteTime = $timestamp

            $inputStream = [IO.File]::OpenRead($file.FullName)
            try {
                $outputStream = $entry.Open()
                try {
                    $inputStream.CopyTo($outputStream)
                } finally {
                    $outputStream.Dispose()
                }
            } finally {
                $inputStream.Dispose()
            }
        }
    } finally {
        $archive.Dispose()
    }
}

Push-Location $SourceRoot
try {
    Invoke-Step "Read single release version source" {
        Write-Host "Numeric version: $($ReleaseVersion.Numeric)"
        Write-Host "Package version: $($ReleaseVersion.Package)"
        Write-Host "Package name: $PackageName"
    }

    Invoke-Step "Configure Release x64" {
        if ($UseNinja) {
            $configureArgs = @("-S", ".", "-B", $BuildDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release")
            if ($NinjaPath) {
                $configureArgs += "-DCMAKE_MAKE_PROGRAM=$NinjaPath"
            }
            Invoke-Native -FileName $CMakePath -Arguments $configureArgs
        } else {
            Invoke-Native -FileName $CMakePath -Arguments @("-S", ".", "-B", $BuildDir, "-A", "x64")
        }
    }

    Invoke-Step "Build Release x64" {
        Invoke-Native -FileName $CMakePath -Arguments @("--build", $BuildDir, "--config", "Release", "--parallel", "1")
    }

    Invoke-Step "Run Release tests" {
        Invoke-Native -FileName $CTestPath -Arguments @("--test-dir", $BuildDir, "-C", "Release", "--output-on-failure")
    }

    Invoke-Step "Verify built dictionary resources" {
        Assert-BuiltDictionaryResources
    }

    Invoke-Step "Stage release package" {
        New-Item -ItemType Directory -Force -Path $DistRoot | Out-Null
        Assert-UnderRoot -Path $StageRoot -Root $DistRoot
        if (Test-Path -LiteralPath $StageRoot) {
            Remove-Item -LiteralPath $StageRoot -Recurse -Force
        }
        if (Test-Path -LiteralPath $ZipPath) {
            Remove-Item -LiteralPath $ZipPath -Force
        }

        New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "bin") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "bin\dictionary") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "scripts") | Out-Null
        New-Item -ItemType Directory -Force -Path (Join-Path $StageRoot "docs") | Out-Null

        foreach ($binary in $ReleaseBinaries) {
            Copy-RequiredFile -Source (Resolve-BuiltFile $binary) -Destination (Join-Path $StageRoot "bin\$binary")
        }

        $dictionarySource = Join-Path $SourceRoot "resources\dictionary\core_zh_pinyin.tsv"
        $dictionaryStage = Join-Path $StageRoot "bin\dictionary\core_zh_pinyin.tsv"
        Copy-RequiredFile -Source $dictionarySource -Destination $dictionaryStage
        $dictionaryStats = Assert-LocalPinyinDictionaryFile -Path $dictionaryStage -MinimumEntries $MinimumDictionaryEntries
        Write-Host "Dictionary staged: $dictionaryStage"
        Write-Host "Dictionary validEntries: $($dictionaryStats.validEntries)"
        Write-Host "Dictionary duplicateRows: $($dictionaryStats.duplicateRows)"

        foreach ($script in @("Install-LocalPinyinIME.ps1", "Uninstall-LocalPinyinIME.ps1", "Verify-LocalPinyinIME.ps1", "Build-Release.ps1", "LocalPinyinRelease.ps1", "Test-ReleasePackage.ps1")) {
            Copy-RequiredFile -Source (Join-Path $SourceRoot "scripts\$script") -Destination (Join-Path $StageRoot "scripts\$script")
        }

        foreach ($doc in @("README-zh-CN.md", "INSTALL-zh-CN.md", "UNINSTALL-zh-CN.md", "TROUBLESHOOTING-zh-CN.md", "SECURITY-NOTES-zh-CN.md")) {
            Copy-ReleaseTextFile -Source (Join-Path $SourceRoot "docs\$doc") -Destination (Join-Path $StageRoot "docs\$doc")
        }

        Copy-RequiredFile -Source (Join-Path $SourceRoot "LICENSE") -Destination (Join-Path $StageRoot "LICENSES-or-NOTICES.txt")
    }

    Invoke-Step "Write release manifest and hashes" {
        $dictionaryStage = Join-Path $StageRoot "bin\dictionary\core_zh_pinyin.tsv"
        $dictionaryStats = Assert-LocalPinyinDictionaryFile -Path $dictionaryStage -MinimumEntries $MinimumDictionaryEntries
        $dictionaryHash = Get-FileHash -LiteralPath $dictionaryStage -Algorithm SHA256
        $manifest = [ordered]@{
            product = "LocalPinyinIME"
            displayName = $DisplayName
            version = $ReleaseVersion.Package
            numericVersion = $ReleaseVersion.Numeric
            architecture = "x64"
            clsid = "{7C0B4B75-80B0-4E1F-A4A5-4D49A5440D8A}"
            profileGuid = "{84D58E7C-481E-4D20-A951-4ED39F01D8D5}"
            langid = "0x0804"
            signed = $false
            releaseType = "unsigned development release"
            packageTimestampUtc = $PackageTimestampUtc
            dictionary = [ordered]@{
                path = "bin/dictionary/core_zh_pinyin.tsv"
                minimumEntries = $MinimumDictionaryEntries
                sourceRows = $dictionaryStats.sourceRows
                commentRows = $dictionaryStats.commentRows
                blankRows = $dictionaryStats.blankRows
                duplicateRows = $dictionaryStats.duplicateRows
                invalidRows = $dictionaryStats.invalidRows
                validEntries = $dictionaryStats.validEntries
                sha256 = $dictionaryHash.Hash
            }
        }
        $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $StageRoot "release-manifest.json") -Encoding UTF8

        $hashLines = New-Object System.Collections.Generic.List[string]
        $files = Get-ChildItem -LiteralPath $StageRoot -Recurse -File |
            Where-Object { $_.Name -ne "SHA256SUMS.txt" } |
            Sort-Object FullName
        foreach ($file in $files) {
            $hash = Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256
            $relative = $file.FullName.Substring($StageRoot.Length + 1).Replace('\', '/')
            $hashLines.Add("$($hash.Hash)  $relative")
        }
        Set-Content -LiteralPath (Join-Path $StageRoot "SHA256SUMS.txt") -Value $hashLines -Encoding ASCII
    }

    Invoke-Step "Write Inno auxiliary version config" {
        $innoConfig = @(
            "#define MyAppVersion `"$($ReleaseVersion.Package)`"",
            "#define MyAppPackageDir `"..\dist\$PackageName`""
        )
        Set-Content -LiteralPath (Join-Path $DistRoot "LocalPinyinIME-InnoVersion.iss") -Value $innoConfig -Encoding ASCII
    }

    Invoke-Step "Create ZIP" {
        New-DeterministicZip -Root $StageRoot -PackageName $PackageName -Destination $ZipPath -TimestampUtc $PackageTimestampUtc
    }

    Invoke-Step "Run ZIP extraction smoke" {
        & (Join-Path $PSScriptRoot "Test-ReleasePackage.ps1") -ZipPath $ZipPath -ExpectedVersion $ReleaseVersion.Package
        if ($LASTEXITCODE -ne 0) {
            throw "Release package smoke failed with exit code $LASTEXITCODE"
        }
    }

    Write-Host "Release directory: $StageRoot"
    Write-Host "Release ZIP: $ZipPath"
} finally {
    Pop-Location
}
