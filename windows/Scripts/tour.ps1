# Tour: one launch of the game that stands at each view in turn and takes a screenshot there, to
# windows/Saved/MuseeDo/<id>.png; then a contact sheet of them all, windows/Saved/MuseeDo/tour.jpg.
#
#   powershell -ExecutionPolicy Bypass -File windows/Scripts/tour.ps1 [-Views salon,atrium] [-Hour 21] [-Exe <game exe>]
#
# Each view: plan x, y, feet, yaw, pitch (as -MuseePose), and an hour (local; -Hour overrides them all).
param(
    [string[]]$Views = @(),
    [double]$Hour = -1,
    [int]$Fov = 84,
    [double]$Settle = 6,
    [string]$Exe = "",
    [string]$Engine = "C:\Program Files\Epic Games\UE_5.8",
    [string[]]$Extra = @()          # more game arguments (e.g. a synchronous shader warm-up)
)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $Project "Saved\MuseeDo"
New-Item -ItemType Directory -Force $OutDir | Out-Null

$Tour = @(
    @{ Id = "salon";        Pose = "-14.6,0,0,180,3";   Hour = 15 },
    @{ Id = "modern-paris"; Pose = "-36.5,5.5,0,-45,3"; Hour = 16 },
    @{ Id = "oval";         Pose = "-96.2,0,0,0,8";     Hour = 12 },
    @{ Id = "oval-back";    Pose = "-86.5,3.6,0,-129,4"; Hour = 12 },
    @{ Id = "reserve";      Pose = "-73.2,0,-5.8,0,4";  Hour = 20 },
    @{ Id = "reserve-rack"; Pose = "-72.4,-3.2,-5.8,2,2"; Hour = 20 },
    @{ Id = "reserve-east"; Pose = "-19.5,-2.2,-5.8,178,6"; Hour = 20 },
    @{ Id = "reserve-easel"; Pose = "-24,0,-5.8,0,4";   Hour = 20 },
    @{ Id = "reserve-aisle"; Pose = "-40.2,-6.0,-5.8,180,6"; Hour = 20 },
    @{ Id = "reserve-backs"; Pose = "-70.2,4.2,-5.8,40,2"; Hour = 20 },
    @{ Id = "reserve-chests"; Pose = "-31,6.2,-5.8,28,-10"; Hour = 20 },
    @{ Id = "reserve-vault"; Pose = "-51.5,0,-5.8,0,38"; Hour = 20 },
    @{ Id = "reserve-night"; Pose = "-73.2,0,-5.8,0,4";  Hour = 23 },
    # Reserve screens: screen C out (its west face, its hardware, its track, its floor slot), screen D drawn out by its
    # prompt (Do: a MuseeDo step run after the look; the screen stays out for the views after), the easel.
    @{ Id = "rv-screen";    Pose = "-73.4,-3.6,-5.8,0,7";  Hour = 20 },
    @{ Id = "rv-hardware";  Pose = "-69.35,-2.9,-5.8,28,36"; Hour = 20 },
    @{ Id = "rv-track";     Pose = "-64.9,-0.6,-5.8,-135,22"; Hour = 20 },
    @{ Id = "rv-slot";      Pose = "-67.6,-7.0,-5.8,124,-52"; Hour = 20 },
    @{ Id = "rv-pulled";    Pose = "-61.2,-3.5,-5.8,180,4"; Hour = 20; Do = "rack.D" },
    @{ Id = "rv-easel";     Pose = "-19.5,0.6,-5.8,-5,2";  Hour = 20 },
    @{ Id = "rotunda";      Pose = "-9.2,0,0,0,9";      Hour = 16 },
    @{ Id = "rotunda-niches"; Pose = "-2.5,2.5,0,-45,14"; Hour = 16 },
    # The pond's edge (pond-edge redesign): the corners from the walk between the coping and the moved benches; the frieze.
    @{ Id = "pond-corner-w"; Pose = "-91.0,1.72,0,-41,-50"; Hour = 12 },
    @{ Id = "pond-corner-e"; Pose = "-82.0,1.72,0,-150,-50"; Hour = 12 },
    @{ Id = "pond-frieze";  Pose = "-81.2,0.45,0,-156,-52"; Hour = 12 },
    @{ Id = "classical";    Pose = "0,-9,-6,-90,2";     Hour = 13 },
    @{ Id = "tribune";      Pose = "-1.5,-33.2,-6,-70,6"; Hour = 13 },
    @{ Id = "busts-west";   Pose = "1.2,-14,-6,180,4";  Hour = 13 },
    @{ Id = "herms";        Pose = "0,-13.5,-6,90,4";   Hour = 13 },
    @{ Id = "tribune-niches"; Pose = "0,-37,-6,-100,4"; Hour = 13 },
    @{ Id = "light-hall";   Pose = "12.5,0,0,0,4";      Hour = 11 },
    @{ Id = "stereo1";      Pose = "17,-0.9,0,-90,-24"; Hour = 11 },
    @{ Id = "stereo2";      Pose = "21.6,2.4,0,0,-18";  Hour = 11 },
    @{ Id = "stereo-top";   Pose = "23,1.45,0,90,-78";  Hour = 11 },
    @{ Id = "stereo-face";  Pose = "23,0.9,0,90,-12";   Hour = 11 },
    @{ Id = "atrium";       Pose = "54,11,0,-90,20";    Hour = 12 },
    @{ Id = "atrium-iris";  Pose = "54,4.6,0,-90,62";   Hour = 12 },
    @{ Id = "atrium-ribs";  Pose = "46,6,0,-30,12";     Hour = 12 },
    @{ Id = "starry";       Pose = "47.4,-0.4,0,-29,4"; Hour = 12 },
    @{ Id = "starry-back";  Pose = "51.9,-2.6,0,160,2";  Hour = 12 },
    # Élan Cube (the Square's views retired): the shaft, the iris, the Cube at rest from the car at its centre (no teleport).
    @{ Id = "cube-shaft";   Pose = "54,0,0,-32,-4";   Hour = 12; NoTp = 1; Cmd = "musee.CarAt -6.57"; Settle = 6 },
    @{ Id = "cube-shaft-up"; Pose = "54,0,0,0,70";    Hour = 12; NoTp = 1; Cmd = "musee.CarAt -11.5"; Settle = 5 },
    @{ Id = "cube-rest";    Pose = "54,0,0,30,12";    Hour = 12; NoTp = 1; Cmd = "musee.CarAt -23.6"; Settle = 8 },
    @{ Id = "cube-iris";    Pose = "54,0,0,0,72";     Hour = 12; NoTp = 1; Cmd = "musee.CarAt -23.6"; Settle = 8 },
    @{ Id = "cube-floor";   Pose = "54,0,0,-60,-50";  Hour = 12; NoTp = 1; Cmd = "musee.CarAt -23.6"; Settle = 8 },
    @{ Id = "exterior";     Pose = "82,26,0,-145,10";   Hour = 17 },
    @{ Id = "sky";          Pose = "80,40,0,-120,55";   Hour = 23 },
    @{ Id = "ext-far";      Pose = "110,55,0,-150,6";   Hour = 17 },
    @{ Id = "ext-south";    Pose = "-44,55,0,-90,4";    Hour = 15 },
    @{ Id = "ext-west";     Pose = "-60,45,0,-60,6";    Hour = 10 },
    @{ Id = "sphere";       Pose = "57.8,0,20.42,180,18"; Hour = 23 },
    @{ Id = "sphere-day";   Pose = "57.8,0,20.42,160,5"; Hour = 12 },
    @{ Id = "sphere-down";  Pose = "57.4,1.9,20.42,207,-48"; Hour = 12 },
    # Albion (the south door): the court from the gilt sun, the nave, the aisles, Morris's bay, the ends, a capital, the
    # porch, the vaults, the outside (musee.AlbionWeather: -1 live London, 0 bright, 1 cloud, 2 rain, 3 fog, 4 snow).
    @{ Id = "al-rotunda";   Pose = "0,0,0,90,4";          Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-porch";     Pose = "0,11.6,0,90,6";       Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-nave";      Pose = "0,18.6,0,90,8";       Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-west";      Pose = "-9.6,19.4,0,118,6";   Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-east";      Pose = "10.2,50.8,0,-98,6";   Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-morris";    Pose = "9.4,23.6,0,-38,-6";   Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-south";     Pose = "0,36.5,0,90,16";      Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-north";     Pose = "0,50.5,0,-90,12";     Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-vault";     Pose = "-2,35,0,90,72";       Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-capital";   Pose = "-4.7,29.2,0,180,66";  Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-floor";     Pose = "1.2,46.8,0,90,-40";   Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-rain";      Pose = "0,30,0,90,55";        Hour = 12; Cmd = "musee.AlbionWeather 2" },
    @{ Id = "al-fog";       Pose = "0,20,0,90,20";        Hour = 12; Cmd = "musee.AlbionWeather 3" },
    @{ Id = "al-ext-se";    Pose = "28,70,0,-131,8";      Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-ext-east";  Pose = "34,35,0,180,12";      Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-book";      Pose = "11.3,20.2,0,0,-42";   Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-case";      Pose = "8.9,41.2,0,0,-38";    Hour = 12; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-cap-eye";   Pose = "-3.2,29.2,0,180,52";  Hour = 12; Cmd = "musee.AlbionWeather 0" },
    # Albion at night: the electroliers in the arcades (AAlbionLamps), the court balanced towards them.
    @{ Id = "al-n-nave";    Pose = "0,18.6,0,90,8";       Hour = 22; Cmd = "musee.AlbionWeather 0" },
    @{ Id = "al-n-east";    Pose = "10.2,50.8,0,-98,6";   Hour = 22; Cmd = "musee.AlbionWeather 0" },
    # Salon interior: each bay from the axis, the oak at grazing light, the lanterns, the case, the willows (the weather
    # set per view: musee.SalonWeather 0 clear, 1 broken, 2 overcast, 3 rain).
    @{ Id = "sa-bay1";      Pose = "-15.2,0,0,180,4";     Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-bay2";      Pose = "-27.9,-1.2,0,160,4";  Hour = 15; Cmd = "musee.SalonWeather 2" },
    @{ Id = "sa-bay3";      Pose = "-39.9,1.2,0,-160,4";  Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-bay4";      Pose = "-51.9,-1.2,0,160,4";  Hour = 15; Cmd = "musee.SalonWeather 1" },
    @{ Id = "sa-bay5";      Pose = "-63.9,1.2,0,-160,4";  Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-back";      Pose = "-73.2,0,0,0,4";       Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-parquet";   Pose = "-41.2,2.3,0,-150,-38"; Hour = 16; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-arch";      Pose = "-29.2,0.6,0,20,38";   Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-vault";     Pose = "-38.1,0,0,180,40";    Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-lantern";   Pose = "-44,-2.2,0,90,62";    Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-lantern-opal"; Pose = "-32,-2.2,0,90,62"; Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-lantern-muslin"; Pose = "-56,-2.2,0,90,62"; Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-rain";      Pose = "-17.5,2.2,0,-120,55"; Hour = 15; Cmd = "musee.SalonWeather 3" },
    @{ Id = "sa-bench";     Pose = "-45.9,4.4,0,-125,-18"; Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-case";      Pose = "-20.1,-0.35,0,-90,-40"; Hour = 15; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-willows";   Pose = "-79.6,0,0,180,4";     Hour = 12; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-willows-north"; Pose = "-86.5,3.2,0,-90,6"; Hour = 12; Cmd = "musee.SalonWeather 1" },
    @{ Id = "sa-pond";      Pose = "-80.8,2.6,0,-160,-22"; Hour = 12; Cmd = "musee.SalonWeather 2" },
    @{ Id = "sa-junction-a"; Pose = "-30.2,-3.4,0,8,52";  Hour = 13; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-junction-b"; Pose = "-59.4,2.8,0,172,55"; Hour = 13; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-junction-c"; Pose = "-17.2,-3.0,0,12,58"; Hour = 13; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-bay1-night"; Pose = "-15.2,0,0,180,4";    Hour = 22; Cmd = "musee.SalonWeather 0" },
    @{ Id = "sa-parquet-top"; Pose = "-44,2.0,0,0,-89";   Hour = 15; Cmd = "musee.SalonWeather 2" },
    @{ Id = "sa-bay1-overcast"; Pose = "-15.2,0,0,180,4"; Hour = 15; Cmd = "musee.SalonWeather 2" },
    # Chenghuai 澄懷 (the north door; the Sculpture Hall's views retired): the loop's stops in order, the garden, and night.
    @{ Id = "ch-porch";       Pose = "0,-9.4,0,-90,5";          Hour = 14 },
    @{ Id = "ch-gate";        Pose = "0,-14.2,0,-90,9";         Hour = 14 },
    @{ Id = "ch-yingbi";      Pose = "0,-21.0,0.9,-90,4";       Hour = 14 },
    @{ Id = "ch-linchi";      Pose = "-4.2,-20.2,0.3,165,-6";   Hour = 14 },
    @{ Id = "ch-frontcourt";  Pose = "-3.2,-25.4,0,180,4";      Hour = 14 },
    @{ Id = "ch-chuihua";     Pose = "-9.2,-23.6,0,-90,9";      Hour = 14 },
    @{ Id = "ch-inner";       Pose = "-9.2,-30.8,0,-90,6";      Hour = 14 },
    @{ Id = "ch-gallery";     Pose = "-15.65,-30.4,0.45,-90,2"; Hour = 14 },
    @{ Id = "ch-tianqing";    Pose = "-16.9,-36.2,0.45,180,-4"; Hour = 14 },
    @{ Id = "ch-hall";        Pose = "-9.2,-46.2,0.75,-90,8";   Hour = 14 },
    @{ Id = "ch-qingbi";      Pose = "-14.0,-49.0,0.75,180,0";   Hour = 14 },
    @{ Id = "ch-changnan";    Pose = "-2.4,-36.2,0.45,0,-4";    Hour = 14 },
    @{ Id = "ch-rear";        Pose = "-9.2,-53.2,0,180,4";      Hour = 14 },
    @{ Id = "ch-shujuan";     Pose = "-0.6,-59.0,0.3,180,-5";   Hour = 14 },
    @{ Id = "ch-moongate";    Pose = "-2.0,-54.3,0,0,4";        Hour = 14 },
    @{ Id = "ch-linquan";     Pose = "10.6,-55.9,0.45,-90,-6";  Hour = 14 },
    @{ Id = "ch-pond";        Pose = "3.9,-47.0,0.15,20,-4";    Hour = 14 },
    @{ Id = "ch-jianshan";    Pose = "10.6,-28.9,2.75,90,4";    Hour = 14 },
    @{ Id = "ch-walk";        Pose = "3.0,-24.0,0.15,-90,3";    Hour = 14 },
    @{ Id = "ch-linchi-case"; Pose = "-12.6,-19.5,0.3,90,-38"; Hour = 14 },
    @{ Id = "ch-shujuan-case"; Pose = "-9.8,-59.4,0.3,-90,-38"; Hour = 14 },
    @{ Id = "ch-tianqing-case"; Pose = "-18.7,-38.2,0.45,180,-18"; Hour = 14 },
    @{ Id = "ch-night-inner"; Pose = "-9.2,-31.0,0,-90,8";      Hour = 21.5 }
)
$Views = @($Views | ForEach-Object { $_ -split "," } | Where-Object { $_ })
if ($Views.Count -gt 0) { $Tour = $Tour | Where-Object { $Views -contains $_.Id } }

$steps = @(); $t = 20.0          # a warm-up first: the ray-tracing pipelines finish compiling in the first seconds
foreach ($v in $Tour) {
    $p = $v.Pose -split ","
    $h = if ($Hour -ge 0) { $Hour } else { $v.Hour }
    $steps += "cmd:musee.Hour $h@$t"
    if ($v.Cmd) { $steps += "cmd:$($v.Cmd)@$t" }   # Salon interior: a view's own console command (its weather)
    if ($v.Do) { $steps += "$($v.Do)@$($t + 0.3)" }   # Reserve screens: a view's own MuseeDo step (a prompt)
    $vs = if ($v.Settle) { $v.Settle } else { $Settle }   # Élan Cube: a view's own settle (a journey's land loads)
    if (-not $v.NoTp) { $steps += "tp:$($p[0]),$($p[1]),$($p[2])@$($t + 0.1)" }   # Élan Cube: in the car, no teleport
    $steps += "look:$($p[3]),$($p[4])@$($t + 0.2)"
    if ($v.NoTp) { $steps += "look:$($p[3]),$($p[4])@$($t + $vs - 0.3)" }
    $steps += "shot:$($v.Id)@$($t + $vs)"
    Remove-Item (Join-Path $OutDir "$($v.Id).png") -ErrorAction SilentlyContinue
    $t += $vs + 1.5
}
$do = $steps -join ";"
$gameArgs = @("-unattended", "-windowed", "-ResX=1600", "-ResY=900", "-MuseeFov=$Fov", "-MuseeDo=`"$do`"", "-MuseeQuit=$($t + 2)", "-nosplash") + $Extra
# A packaged game saves the test window's size into the player's own settings: keep theirs and put it back.
$userSettings = Join-Path $env:LOCALAPPDATA "MuseeVision\Saved\Config\Windows\GameUserSettings.ini"
$keep = if ($Exe -and (Test-Path $userSettings)) { Get-Content $userSettings -Raw } else { $null }
if ($Exe) { & $Exe @gameArgs | Out-Null
    if ($null -ne $keep) { Set-Content -Path $userSettings -Value $keep -NoNewline -Encoding utf8 } else { Remove-Item $userSettings -ErrorAction SilentlyContinue } }
else { & "$Engine\Engine\Binaries\Win64\UnrealEditor.exe" "$Project\MuseeVision.uproject" /Game/Maps/Museum -game @gameArgs "-log=Tour.log" | Out-Null }

# A packaged game writes to %LOCALAPPDATA%\MuseeVision\Saved; gather its shots here.
if ($Exe) {
    foreach ($v in $Tour) {
        $from = Join-Path $env:LOCALAPPDATA "MuseeVision\Saved\MuseeDo\$($v.Id).png"
        if (Test-Path $from) { Move-Item $from (Join-Path $OutDir "$($v.Id).png") -Force }
    }
}
$ids = ($Tour | ForEach-Object { $_.Id }) -join ","
python -c @"
from PIL import Image, ImageDraw
import os
ids = '$ids'.split(','); d = r'$OutDir'
shots = [(i, Image.open(os.path.join(d, i + '.png')).convert('RGB')) for i in ids if os.path.exists(os.path.join(d, i + '.png'))]
if shots:
    w, h = 800, 450; cols = 2; rows = (len(shots) + cols - 1) // cols
    sheet = Image.new('RGB', (cols * w, rows * h), (20, 20, 20))
    for k, (i, im) in enumerate(shots):
        im = im.resize((w, h)); ImageDraw.Draw(im).text((8, 6), i, fill=(255, 255, 0))
        sheet.paste(im, ((k % cols) * w, (k // cols) * h))
    sheet.save(os.path.join(d, 'tour.jpg'), quality=88)
print('shots:', len(shots), 'of', len(ids))
"@
