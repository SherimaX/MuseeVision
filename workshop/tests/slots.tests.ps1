<#
slots.tests.ps1: tests unreal.ps1's slots, jobs and job objects with stand-in runs (PowerShell sleeping), never Unreal.

It runs in its own mutex namespace (Global\MuseeUnrealSelfTest_*), its own run folder and a scratch Register, so it
never takes a real slot or writes the real Register. It checks that:
  two HEAVY runs at once serialise; a submitted job survives the process that submitted it; killing a worker
  releases its mutex and ends its run; LIGHT runs two at a time; LIGHT waits for memory; one-Unreal-at-a-time makes
  LIGHT hold HEAVY too; a blocking run ends when its caller ends; children are waited for, and ended after the grace;
  cancel works; slot jobs are recorded in the Register.

  powershell -NoProfile -File workshop\tests\slots.tests.ps1 -Scratch C:\mv\run-selftest\slots
Prints one PASS or FAIL line per check and exits with the number of failures.
#>
param([Parameter(Mandatory)] [string] $Scratch)
$ErrorActionPreference = 'Stop'
$workshop = Split-Path -Parent $PSScriptRoot
Import-Module (Join-Path $workshop 'lib\Workshop.psm1') -Force
$script:Failures = 0
$unreal = Join-Path $workshop 'unreal.ps1'

function Test-That([string] $What, [bool] $Holds, [string] $Detail = '') {
    if ($Holds) { Write-Host "PASS  $What" } else { $script:Failures++; Write-Host "FAIL  $What $Detail" }
}

function Write-SlotTable([string] $Name, [bool] $OneAtATime, [double] $LightGB, [int] $GraceSeconds) {
    $path = Join-Path $Scratch "$Name.json"
    Write-JsonFile $path ([ordered]@{
        slots = [ordered]@{ GPU = @{ mutexes = @('GPU') }; HEAVY = @{ mutexes = @('HEAVY') }; LIGHT = @{ mutexes = @('LIGHT_1', 'LIGHT_2'); minAvailableGB = $LightGB } }
        oneUnrealAtATime = $OneAtATime; childGraceSeconds = $GraceSeconds; sharedServices = @('zenserver.exe') })
    return $path
}

function Invoke-Unreal([string[]] $Arguments) {
    $result = Invoke-Native -FilePath 'powershell.exe' -Arguments (@('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $unreal) + $Arguments)
    $lines = @(Get-GitLines $result.Output)
    return [pscustomobject]@{ Code = $result.ExitCode; Text = ($result.Output + $result.Error); Last = $(if ($lines.Count) { $lines[-1].Trim() } else { '' }) }
}

function Submit-Stand([string] $Slot, [string] $Script, [string] $Label) {
    <# Submits a stand-in run (PowerShell running $Script) and returns its job id. #>
    $submitted = Invoke-Unreal @('submit', '-Slot', $Slot, '-Exe', 'powershell.exe', '-Args', "-NoProfile -Command `"$Script`"", '-Label', $Label, '-Order', 'T-SELFTEST-01')
    if ($submitted.Code -ne 0) { throw "submit failed: $($submitted.Text)" }
    return $submitted.Last
}

function Read-State([string] $Id) { return Read-JsonFile (Join-Path $Scratch "run\jobs\$Id\state.json") }

function Wait-State([string] $Id, [string[]] $States, [int] $Seconds) {
    $until = (Get-Date).AddSeconds($Seconds)
    while ((Get-Date) -lt $until) {
        $path = Join-Path $Scratch "run\jobs\$Id\state.json"
        if ((Test-Path $path) -and (Read-State $Id).state -in $States) { return (Read-State $Id) }
        Start-Sleep -Milliseconds 300
    }
    return $null
}

function Get-Time([string] $Stamp) { return [DateTime]::Parse($Stamp).ToUniversalTime() }

if (Test-Path -LiteralPath $Scratch) { Remove-Item -LiteralPath $Scratch -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Scratch | Out-Null
$env:MUSEE_RUN = Join-Path $Scratch 'run'
$env:MUSEE_SLOT_NAMESPACE = 'MuseeUnrealSelfTest'
$env:MUSEE_REGISTER = Join-Path $Scratch 'register'
Invoke-Native -FilePath (Get-WorkshopSettings).Git -Arguments @('init', '--quiet', '-b', 'register', $env:MUSEE_REGISTER) | Out-Null
Invoke-Native -FilePath 'powershell.exe' -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $workshop 'register.ps1'), 'init') | Out-Null
$env:MUSEE_SLOTS = Write-SlotTable 'parallel' $false 1 5

# Two HEAVY runs at once serialise.
$first = Submit-Stand 'HEAVY' 'Start-Sleep 5' 'heavy one'
$second = Submit-Stand 'HEAVY' 'Start-Sleep 5' 'heavy two'
$a = Wait-State $first @('done') 60; $b = Wait-State $second @('done') 60
$ordered = @($a, $b) | Sort-Object { Get-Time $_.started }
Test-That 'two HEAVY runs submitted together run one after the other' ($a -and $b -and
    (Get-Time $ordered[1].started) -ge (Get-Time $ordered[0].finished) -and $ordered[1].queueSeconds -ge 4) "queue=$($ordered[1].queueSeconds)"

# A submitted job survives the process that submitted it.
$submitter = Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass',
    '-File', "`"$unreal`"", 'submit', '-Slot', 'HEAVY', '-Exe', 'powershell.exe', '-Args', '"-NoProfile -Command Start-Sleep 6"', '-Label', 'survivor')
$submitter.WaitForExit()
$survivor = @(Get-ChildItem (Join-Path $Scratch 'run\jobs') -Directory | Where-Object { (Read-JsonFile "$($_.FullName)\spec.json").label -eq 'survivor' })[0].Name
$running = Wait-State $survivor @('running') 30
$parent = $null
if ($running) {
    $worker = Get-CimInstance Win32_Process -Filter "ProcessId=$($running.workerPid)"
    $parent = (Get-Process -Id $worker.ParentProcessId -ErrorAction SilentlyContinue).ProcessName
}
$waited = Invoke-Unreal @('wait', $survivor, '-TimeoutSeconds', '60')
Test-That 'a submitted job outlives its submitter (worker started by WMI) and finishes' ($submitter.HasExited -and $parent -eq 'WmiPrvSE' -and
    $waited.Code -eq 0 -and (Read-State $survivor).state -eq 'done') "parent=$parent wait=$($waited.Text)"

# Killing a worker releases its mutex and ends its run.
$victim = Submit-Stand 'HEAVY' 'Start-Sleep 120' 'victim'
$running = Wait-State $victim @('running') 30
Start-Sleep -Seconds 1
$runPid = $running.processId
Stop-Process -Id $running.workerPid -Force
Start-Sleep -Milliseconds 1500
$mutex = Enter-NamedMutex -Name 'Global\MuseeUnrealSelfTest_HEAVY' -TimeoutMilliseconds 3000
$runGone = -not (Get-Process -Id $runPid -ErrorAction SilentlyContinue)
Exit-NamedMutex $mutex
$status = Invoke-Unreal @('wait', $victim, '-TimeoutSeconds', '5')
Test-That 'killing a worker releases the HEAVY mutex, ends its run, and wait reports it' ($null -ne $mutex -and $runGone -and $status.Code -eq 126) $status.Text

# LIGHT runs two at a time; the third waits.
$lights = @(1..3 | ForEach-Object { Submit-Stand 'LIGHT' 'Start-Sleep 6' "light $_" })
$states = @($lights | ForEach-Object { Wait-State $_ @('done') 90 }) | Sort-Object { Get-Time $_.started }
$overlap = (Get-Time $states[1].started) -lt (Get-Time $states[0].finished)
$third = (Get-Time $states[2].started) -ge ([DateTime[]]@((Get-Time $states[0].finished), (Get-Time $states[1].finished)) | Sort-Object)[0]
$names = @($states[0].slotsHeld + $states[1].slotsHeld) | Sort-Object
Test-That 'LIGHT runs two at a time on LIGHT_1 and LIGHT_2, and a third waits' ($overlap -and $third -and ($names -join ',') -eq 'LIGHT_1,LIGHT_2') ($names -join ',')

# LIGHT waits while RAM is short.
$env:MUSEE_SLOTS = Write-SlotTable 'hungry' $false 100000 5
$hungry = Invoke-Unreal @('run', '-Slot', 'LIGHT', '-Exe', 'powershell.exe', '-Args', '-NoProfile -Command exit', '-TimeoutMinutes', '0.15')
Test-That 'a LIGHT run is not admitted while available RAM is below the slot''s floor' ($hungry.Code -eq 124 -and $hungry.Text -match 'never got the LIGHT slot') $hungry.Text

# One Unreal at a time: LIGHT also holds HEAVY.
$env:MUSEE_SLOTS = Write-SlotTable 'exclusive' $true 1 5
$heavy = Submit-Stand 'HEAVY' 'Start-Sleep 5' 'exclusive heavy'
[void](Wait-State $heavy @('running') 30)
$light = Submit-Stand 'LIGHT' 'Start-Sleep 1' 'exclusive light'
$h = Wait-State $heavy @('done') 60; $l = Wait-State $light @('done') 60
Test-That 'while one Unreal runs at a time, a LIGHT run waits for HEAVY and holds it' ($h -and $l -and
    (Get-Time $l.started) -ge (Get-Time $h.finished) -and ($l.slotsHeld -contains 'HEAVY')) "held=$($l.slotsHeld -join ',')"
$env:MUSEE_SLOTS = Write-SlotTable 'parallel' $false 1 5

# A blocking run ends when its caller ends.
$caller = Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass',
    '-File', "`"$unreal`"", 'run', '-Slot', 'HEAVY', '-Exe', 'powershell.exe', '-Args', '"-NoProfile -Command Start-Sleep 60"', '-Label', 'bound')
Start-Sleep -Seconds 1
$bound = $null
for ($i = 0; $i -lt 40 -and -not $bound; $i++) {
    $bound = @(Get-ChildItem (Join-Path $Scratch 'run\jobs') -Directory | Where-Object { (Read-JsonFile "$($_.FullName)\spec.json").label -eq 'bound' }) | Select-Object -First 1
    Start-Sleep -Milliseconds 250
}
[void](Wait-State $bound.Name @('running') 30)
Stop-Process -Id $caller.Id -Force
$ended = Wait-State $bound.Name @('cancelled') 15
Test-That 'a blocking run is stopped when its caller ends, and its slot is released' ($ended -and $ended.message -match 'caller ended' -and
    (Test-MutexFree 'Global\MuseeUnrealSelfTest_HEAVY')) "state=$($ended.state)"

# Children are waited for; children still running after the grace are ended.
$spawn = 'Start-Process -WindowStyle Hidden powershell -ArgumentList ''-NoProfile -Command Start-Sleep {0}''; exit 0'
$short = Submit-Stand 'HEAVY' ($spawn -f 4) 'child short'
$s = Wait-State $short @('done') 60
Test-That 'the worker waits for the run''s children before it releases the slot' ($s -and $s.runSeconds -ge 3.5 -and @($s.leftovers).Count -eq 0) "ran=$($s.runSeconds)"
$long = Submit-Stand 'HEAVY' ($spawn -f 120) 'child long'
$lg = Wait-State $long @('done') 60
Test-That 'children still running after the grace period are ended and named' ($lg -and @($lg.leftovers | Where-Object { $_ -match '^powershell' }).Count -eq 1 -and $lg.runSeconds -lt 20) "leftovers=$($lg.leftovers -join ',')"

# Cancel.
$cancelled = Submit-Stand 'HEAVY' 'Start-Sleep 120' 'cancel me'
[void](Wait-State $cancelled @('running') 30)
[void](Invoke-Unreal @('cancel', $cancelled))
$c = Invoke-Unreal @('wait', $cancelled, '-TimeoutSeconds', '20')
Test-That 'cancel stops a running job' ($c.Code -eq 127) $c.Text

# Cancel reaches a job that is still waiting for its slot at once (its wait includes the cancel event).
$blocker = Enter-NamedMutex -Name 'Global\MuseeUnrealSelfTest_HEAVY' -TimeoutMilliseconds 5000
$waiting = Submit-Stand 'HEAVY' 'exit 0' 'queued cancel'
[void](Wait-State $waiting @('queued') 30)
Start-Sleep -Seconds 1
$asked = Get-Date
[void](Invoke-Unreal @('cancel', $waiting))
$q = Wait-State $waiting @('cancelled') 10
$latency = ((Get-Date) - $asked).TotalSeconds
Exit-NamedMutex $blocker
Test-That 'cancel ends a job waiting in the HEAVY queue within seconds' ($q -and $q.message -match 'waiting for its slot' -and $latency -lt 6) "latency=$latency"

# Slot jobs are recorded in the Register.
$records = @(Get-ChildItem (Join-Path $env:MUSEE_REGISTER 'jobs') -Filter '*.json')
$pending = Invoke-Git -Repo $env:MUSEE_REGISTER -Arguments @('status', '--porcelain')
Test-That 'every finished slot job is committed to the Register''s jobs folder' ($records.Count -ge 12 -and -not $pending) "records=$($records.Count)"

$statusText = (Invoke-Unreal @('status')).Text
Test-That 'status names the slots and the last jobs' ($statusText -match 'LIGHT_2\s+free' -and $statusText -match 'Last finished')

Write-Host ("{0} failures." -f $script:Failures)
exit $script:Failures
