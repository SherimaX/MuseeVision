# Look-match: render each target view of LOOK_TARGETS.md from its rendering's camera and put it beside
# the rendering, in windows/Saved/LookMatch/<view>.png (ours on the left, the rendering on the right).
#
#   powershell -ExecutionPolicy Bypass -File windows/Scripts/lookmatch.ps1 [-Views 01,05] [-Exe <game exe>]
#
# Runs the editor build of the game by default; -Exe runs a packaged build instead.
param(
    [string[]]$Views = @(),
    [string]$Exe = "",
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8"
)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$Repo = Split-Path -Parent $Project
$Renderings = Join-Path $Repo "plan\renderings"
$OutDir = Join-Path $Project "Saved\LookMatch"
New-Item -ItemType Directory -Force $OutDir | Out-Null

# View, rendering, pose (plan x, y, feet, yaw, pitch), horizontal FOV, local hour.
$Targets = @(
    @{ Id = "01"; Image = "01-salon-arrival.png";  Pose = "-14.6,0,0,180,3";    Fov = 74; Hour = 15 },
    @{ Id = "02"; Image = "02-modern-paris.png";   Pose = "-36.5,5.5,0,-45,3";  Fov = 74; Hour = 16 },
    @{ Id = "03"; Image = "03-nympheas-oval.png";  Pose = "-96.2,0,0,0,10";     Fov = 84; Hour = 12 },
    @{ Id = "04"; Image = "04-the-reserve-v2.png"; Pose = "-72,0,-5.8,0,3";     Fov = 74; Hour = 20 },
    @{ Id = "05"; Image = "05-the-rotunda-v2.png"; Pose = "-9.2,0,0,0,9";       Fov = 110; Hour = 16 },
    @{ Id = "07"; Image = "07-future-atrium.png";  Pose = "54,11,0,-90,20";     Fov = 84; Hour = 12 }
)
$Views = @($Views | ForEach-Object { $_ -split "," } | Where-Object { $_ })   # "-Views 01,05" arrives as one string under -File
if ($Views.Count -gt 0) { $Targets = $Targets | Where-Object { $Views -contains $_.Id } }

foreach ($t in $Targets) {
    $shot = Join-Path $OutDir ("ours-" + $t.Id + ".png")
    if (Test-Path $shot) { Remove-Item $shot }
    $gameArgs = @("-windowed", "-ResX=1536", "-ResY=1024", "-MuseeShot=$shot", "-MuseePose=$($t.Pose)",
              "-MuseeFov=$($t.Fov)", "-ExecCmds=musee.Hour $($t.Hour)", "-nosplash")
    if ($Exe) { & $Exe @gameArgs | Out-Null }
    else { & "$Engine\Engine\Binaries\Win64\UnrealEditor.exe" "$Project\MuseeVision.uproject" /Game/Maps/Museum -game @gameArgs "-log=LookMatch_$($t.Id).log" | Out-Null }
    if (-not (Test-Path $shot)) { Write-Warning "View $($t.Id): no screenshot"; continue }
    $pair = Join-Path $OutDir ($t.Id + ".png")
    python -c @"
from PIL import Image
a = Image.open(r'$shot').convert('RGB'); b = Image.open(r'$(Join-Path $Renderings $t.Image)').convert('RGB')
b = b.resize(a.size)
out = Image.new('RGB', (a.width * 2 + 16, a.height), (23, 21, 15))
out.paste(a, (0, 0)); out.paste(b, (a.width + 16, 0)); out.save(r'$pair')
"@
    Write-Host "View $($t.Id): $pair"
}
