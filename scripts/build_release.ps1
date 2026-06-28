param(
    [string]$BuildDir = "build-release-x64",
    [string]$DistDir = "dist"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "Build-Release.ps1") -BuildDir $BuildDir -DistDir $DistDir
