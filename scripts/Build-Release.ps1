[CmdletBinding()]
param(
    [string]$Version = "",
    [string]$BuildDir = "build-release-x64",
    [string]$DistDir = "dist",
    [switch]$UseNinja,
    [string]$CMakePath = "cmake",
    [string]$CTestPath = "ctest",
    [string]$NinjaPath = "",
    [string]$PythonExecutable = "",
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
$ArtifactEvidenceDir = Join-Path $StageRoot "evidence"
$ArtifactSymbolsDir = Join-Path $ArtifactEvidenceDir "symbols"
$ArtifactManifestPath = Join-Path $ArtifactEvidenceDir "ARTIFACT-MANIFEST.json"
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

function Assert-NonEmptyFile {
    param([string]$Path, [string]$Description)
    if (!(Test-Path -LiteralPath $Path)) {
        throw "$Description is missing: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.Length -le 0) {
        throw "$Description is empty: $Path"
    }
}

function Get-ArtifactFileEvidence {
    param([string]$Path)

    Assert-NonEmptyFile -Path $Path -Description "Artifact"
    $item = Get-Item -LiteralPath $Path
    $hash = Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256
    return [ordered]@{
        path = $item.FullName
        sha256 = $hash.Hash
        size = $item.Length
        creationTimeUtc = $item.CreationTimeUtc.ToString("o")
        lastWriteTimeUtc = $item.LastWriteTimeUtc.ToString("o")
    }
}

function Assert-SameFileHash {
    param([string]$Source, [string]$Destination, [string]$Description)
    $sourceHash = Get-FileHash -LiteralPath $Source -Algorithm SHA256
    $destinationHash = Get-FileHash -LiteralPath $Destination -Algorithm SHA256
    if ($sourceHash.Hash -ne $destinationHash.Hash) {
        throw "$Description hash mismatch. source=$($sourceHash.Hash) destination=$($destinationHash.Hash)"
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
    $buildRoot = Join-Path $SourceRoot $BuildDir
    $candidates = @(
        (Join-Path $SourceRoot "$BuildDir\bin\Release\$Name"),
        (Join-Path $SourceRoot "$BuildDir\bin\$Name"),
        (Join-Path $SourceRoot "$BuildDir\Release\$Name")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            $resolved = (Resolve-Path -LiteralPath $candidate).Path
            Assert-UnderRoot -Path $resolved -Root $buildRoot
            Assert-NonEmptyFile -Path $resolved -Description "Required Release build output"
            return $resolved
        }
    }
    throw "Required Release build output was not found: $Name"
}

function Resolve-BuiltPdb {
    param([string]$BinaryName)
    $pdbName = [IO.Path]::GetFileNameWithoutExtension($BinaryName) + ".pdb"
    $binaryPath = Resolve-BuiltFile $BinaryName
    $binaryDir = Split-Path -Parent $binaryPath
    $buildRoot = Join-Path $SourceRoot $BuildDir
    $candidates = @(
        (Join-Path $binaryDir $pdbName),
        (Join-Path $SourceRoot "$BuildDir\bin\Release\$pdbName"),
        (Join-Path $SourceRoot "$BuildDir\bin\$pdbName"),
        (Join-Path $SourceRoot "$BuildDir\Release\$pdbName")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            $resolved = (Resolve-Path -LiteralPath $candidate).Path
            Assert-UnderRoot -Path $resolved -Root $buildRoot
            Assert-NonEmptyFile -Path $resolved -Description "Required private PDB for $BinaryName"
            return $resolved
        }
    }
    throw "Required private PDB was not found for $BinaryName in build directory '$BuildDir'."
}

function Copy-RequiredFile {
    param([string]$Source, [string]$Destination)
    Assert-NonEmptyFile -Path $Source -Description "Required file"
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
    Assert-NonEmptyFile -Path $Destination -Description "Copied file"
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

function Copy-ReleaseBinaryToStage {
    param([string]$Name)
    $source = Resolve-BuiltFile $Name
    $destination = Join-Path $StageRoot "bin\$Name"
    if (Test-Path -LiteralPath $destination) {
        throw "Staging destination already contains a same-name binary before copy: $destination"
    }
    Copy-RequiredFile -Source $source -Destination $destination
    Assert-SameFileHash -Source $source -Destination $destination -Description "Staged $Name"
}

function Copy-PrivatePdbEvidence {
    param([string]$BinaryName)
    $sourcePdb = Resolve-BuiltPdb $BinaryName
    New-Item -ItemType Directory -Force -Path $ArtifactSymbolsDir | Out-Null
    $destinationPdb = Join-Path $ArtifactSymbolsDir (Split-Path -Leaf $sourcePdb)
    Copy-RequiredFile -Source $sourcePdb -Destination $destinationPdb
    Assert-SameFileHash -Source $sourcePdb -Destination $destinationPdb -Description "Private PDB for $BinaryName"
    return [ordered]@{
        source = Get-ArtifactFileEvidence -Path $sourcePdb
        private = Get-ArtifactFileEvidence -Path $destinationPdb
        exists = $true
    }
}

function New-BinaryArtifactTrace {
    param([string]$BinaryName)
    $source = Resolve-BuiltFile $BinaryName
    $staged = Join-Path $StageRoot "bin\$BinaryName"
    Assert-NonEmptyFile -Path $staged -Description "Staged $BinaryName"
    $sourceEvidence = Get-ArtifactFileEvidence -Path $source
    $stagedEvidence = Get-ArtifactFileEvidence -Path $staged
    return [ordered]@{
        source = $sourceEvidence
        staged = $stagedEvidence
        source_staging_hash_match = ($sourceEvidence.sha256 -eq $stagedEvidence.sha256)
    }
}

function Write-ArtifactEvidenceManifest {
    $buildRoot = Join-Path $SourceRoot $BuildDir
    New-Item -ItemType Directory -Force -Path $ArtifactEvidenceDir | Out-Null
    New-Item -ItemType Directory -Force -Path $ArtifactSymbolsDir | Out-Null

    $dllTrace = New-BinaryArtifactTrace -BinaryName "LocalPinyinIME.dll"
    $setupTrace = New-BinaryArtifactTrace -BinaryName "LocalPinyinImeSetup.exe"
    if (!$dllTrace.source_staging_hash_match) {
        throw "LocalPinyinIME.dll source/staging SHA-256 mismatch."
    }
    if (!$setupTrace.source_staging_hash_match) {
        throw "LocalPinyinImeSetup.exe source/staging SHA-256 mismatch."
    }

    $dllPdb = Copy-PrivatePdbEvidence -BinaryName "LocalPinyinIME.dll"
    $setupPdb = Copy-PrivatePdbEvidence -BinaryName "LocalPinyinImeSetup.exe"

    $dllTrace["pdb"] = $dllPdb
    $setupTrace["pdb"] = $setupPdb

    $manifest = [ordered]@{
        candidate_version = $ReleaseVersion.Package
        numeric_version = $ReleaseVersion.Numeric
        generated_at_utc = (Get-Date).ToUniversalTime().ToString("o")
        build_directory = (Resolve-Path -LiteralPath $buildRoot).Path
        staging_directory = (Resolve-Path -LiteralPath $StageRoot).Path
        artifact_manifest_path = $ArtifactManifestPath
        artifacts = [ordered]@{
            LocalPinyinIME_dll = $dllTrace
            LocalPinyinImeSetup_exe = $setupTrace
        }
    }
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $ArtifactManifestPath -Encoding UTF8

    Write-Host "Artifact evidence manifest: $ArtifactManifestPath"
    Write-Host "LocalPinyinIME.dll source/staging hash_match: $($dllTrace.source_staging_hash_match)"
    Write-Host "LocalPinyinImeSetup.exe source/staging hash_match: $($setupTrace.source_staging_hash_match)"
    Write-Host "Private symbols: $ArtifactSymbolsDir"
}

function Assert-BuiltDictionaryResources {
    $sourceDictionary = Join-Path $SourceRoot "resources\dictionary\core_zh_pinyin.tsv"
    $sourceLocalCoreDictionary = Join-Path $SourceRoot "resources\dictionary\local_core_zh_pinyin.tsv"
    $sourceHash = Get-FileHash -LiteralPath $sourceDictionary -Algorithm SHA256
    $sourceLocalCoreHash = Get-FileHash -LiteralPath $sourceLocalCoreDictionary -Algorithm SHA256
    foreach ($name in $BuildTargetsWithDictionary) {
        $builtFile = Resolve-BuiltFile $name
        $dictionary = Join-Path (Split-Path -Parent $builtFile) "dictionary\core_zh_pinyin.tsv"
        $localCoreDictionary = Join-Path (Split-Path -Parent $builtFile) "dictionary\local_core_zh_pinyin.tsv"
        $stats = Assert-LocalPinyinDictionaryFile -Path $dictionary -MinimumEntries $MinimumDictionaryEntries
        $hash = Get-FileHash -LiteralPath $dictionary -Algorithm SHA256
        if ($hash.Hash -ne $sourceHash.Hash) {
            throw "Built dictionary hash mismatch beside $name. expected=$($sourceHash.Hash) actual=$($hash.Hash)"
        }
        $localCoreStats = Get-LocalPinyinDictionaryStats -Path $localCoreDictionary
        $localCoreHash = Get-FileHash -LiteralPath $localCoreDictionary -Algorithm SHA256
        if ($localCoreHash.Hash -ne $sourceLocalCoreHash.Hash) {
            throw "Built local core dictionary hash mismatch beside $name. expected=$($sourceLocalCoreHash.Hash) actual=$($localCoreHash.Hash)"
        }
        Write-Host "Dictionary beside ${name}: validEntries=$($stats.validEntries), duplicateRows=$($stats.duplicateRows)"
        Write-Host "Local core dictionary beside ${name}: validEntries=$($localCoreStats.validEntries), duplicateRows=$($localCoreStats.duplicateRows)"
    }
}

function New-DeterministicZip {
    param([string]$Root, [string]$PackageName, [string]$Destination, [string]$TimestampUtc)

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $timestamp = [DateTimeOffset]::Parse($TimestampUtc).ToUniversalTime()
    $archive = [System.IO.Compression.ZipFile]::Open($Destination, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        $evidenceRoot = [IO.Path]::GetFullPath((Join-Path $Root "evidence")).TrimEnd('\') + '\'
        $files = Get-ChildItem -LiteralPath $Root -Recurse -File |
            Where-Object {
                $fullName = [IO.Path]::GetFullPath($_.FullName)
                !$fullName.StartsWith($evidenceRoot, [StringComparison]::OrdinalIgnoreCase)
            } |
            Sort-Object FullName
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
            if ($PythonExecutable) {
                $configureArgs += "-DLOCALPINYINIME_PYTHON_EXECUTABLE=$PythonExecutable"
            }
            Invoke-Native -FileName $CMakePath -Arguments $configureArgs
        } else {
            $configureArgs = @("-S", ".", "-B", $BuildDir, "-A", "x64")
            if ($PythonExecutable) {
                $configureArgs += "-DLOCALPINYINIME_PYTHON_EXECUTABLE=$PythonExecutable"
            }
            Invoke-Native -FileName $CMakePath -Arguments $configureArgs
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
            Copy-ReleaseBinaryToStage -Name $binary
        }

        $dictionarySource = Join-Path $SourceRoot "resources\dictionary\core_zh_pinyin.tsv"
        $dictionaryStage = Join-Path $StageRoot "bin\dictionary\core_zh_pinyin.tsv"
        $localCoreDictionarySource = Join-Path $SourceRoot "resources\dictionary\local_core_zh_pinyin.tsv"
        $localCoreDictionaryStage = Join-Path $StageRoot "bin\dictionary\local_core_zh_pinyin.tsv"
        Copy-RequiredFile -Source $dictionarySource -Destination $dictionaryStage
        Copy-RequiredFile -Source $localCoreDictionarySource -Destination $localCoreDictionaryStage
        $dictionaryStats = Assert-LocalPinyinDictionaryFile -Path $dictionaryStage -MinimumEntries $MinimumDictionaryEntries
        $localCoreDictionaryStats = Get-LocalPinyinDictionaryStats -Path $localCoreDictionaryStage
        Write-Host "Dictionary staged: $dictionaryStage"
        Write-Host "Dictionary validEntries: $($dictionaryStats.validEntries)"
        Write-Host "Dictionary duplicateRows: $($dictionaryStats.duplicateRows)"
        Write-Host "Local core dictionary staged: $localCoreDictionaryStage"
        Write-Host "Local core dictionary validEntries: $($localCoreDictionaryStats.validEntries)"
        Write-Host "Local core dictionary duplicateRows: $($localCoreDictionaryStats.duplicateRows)"

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
        $localCoreDictionaryStage = Join-Path $StageRoot "bin\dictionary\local_core_zh_pinyin.tsv"
        $dictionaryStats = Assert-LocalPinyinDictionaryFile -Path $dictionaryStage -MinimumEntries $MinimumDictionaryEntries
        $localCoreDictionaryStats = Get-LocalPinyinDictionaryStats -Path $localCoreDictionaryStage
        $dictionaryHash = Get-FileHash -LiteralPath $dictionaryStage -Algorithm SHA256
        $localCoreDictionaryHash = Get-FileHash -LiteralPath $localCoreDictionaryStage -Algorithm SHA256
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
            localCoreDictionary = [ordered]@{
                path = "bin/dictionary/local_core_zh_pinyin.tsv"
                sourceRows = $localCoreDictionaryStats.sourceRows
                commentRows = $localCoreDictionaryStats.commentRows
                blankRows = $localCoreDictionaryStats.blankRows
                duplicateRows = $localCoreDictionaryStats.duplicateRows
                invalidRows = $localCoreDictionaryStats.invalidRows
                validEntries = $localCoreDictionaryStats.validEntries
                sha256 = $localCoreDictionaryHash.Hash
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

    Invoke-Step "Write private artifact evidence" {
        Write-ArtifactEvidenceManifest
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
