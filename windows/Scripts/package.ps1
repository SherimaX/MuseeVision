# Package Musée Vision for Windows: a standalone game in its own folder under windows/Builds/
# (MuseeVision-<yyyyMMdd-HHmm>/MuseeVision.exe), so a build that is running is never overwritten.
# windows/Builds/LATEST.txt names the newest.
#
#   powershell -ExecutionPolicy Bypass -File windows/Scripts/package.ps1 [-Config Shipping|Development] [-Out <folder>]
#
# Stages the files the game reads at runtime (the catalogues and the star catalogue) into
# Content/MuseeData, then builds, cooks and packages with Unreal's BuildCookRun.
param(
    [ValidateSet("Shipping", "Development")] [string]$Config = "Shipping",
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8",
    [string]$Out = ""
)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$Repo = Split-Path -Parent $Project
$Builds = Join-Path $Project "Builds"
if (-not $Out) { $Out = Join-Path $Builds ("MuseeVision-" + (Get-Date -Format "yyyyMMdd-HHmm")) }

# The same relative paths as in the repository (see Source/MuseeVision/Catalog/MuseeFiles.h).
$Data = Join-Path $Project "Content\MuseeData"
foreach ($f in @("data\artworks.json", "data\sculptures.json", "data\classical.json", "data\albion.json", "assets\collection.json", "assets\sky\stars.bin")) {
    $to = Join-Path $Data $f
    New-Item -ItemType Directory -Force (Split-Path -Parent $to) | Out-Null
    Copy-Item (Join-Path $Repo $f) $to -Force
}
Write-Host "Staged runtime data in $Data"

& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
    "-project=$Project\MuseeVision.uproject" -noP4 -utf8output `
    -target=MuseeVision -platform=Win64 "-clientconfig=$Config" `
    -build -cook -stage -pak -iostore -prereqs `
    -archive "-archivedirectory=$Out"
if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed ($LASTEXITCODE)" }

$Exe = Join-Path $Out "MuseeVision.exe"
New-Item -ItemType Directory -Force $Builds | Out-Null
Set-Content -Path (Join-Path $Builds "LATEST.txt") -Value $Exe -Encoding utf8
Write-Host "Packaged: $Exe"
