# Package the VR build of Musée Vision: MuseeVisionVR.exe, the museum with OpenXR for a headset
# through SteamVR (the Vision Pro through ALVR; see VR.md). Its own folder under windows/Builds/
# (MuseeVisionVR-<yyyyMMdd-HHmm>/), named in windows/Builds/LATEST_VR.txt.
#
#   powershell -ExecutionPolicy Bypass -File windows/Scripts/package_vr.ps1 [-SkipCook] [-Out <folder>]
#
# -SkipCook reuses the last cook (Saved/CookedVR) for quick changes to the VR module alone. If any
# class the map saves has changed since that cook (e.g. a UPROPERTY in MuseeVision), the game
# crashes loading the map: cook again.
#
# The VR target is DebugGame (Source/MuseeVisionVR.Target.cs). It cooks and stages into its own
# folders (Saved/CookedVR/Windows, Saved/StagedBuildsVR), so it never touches the desktop package's, and it
# cooks with the editor as it is (-nocompileeditor): nothing cooked needs the VR code, and the
# editor may be open for other work.
param(
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8",
    [string]$Out = "",
    [switch]$SkipCook
)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$Repo = Split-Path -Parent $Project
$Builds = Join-Path $Project "Builds"
if (-not $Out) { $Out = Join-Path $Builds ("MuseeVisionVR-" + (Get-Date -Format "yyyyMMdd-HHmm")) }

# The same relative paths as in the repository (see Source/MuseeVision/Catalog/MuseeFiles.h).
$Data = Join-Path $Project "Content\MuseeData"
foreach ($f in @("data\artworks.json", "data\sculptures.json", "assets\collection.json", "assets\sky\stars.bin")) {
    $to = Join-Path $Data $f
    New-Item -ItemType Directory -Force (Split-Path -Parent $to) | Out-Null
    Copy-Item (Join-Path $Repo $f) $to -Force
}
Write-Host "Staged runtime data in $Data"

& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
    "-project=$Project\MuseeVision.uproject" -noP4 -utf8output `
    -platform=Win64 -target=MuseeVisionVR -clientconfig=DebugGame `
    "-CookOutputDir=$Project\Saved\CookedVR\Windows" "-stagingdirectory=$Project\Saved\StagedBuildsVR" `
    -build -nocompileeditor $(if ($SkipCook) { "-skipcook" } else { "-cook" }) -stage -pak -iostore -prereqs `
    -archive "-archivedirectory=$Out"
if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed ($LASTEXITCODE)" }

$Exe = Get-ChildItem $Out -Filter "MuseeVisionVR*.exe" | Select-Object -First 1
if (-not $Exe) { throw "No MuseeVisionVR executable in $Out" }
New-Item -ItemType Directory -Force $Builds | Out-Null
Set-Content -Path (Join-Path $Builds "LATEST_VR.txt") -Value $Exe.FullName -Encoding utf8

# A fixed path to the newest build for the Steam shortcut in SteamVR's library (VR.md):
# Builds\MuseeVisionVR-Latest is a junction, repointed here. rmdir without /s removes only the link.
$Latest = Join-Path $Builds "MuseeVisionVR-Latest"
if (Test-Path $Latest) { cmd /c rmdir "$Latest" | Out-Null }
cmd /c mklink /J "$Latest" "$Out" | Out-Null
Write-Host "Latest: $Latest\$($Exe.Name)"
Write-Host "Packaged: $($Exe.FullName)"
