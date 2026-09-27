# Packages Musée Vision into a dated folder under C:\mv\Builds, so a build that is running is never overwritten:
#   desktop: MuseeVision-<yyyyMMdd-HHmm>\   (target MuseeVision, Shipping), named in C:\mv\Builds\LATEST.txt
#   -VR:     MuseeVisionVR-<yyyyMMdd-HHmm>\ (target MuseeVisionVR, DebugGame, OpenXR), named in C:\mv\Builds\LATEST_VR.txt
#
#   powershell -NoProfile -File MuseeVision\Scripts\package.ps1 [-VR]
#
# It builds, cooks (always: a stale cook crashes a map whose classes changed), stages, paks and archives with Unreal's
# BuildCookRun, run through the machine's Unreal lock. The cook takes the museum level and every stage level Studio
# made; the records under Data/ that the museum reads while it runs are copied beside the game. The VR build cooks and
# stages into its own folders, so it never touches the desktop build's.
param(
    [switch] $VR,
    [string] $Engine = "C:\Program Files\Epic Games\UE_5.8",
    [string] $Lock = "C:\mv\bin\with-unreal-lock.ps1",
    [string] $Builds = "C:\mv\Builds",
    [int] $WaitMinutes = 60,
    [int] $RunMinutes = 240
)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$Stamp = Get-Date -Format "yyyyMMdd-HHmm"
if ($VR) {
    $Target = "MuseeVisionVR"; $Config = "DebugGame"; $Latest = "LATEST_VR.txt"
    $Own = "`"-CookOutputDir=$Project\Saved\CookedVR\Windows`" `"-stagingdirectory=$Project\Saved\StagedBuildsVR`""
} else {
    $Target = "MuseeVision"; $Config = "Shipping"; $Latest = "LATEST.txt"; $Own = ""
}
$Out = Join-Path $Builds "$Target-$Stamp"

# The maps: the museum level, and every stage level it places (its Level Instances name them only softly).
$Generated = Join-Path $Project "Content\Generated"
$Museum = Join-Path $Generated "Site\L_Museum.umap"
if (-not (Test-Path $Museum)) { Write-Error "There is no museum level: run Scripts\make.ps1 all first."; exit 2 }
$Maps = @("/Game/Generated/Site/L_Museum")
Get-ChildItem (Join-Path $Generated "Stages") -Recurse -Filter "L_*.umap" -ErrorAction SilentlyContinue | ForEach-Object {
    $Maps += "/Game/Generated/Stages/" + $_.Directory.Name + "/" + $_.BaseName
}

$Arguments = "BuildCookRun `"-project=$Project\MuseeVision.uproject`" -noP4 -utf8output -unattended " +
    "-target=$Target -platform=Win64 -clientconfig=$Config $Own " +
    "-build -cook `"-map=$($Maps -join '+')`" -stage -pak -iostore -prereqs -archive `"-archivedirectory=$Out`""
& $Lock -Exe "$Engine\Engine\Build\BatchFiles\RunUAT.bat" -Args $Arguments -WaitMinutes $WaitMinutes -RunMinutes $RunMinutes
if ($LASTEXITCODE -ne 0) { Write-Error "BuildCookRun failed ($LASTEXITCODE)"; exit $LASTEXITCODE }

# The records the museum reads while it runs (the clock's cities, the weather's normals) live beside the game.
$Game = Get-ChildItem $Out -Directory -Recurse -Filter "MuseeVision" | Where-Object { Test-Path (Join-Path $_.FullName "Content") } | Select-Object -First 1
if (-not $Game) { Write-Error "No game folder in $Out"; exit 1 }
Copy-Item (Join-Path $Project "Data") (Join-Path $Game.FullName "Data") -Recurse -Force

$Exe = Get-ChildItem $Out -Recurse -Filter "$Target*.exe" | Where-Object { $_.DirectoryName -notmatch 'Binaries' } | Select-Object -First 1
if (-not $Exe) { Write-Error "No $Target executable in $Out"; exit 1 }
New-Item -ItemType Directory -Force $Builds | Out-Null
Set-Content -Path (Join-Path $Builds $Latest) -Value $Exe.FullName -Encoding utf8
Write-Host "Packaged: $($Exe.FullName)"
