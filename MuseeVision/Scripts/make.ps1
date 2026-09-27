# Makes a stage of Musée Vision with the editor closed: the MuseeMake commandlet, run through the machine's Unreal lock
# so only one Unreal process runs at a time.
#
#   powershell -NoProfile -File MuseeVision\Scripts\make.ps1 <stage key | site | all>
#
# <stage key> makes that stage's level (/Game/Generated/Stages/<Stage>/L_<Stage>); site makes the museum level
# (/Game/Generated/Site/L_Museum) from Data/Site/SitePlan.json; all makes every stage of the site plan, then the museum.
# The make report is in Saved/Logs/make-<name>.log. Exit codes: 0 made cleanly; 1 made, with failures or findings; 2 not
# a stage; 3 the lock or the machine never became free; 4 the run timed out.
param(
    [Parameter(Mandatory, Position = 0)] [string] $Stage,
    [string] $Engine = "C:\Program Files\Epic Games\UE_5.8",
    [string] $Lock = "C:\mv\bin\with-unreal-lock.ps1",
    [int] $WaitMinutes = 30,
    [int] $RunMinutes = 60
)
$ErrorActionPreference = "Stop"
if ($Stage -notmatch '^[a-z0-9]+(-[a-z0-9]+)*(\.[a-z0-9]+(-[a-z0-9]+)*)*$') {
    Write-Error "'$Stage' is not a stage's key, site or all."
    exit 2
}
$Project = Split-Path -Parent $PSScriptRoot
$Log = Join-Path $Project "Saved\Logs\make-$Stage.log"
New-Item -ItemType Directory -Force (Split-Path -Parent $Log) | Out-Null

$Arguments = "`"$Project\MuseeVision.uproject`" -run=MuseeMake -Stage=$Stage -unattended -nosplash -nosound -abslog=`"$Log`""
& $Lock -Exe "$Engine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -Args $Arguments -WaitMinutes $WaitMinutes -RunMinutes $RunMinutes
$Code = $LASTEXITCODE
Get-Content $Log -ErrorAction SilentlyContinue | Select-String -Pattern 'LogMuseeStudio' | Select-Object -Last 40 | ForEach-Object { $_.Line }
Write-Host "make $Stage finished: exit $Code (report: $Log)"
exit $Code
