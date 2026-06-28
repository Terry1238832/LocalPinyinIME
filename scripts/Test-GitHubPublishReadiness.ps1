[CmdletBinding()]
param(
    [string]$ProjectRoot
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

$issues = New-Object System.Collections.Generic.List[string]
$warnings = New-Object System.Collections.Generic.List[string]

function Add-Issue {
    param([string]$Message)
    $issues.Add($Message) | Out-Null
    Write-Host "[FAIL] $Message"
}

function Add-Warning {
    param([string]$Message)
    $warnings.Add($Message) | Out-Null
    Write-Host "[WARN] $Message"
}

function Add-Pass {
    param([string]$Message)
    Write-Host "[ OK ] $Message"
}

function Get-RelativePathForReport {
    param([string]$Path)
    $fullRoot = [System.IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\', '/')
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath.StartsWith($fullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $fullPath.Substring($fullRoot.Length).TrimStart('\', '/')
    }
    return $fullPath
}

function Test-Utf8File {
    param([string]$Path)
    try {
        $bytes = [System.IO.File]::ReadAllBytes($Path)
        $decoder = New-Object System.Text.UTF8Encoding($false, $true)
        [void]$decoder.GetString($bytes)
        return $true
    } catch {
        return $false
    }
}

function Get-GitIgnoreLines {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        return @()
    }

    return Get-Content -LiteralPath $Path | ForEach-Object { $_.Trim() } |
        Where-Object { $_ -and -not $_.StartsWith("#") }
}

function Test-IgnoreRulePresent {
    param(
        [string[]]$Rules,
        [string]$Expected
    )
    return [bool]($Rules | Where-Object { $_ -eq $Expected })
}

function Test-SkippedGeneratedPath {
    param([string]$Path)
    $relative = Get-RelativePathForReport -Path $Path
    return $relative -match '(^|[\\/])(\.git|\.vs|build[^\\/]*|dist|out|CMakeFiles|Testing)([\\/]|$)'
}

Write-Host "LocalPinyinIME GitHub publish readiness check"
Write-Host "Project root: $ProjectRoot"
Write-Host "This script is read-only and does not replace manual review."

$requiredFiles = @(
    ".gitignore",
    ".gitattributes",
    "README.md",
    "resources/dictionary/core_zh_pinyin.tsv"
)

foreach ($relativePath in $requiredFiles) {
    $fullPath = Join-Path $ProjectRoot $relativePath
    if (Test-Path -LiteralPath $fullPath) {
        Add-Pass "Required file exists: $relativePath"
    } else {
        Add-Issue "Required file missing: $relativePath"
    }
}

$gitIgnorePath = Join-Path $ProjectRoot ".gitignore"
$gitIgnoreRules = Get-GitIgnoreLines -Path $gitIgnorePath

$requiredIgnoreRules = @(
    "build/",
    "build-*/",
    "out/",
    "dist/",
    "*.dll",
    "*.exe",
    "*.pdb",
    "*.lib",
    "*.exp",
    "*.obj",
    "*.idb",
    "*.ilk",
    "*.ipdb",
    "*.iobj",
    "*.vcxproj.user",
    ".vs/",
    "CMakeFiles/",
    "CMakeCache.txt",
    "Testing/",
    "CTestTestfile.cmake",
    "install_manifest.txt",
    "compile_commands.json",
    "*.db",
    "*.sqlite",
    "*.sqlite3",
    "*.log",
    "status.log",
    "LocalPinyinIME.log"
)

foreach ($rule in $requiredIgnoreRules) {
    if (Test-IgnoreRulePresent -Rules $gitIgnoreRules -Expected $rule) {
        Add-Pass ".gitignore contains: $rule"
    } else {
        Add-Issue ".gitignore missing required rule: $rule"
    }
}

$dictionaryPath = Join-Path $ProjectRoot "resources/dictionary/core_zh_pinyin.tsv"
if (Test-Path -LiteralPath $dictionaryPath) {
    if (Test-Utf8File -Path $dictionaryPath) {
        Add-Pass "Dictionary TSV is valid UTF-8"
    } else {
        Add-Issue "Dictionary TSV is not valid UTF-8"
    }

    $releaseHelper = Join-Path $ProjectRoot "scripts/LocalPinyinRelease.ps1"
    if (Test-Path -LiteralPath $releaseHelper) {
        . $releaseHelper
        $stats = Get-LocalPinyinDictionaryStats -Path $dictionaryPath
        Write-Host ("Dictionary stats: sourceRows={0}, duplicateRows={1}, invalidRows={2}, validEntries={3}" -f `
            $stats.sourceRows, $stats.duplicateRows, $stats.invalidRows, $stats.validEntries)
        if ($stats.validEntries -ge 300) {
            Add-Pass "Dictionary valid entries >= 300"
        } else {
            Add-Issue "Dictionary valid entries below 300"
        }
    } else {
        Add-Issue "Dictionary stats helper missing: scripts/LocalPinyinRelease.ps1"
    }
}

$generatedDirs = Get-ChildItem -LiteralPath $ProjectRoot -Directory -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -eq "build" -or $_.Name -like "build-*" -or $_.Name -eq "dist" -or $_.Name -eq "out" }

foreach ($dir in $generatedDirs) {
    $expectedRule = $null
    if ($dir.Name -eq "build") { $expectedRule = "build/" }
    elseif ($dir.Name -like "build-*") { $expectedRule = "build-*/" }
    elseif ($dir.Name -eq "dist") { $expectedRule = "dist/" }
    elseif ($dir.Name -eq "out") { $expectedRule = "out/" }

    if ($expectedRule -and (Test-IgnoreRulePresent -Rules $gitIgnoreRules -Expected $expectedRule)) {
        Add-Warning ("Generated directory exists but is covered by .gitignore: {0}" -f $dir.Name)
    } else {
        Add-Issue ("Generated directory exists and is not covered by .gitignore: {0}" -f $dir.Name)
    }
}

$sensitiveNamePattern = '(?i)(^\.env$|api.?key|token|password|credential|private.?key|\.pfx$|\.pem$|\.key$)'
$allFiles = Get-ChildItem -LiteralPath $ProjectRoot -Recurse -File -Force -ErrorAction SilentlyContinue |
    Where-Object { -not (Test-SkippedGeneratedPath -Path $_.FullName) }

foreach ($file in $allFiles) {
    if ($file.Name -match $sensitiveNamePattern) {
        Add-Issue ("Suspicious file name: {0}" -f (Get-RelativePathForReport -Path $file.FullName))
    }
}

$scanRoots = @("src", "scripts", "cmake", "tests", "installer") |
    ForEach-Object { Join-Path $ProjectRoot $_ } |
    Where-Object { Test-Path -LiteralPath $_ }

$scanExtensions = @(".cpp", ".c", ".h", ".hpp", ".cmake", ".ps1", ".psm1", ".md", ".json", ".rc", ".iss", ".txt", ".tsv", ".in", ".def")
$thisScript = [System.IO.Path]::GetFullPath($MyInvocation.MyCommand.Path)
$contentPatterns = @(
    @{ Name = "user absolute path"; Regex = 'C:\\Users\\29783\\' },
    @{ Name = "local Program Files path"; Regex = 'C:\\Program Files\\LocalPinyinIME\\' },
    @{ Name = "GitHub token"; Regex = '(ghp|gho|ghu|ghs|ghr)_[A-Za-z0-9_]{30,}|github_pat_[A-Za-z0-9_]{20,}' },
    @{ Name = "possible secret assignment"; Regex = '(?i)\b(api[_-]?key|access[_-]?token|secret[_-]?token|password|credential)\b\s*[:=]\s*["''][^"'']{8,}["'']?' }
)

foreach ($root in $scanRoots) {
    $files = Get-ChildItem -LiteralPath $root -Recurse -File -Force -ErrorAction SilentlyContinue |
        Where-Object {
            $full = [System.IO.Path]::GetFullPath($_.FullName)
            ($scanExtensions -contains $_.Extension.ToLowerInvariant()) -and
                ($full -ne $thisScript) -and
                (-not (Test-SkippedGeneratedPath -Path $full))
        }

    foreach ($file in $files) {
        $lineNumber = 0
        foreach ($line in [System.IO.File]::ReadLines($file.FullName)) {
            $lineNumber++
            foreach ($pattern in $contentPatterns) {
                if ($line -match $pattern.Regex) {
                    Add-Issue ("Sensitive content pattern '{0}' at {1}:{2}" -f `
                        $pattern.Name, (Get-RelativePathForReport -Path $file.FullName), $lineNumber)
                }
            }
        }
    }
}

Write-Host ("Warnings: {0}" -f $warnings.Count)
Write-Host ("Issues: {0}" -f $issues.Count)
Write-Host "This script cannot replace manual review before publishing."

if ($issues.Count -gt 0) {
    exit 1
}

exit 0
