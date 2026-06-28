$ErrorActionPreference = "Stop"

function Get-LocalPinyinReleaseVersion {
    param([string]$SourceRoot)

    $versionFile = Join-Path $SourceRoot "cmake\LocalPinyinReleaseVersion.cmake"
    if (!(Test-Path -LiteralPath $versionFile)) {
        throw "Release version source is missing: $versionFile"
    }

    $values = @{}
    foreach ($line in Get-Content -LiteralPath $versionFile -Encoding UTF8) {
        if ($line -match '^\s*set\s*\(\s*(LOCALPINYIN_VERSION_(?:MAJOR|MINOR|PATCH|CHANNEL))\s+"?([^"\)]+)"?\s*\)') {
            $values[$Matches[1]] = $Matches[2]
        }
    }

    foreach ($name in @("LOCALPINYIN_VERSION_MAJOR", "LOCALPINYIN_VERSION_MINOR", "LOCALPINYIN_VERSION_PATCH", "LOCALPINYIN_VERSION_CHANNEL")) {
        if (!$values.ContainsKey($name)) {
            throw "Release version source does not define $name"
        }
    }

    $numeric = "$($values.LOCALPINYIN_VERSION_MAJOR).$($values.LOCALPINYIN_VERSION_MINOR).$($values.LOCALPINYIN_VERSION_PATCH)"
    $channel = [string]$values.LOCALPINYIN_VERSION_CHANNEL
    $package = $numeric
    if (![string]::IsNullOrWhiteSpace($channel)) {
        $package = "$numeric-$channel"
    }

    return [PSCustomObject]@{
        Major = [int]$values.LOCALPINYIN_VERSION_MAJOR
        Minor = [int]$values.LOCALPINYIN_VERSION_MINOR
        Patch = [int]$values.LOCALPINYIN_VERSION_PATCH
        Channel = $channel
        Numeric = $numeric
        Package = $package
    }
}

function Get-LocalPinyinDictionaryStats {
    param([string]$Path)

    if (!(Test-Path -LiteralPath $Path)) {
        throw "Dictionary file is missing: $Path"
    }

    $seen = @{}
    $stats = [ordered]@{
        sourceRows = 0
        commentRows = 0
        blankRows = 0
        duplicateRows = 0
        invalidRows = 0
        validEntries = 0
    }

    foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
        $stats.sourceRows++
        $text = [string]$line
        $trimmed = $text.Trim()
        if ([string]::IsNullOrWhiteSpace($trimmed)) {
            $stats.blankRows++
            continue
        }
        if ($trimmed.StartsWith("#")) {
            $stats.commentRows++
            continue
        }

        $parts = $text -split "`t", -1
        $frequency = 0
        if ($parts.Count -ne 3 -or
            [string]::IsNullOrWhiteSpace($parts[0]) -or
            [string]::IsNullOrWhiteSpace($parts[1]) -or
            ![int]::TryParse($parts[2].Trim(), [ref]$frequency) -or
            $frequency -le 0) {
            $stats.invalidRows++
            continue
        }

        $pinyin = ([string]$parts[0]).Trim().ToLowerInvariant() -replace '[^a-z]', ''
        $word = ([string]$parts[1]).Trim()
        if ([string]::IsNullOrWhiteSpace($pinyin) -or [string]::IsNullOrWhiteSpace($word)) {
            $stats.invalidRows++
            continue
        }

        $key = "$pinyin`t$word"
        if ($seen.ContainsKey($key)) {
            $stats.duplicateRows++
            continue
        }

        $seen[$key] = $true
        $stats.validEntries++
    }

    return [PSCustomObject]$stats
}

function Assert-LocalPinyinDictionaryFile {
    param([string]$Path, [int]$MinimumEntries)

    if (!(Test-Path -LiteralPath $Path)) {
        throw "Dictionary file is missing: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if ($item.Length -le 0) {
        throw "Dictionary file is empty: $Path"
    }

    $stats = Get-LocalPinyinDictionaryStats -Path $Path
    if ($stats.validEntries -lt $MinimumEntries) {
        throw "Dictionary valid entry count is too small: $($stats.validEntries) < $MinimumEntries"
    }
    return $stats
}

function Expand-LocalPinyinReleaseText {
    param([string]$Text, [object]$Version)

    return $Text.
        Replace("@LOCALPINYIN_VERSION_NUMERIC@", $Version.Numeric).
        Replace("@LOCALPINYIN_VERSION_PACKAGE@", $Version.Package).
        Replace("@LOCALPINYIN_PACKAGE_NAME@", "LocalPinyinIME-$($Version.Package)-win-x64")
}
