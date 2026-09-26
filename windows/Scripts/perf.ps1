# Perf: one launch of the game, a window at the display's resolution, that stands at each tour view (tour.ps1's
# poses), lets it settle, then records ~8 s of frame and GPU times (MuseeDo perf steps) to
# windows/Saved/MuseeDo/perf.txt (%LOCALAPPDATA%\MuseeVision\Saved\MuseeDo for a packaged game).
#
#   powershell -ExecutionPolicy Bypass -File windows/Scripts/perf.ps1 [-Views salon,atrium] [-ResX 3840 -ResY 2160] [-Exe <game exe>]
param(
    [string[]]$Views = @("salon", "oval", "reserve", "rotunda", "al-nave", "ch-inner", "classical", "tribune",
                         "light-hall", "atrium", "square", "exterior", "sphere"),
    [int]$ResX = 3840,
    [int]$ResY = 2160,
    [double]$Settle = 6,
    [double]$Measure = 8,
    [string]$Exe = "",
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8"
)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$Views = @($Views | ForEach-Object { $_ -split "," } | Where-Object { $_ })

# The poses: tour.ps1's table.
$poses = @{}
foreach ($m in [regex]::Matches((Get-Content (Join-Path $PSScriptRoot "tour.ps1") -Raw), 'Id = "([^"]+)";\s*Pose = "([^"]+)";\s*Hour = ([\d.]+)')) {
    $poses[$m.Groups[1].Value] = @($m.Groups[2].Value, $m.Groups[3].Value)
}
$steps = @(); $t = 4.0
foreach ($v in $Views) {
    if (-not $poses.ContainsKey($v)) { Write-Warning "no pose for $v"; continue }
    $p = $poses[$v][0] -split ","
    $steps += "cmd:musee.Hour $($poses[$v][1])@$t"
    $steps += "tp:$($p[0]),$($p[1]),$($p[2])@$($t + 0.1)"
    $steps += "look:$($p[3]),$($p[4])@$($t + 0.2)"
    $steps += "perf:-@$($t + $Settle)"
    $steps += "perf:$v@$($t + $Settle + $Measure)"
    $t += $Settle + $Measure + 0.5
}
$saved = if ($Exe) { Join-Path $env:LOCALAPPDATA "MuseeVision\Saved\MuseeDo" } else { Join-Path $Project "Saved\MuseeDo" }
New-Item -ItemType Directory -Force $saved | Out-Null
$out = Join-Path $saved "perf.txt"
Remove-Item $out -ErrorAction SilentlyContinue
$gameArgs = @("-windowed", "-ResX=$ResX", "-ResY=$ResY", "-MuseeDo=`"$($steps -join ';')`"", "-MuseeQuit=$($t + 2)", "-nosplash")
# A packaged game saves the test window's size into the player's own settings: keep theirs and put it back.
$userSettings = Join-Path $env:LOCALAPPDATA "MuseeVision\Saved\Config\Windows\GameUserSettings.ini"
$keep = if ($Exe -and (Test-Path $userSettings)) { Get-Content $userSettings -Raw } else { $null }
if ($Exe) { & $Exe @gameArgs | Out-Null
    if ($null -ne $keep) { Set-Content -Path $userSettings -Value $keep -NoNewline -Encoding utf8 } else { Remove-Item $userSettings -ErrorAction SilentlyContinue } }
else { & "$Engine\Engine\Binaries\Win64\UnrealEditor.exe" "$Project\MuseeVision.uproject" /Game/Maps/Museum -game @gameArgs "-log=Perf.log" | Out-Null }
if (Test-Path $out) { Get-Content $out } else { Write-Warning "no perf.txt" }
