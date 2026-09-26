<#
unreal.ps1: runs Unreal, and UnrealBuildTool, by slot, machine-wide.

What really conflicts across this PC is RAM, the GPU and the shared plugin build folder, so Unreal runs take slots
(BUILD_PLAN 7.4). Each slot is a Windows named mutex held by the worker process that launches the run:
  GPU    (1)  Global\MuseeUnreal_GPU      measurements and renders; admitted only while the GPU is idle
  HEAVY  (1)  Global\MuseeUnreal_HEAVY    generation, cooking, packaging and every game-target build
  LIGHT  (2)  Global\MuseeUnreal_LIGHT_1 or _2, whichever is free; admitted only while 40 GB of RAM is available
  NONE        no slot (editor builds and JsonExport queue on UnrealBuildTool's own mutex instead)
The table lives in slots.json. While its "oneUnrealAtATime" is true, GPU and LIGHT runs also hold HEAVY, which the
bootstrap lock C:\mv\bin\with-unreal-lock.ps1 uses too, so the two cooperate.

Every run is carried out by a worker that is started outside the caller's process tree, so a submitted job survives
the end of the call that submitted it. The worker puts the command inside a Windows job object: Unreal dies with its
worker, the worker waits for Unreal's ShaderCompileWorkers, and shared services (zenserver) are never killed.

  unreal.ps1 run    -Slot HEAVY -Exe <path> -Args "<arguments>" [-TimeoutMinutes 8] [-Order <id>] [-Label <text>] [-Log <path>]
                    Blocks until the run ends; the minutes cover the wait for the slot and the run, so keep them under
                    9 from an agent's tool call. Prints the exit code and the log path, and exits with the run's code.
  unreal.ps1 submit -Slot LIGHT -Exe <path> -Args "<arguments>" [-TimeoutMinutes 120] [-QueueHours 12] [...]
                    Returns a job id at once. The job record (queue wait, run time) goes into the Register's jobs folder.
  unreal.ps1 wait <job id> [-TimeoutSeconds 540]
                    Waits within one tool call's limit; exit code 259 means the job is still going (call wait again).
  unreal.ps1 status Shows who holds each slot, the jobs waiting and running, available RAM and the GPU's load.
  unreal.ps1 cancel <job id>
  unreal.ps1 build  -Slot <worktree> -Target Editor|Game|VR [-Log <path>] [-Submit] [-TimeoutMinutes 8]
                    UnrealBuildTool through Build.bat with -WaitMutex. Game and VR take the HEAVY slot, because they
                    compile the shared plugin folder. VR builds DebugGame (Decision D-010); the others Development.
  unreal.ps1 export -Slot <worktree> -Target Editor|Game|VR -OutputFile <path> [-Log <path>]
                    UnrealBuildTool -Mode=JsonExport: the target's resolved module graph, for verify's wiring step.

Exit codes of run and wait: the run's own exit code when it ran; 124 when it ran past its limit or never got its slot
in time; 125 when it couldn't start; 126 when its worker died; 127 when it was cancelled; 259 while still going.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateSet('run', 'submit', 'wait', 'status', 'cancel', 'build', 'export', 'work')] [string] $Command,
    [Parameter(Position = 1)] [string] $JobId,
    [string] $Slot,
    [string] $Exe,
    [Alias('Args')] [string] $Arguments = '',
    [double] $TimeoutMinutes = 0,
    [double] $QueueHours = 12,
    [int] $TimeoutSeconds = 540,
    [string] $Order = '',
    [string] $Label = '',
    [string] $Log = '',
    [string] $WorkDir = '',
    [ValidateSet('Editor', 'Game', 'VR')] [string] $Target = 'Editor',
    [string] $OutputFile = '',
    [int] $Tail = 0,
    [switch] $Submit,
    [string] $RunRoot = ''
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
$settings = Get-WorkshopSettings
if ($RunRoot) { $settings.RunRoot = $RunRoot }
$jobsRoot = Join-Path $settings.RunRoot 'jobs'
$holdersRoot = Join-Path $settings.RunRoot 'slots'
$finalStates = @('done', 'timed-out', 'queue-timeout', 'cancelled', 'failed-to-start', 'worker-died')

function Use-ProcessJobType([string] $Workshop) {
    if (-not ('MuseeWorkshop.ProcessJob' -as [type])) { Add-Type -Path (Join-Path $Workshop 'lib\ProcessJob.cs') }
}

# ---- Jobs on disk: C:\mv\run\jobs\<id>\ holds spec.json (what to run), launcher.json (the worker's process id),
# ---- state.json (written only by the worker) and, when asked, a cancel file.

function Get-JobFolder([string] $Id) { return Join-Path $jobsRoot $Id }

function Read-JobState([string] $Id) {
    $path = Join-Path (Get-JobFolder $Id) 'state.json'
    if (Test-Path -LiteralPath $path) { return Read-JsonFile $path }
    if (-not (Test-Path -LiteralPath (Get-JobFolder $Id))) { throw "There is no job $Id in $jobsRoot." }
    return [pscustomobject]@{ id = $Id; state = 'starting'; workerPid = 0; message = 'its worker is starting' }
}

function Write-JobState([string] $Id, $State) { Write-JsonFile (Join-Path (Get-JobFolder $Id) 'state.json') $State }

function Read-JobSpec([string] $Id) { return Read-JsonFile (Join-Path (Get-JobFolder $Id) 'spec.json') }

function Read-SlotTable([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "The slot table $Path is missing." }
    return Read-JsonFile $Path
}

function Format-Duration($Seconds) {
    $span = [TimeSpan]::FromSeconds([Math]::Round([double]$Seconds))
    if ($span.TotalHours -ge 1) { return '{0}h {1}m' -f [int][Math]::Floor($span.TotalHours), $span.Minutes }
    if ($span.TotalMinutes -ge 1) { return '{0}m {1}s' -f $span.Minutes, $span.Seconds }
    return '{0}s' -f $span.Seconds
}

function Resolve-Program([string] $Program) {
    if (-not $Program) { throw '-Exe is required: the program to run.' }
    if (Test-Path -LiteralPath $Program) { return (Resolve-Path -LiteralPath $Program).Path }
    $found = Get-Command $Program -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    throw "There is no program $Program."
}

function Add-JobSpec {
    <# Writes a job's spec and returns it. A blocking run carries a deadline and its caller, so it ends with them. #>
    param([string] $SlotName, [string] $Program, [string] $CommandArguments, [double] $RunMinutes, [bool] $Blocking,
          [string] $LogPath, [string] $JobLabel, [int] $TailLines)
    if ($SlotName -notin @('GPU', 'HEAVY', 'LIGHT', 'NONE')) { throw "-Slot must be GPU, HEAVY, LIGHT or NONE (got '$SlotName')." }
    $Program = Resolve-Program $Program
    $random = -join ((1..4) | ForEach-Object { '{0:x}' -f (Get-Random -Maximum 16) })
    $id = '{0:yyyyMMdd-HHmmss}-{1}-{2}' -f (Get-Date), $SlotName.ToLowerInvariant(), $random
    $folder = Get-JobFolder $id
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    if (-not $LogPath) { $LogPath = Join-Path $folder 'output.log' }
    $caller = Get-Process -Id $PID
    $now = [DateTime]::UtcNow
    $spec = [ordered]@{
        id = $id; slot = $SlotName; exe = $Program; args = $CommandArguments
        workDir = $(if ($WorkDir) { $WorkDir } else { (Get-Location).ProviderPath })
        runMinutes = $RunMinutes; queueHours = $QueueHours
        deadline = $(if ($Blocking) { $now.AddMinutes($RunMinutes).ToString('o') } else { $null })
        callerPid = $(if ($Blocking) { $PID } else { 0 })
        callerStart = $(if ($Blocking) { $caller.StartTime.ToUniversalTime().ToString('o') } else { $null })
        order = $Order; label = $(if ($JobLabel) { $JobLabel } else { [IO.Path]::GetFileName($Program) })
        log = $LogPath; tail = $TailLines; submitted = $now.ToString('o')
        slotTable = $settings.SlotTable; namespace = $settings.Namespace
        register = $settings.Register; runRoot = $settings.RunRoot; workshop = $PSScriptRoot
        environment = Get-MuseeEnvironment
    }
    Write-JsonFile (Join-Path $folder 'spec.json') $spec
    return [pscustomobject]$spec
}

function Get-MuseeEnvironment {
    <# The MUSEE_* variables of the submitter, which the worker (started by WMI with the user's plain environment)
       sets again, so the job and every tool it calls see the same places. #>
    $found = [ordered]@{}
    foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
        if ($entry.Key -like 'MUSEE_*') { $found[$entry.Key] = $entry.Value }
    }
    return $found
}

function Start-JobWorker($Spec) {
    <# Starts the worker through WMI, so it belongs to no caller's process tree or job and outlives the caller. #>
    $powershell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    $line = '"{0}" -NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File "{1}" work {2} -RunRoot "{3}"' -f `
        $powershell, $PSCommandPath, $Spec.id, $settings.RunRoot
    $startup = New-CimInstance -ClassName Win32_ProcessStartup -ClientOnly -Property @{ ShowWindow = [uint16]0 }
    $created = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
        CommandLine = $line; CurrentDirectory = $settings.RunRoot; ProcessStartupInformation = $startup
    }
    if ($created.ReturnValue -ne 0) { throw "Windows refused to start the job worker (Win32_Process.Create returned $($created.ReturnValue))." }
    Write-JsonFile (Join-Path (Get-JobFolder $Spec.id) 'launcher.json') ([ordered]@{ workerPid = [int]$created.ProcessId })
}

# ---- The worker: waits for the slot, runs the command in a job object, records the result ------------------------

function Get-MutexName([string] $Namespace, [string] $Name) { return "Global\${Namespace}_$Name" }

function Test-CallerAlive($Spec) {
    if (-not $Spec.callerPid) { return $true }
    try { $process = Get-Process -Id $Spec.callerPid -ErrorAction Stop } catch { return $false }
    $started = [DateTime]::Parse($Spec.callerStart).ToUniversalTime()
    return ([Math]::Abs(($process.StartTime.ToUniversalTime() - $started).TotalSeconds) -lt 2)
}

function Get-StopReason($Spec, [DateTime] $Deadline) {
    <# Why the worker must stop now, or '' to carry on: cancelled, caller gone, or past its deadline. #>
    if (Test-Path -LiteralPath (Join-Path (Get-JobFolder $Spec.id) 'cancel')) { return 'cancelled' }
    if (-not (Test-CallerAlive $Spec)) { return 'caller-gone' }
    if ([DateTime]::UtcNow -gt $Deadline) { return 'deadline' }
    return ''
}

function Get-GpuLoad {
    <# The GPU's utilisation in percent, the highest of three readings a second apart, or $null without nvidia-smi. #>
    $smi = Get-Command nvidia-smi.exe -ErrorAction SilentlyContinue
    if (-not $smi) { return $null }
    $highest = 0
    for ($i = 0; $i -lt 3; $i++) {
        $reading = Invoke-Native -FilePath $smi.Source -Arguments @('--query-gpu=utilization.gpu', '--format=csv,noheader,nounits')
        foreach ($value in (Get-GitLines $reading.Output)) { $highest = [Math]::Max($highest, [int]$value.Trim()) }
        if ($i -lt 2) { Start-Sleep -Seconds 1 }
    }
    return $highest
}

function Get-AdmissionWait($SlotRules) {
    <# Why the slot can't admit a run right now (RAM or GPU), or '' when it can. #>
    if ($SlotRules.minAvailableGB) {
        $available = [MuseeWorkshop.ProcessJob]::AvailableMemoryBytes() / 1GB
        if ($available -lt $SlotRules.minAvailableGB) {
            return ('memory: {0:N1} GB available, the slot admits above {1} GB' -f $available, $SlotRules.minAvailableGB)
        }
    }
    if ($SlotRules.gpuIdleBelowPercent) {
        $busy = Get-GpuLoad
        if ($null -ne $busy -and $busy -ge $SlotRules.gpuIdleBelowPercent) {
            return "the GPU: it is $busy% busy, and the owner's play and VR come first"
        }
    }
    return ''
}

function Enter-OneOf($Mutexes, [int] $Milliseconds) {
    <# Waits up to the time for any one of the mutexes; returns the index taken, or -1. #>
    try {
        $index = [System.Threading.WaitHandle]::WaitAny([System.Threading.WaitHandle[]]$Mutexes, $Milliseconds)
        if ($index -eq [System.Threading.WaitHandle]::WaitTimeout) { return -1 }
        return $index
    } catch [System.Threading.AbandonedMutexException] {
        return $_.Exception.MutexIndex
    }
}

function Wait-ForSlot($Spec, $Table, $State, [DateTime] $Deadline, [System.Collections.ArrayList] $Held) {
    <# Takes the job's slot, and HEAVY too while one Unreal runs at a time, adding each mutex to $Held.
       Returns $true when held, or $false with $State saying why it gave up. #>
    if ($Spec.slot -eq 'NONE') { return $true }
    $rules = $Table.slots.($Spec.slot)
    $names = @($rules.mutexes | ForEach-Object { $_ })
    $own = @($names | ForEach-Object { New-Object System.Threading.Mutex($false, (Get-MutexName $Spec.namespace $_)) })
    $heavy = New-Object System.Threading.Mutex($false, (Get-MutexName $Spec.namespace 'HEAVY'))
    $alsoHeavy = $Table.oneUnrealAtATime -and $Spec.slot -ne 'HEAVY'
    $reason = ''
    while (-not ($reason = Get-StopReason $Spec $Deadline)) {
        $admission = Get-AdmissionWait $rules
        if ($admission) { $State.message = "waiting for $admission"; Write-JobState $Spec.id $State; Start-Sleep -Seconds 2; continue }
        $index = Enter-OneOf $own 1000
        if ($index -lt 0) { continue }
        [void]$Held.Add([pscustomobject]@{ Mutex = $own[$index]; Name = $names[$index] })
        if (-not $alsoHeavy) { return $true }
        while (-not ($reason = Get-StopReason $Spec $Deadline)) {
            if ((Enter-OneOf @($heavy) 1000) -ge 0) { [void]$Held.Add([pscustomobject]@{ Mutex = $heavy; Name = 'HEAVY' }); return $true }
        }
        Exit-NamedMutex $Held[0].Mutex
        $Held.Clear()
        break
    }
    $State.state = @{ 'cancelled' = 'cancelled'; 'caller-gone' = 'cancelled'; 'deadline' = 'queue-timeout' }[$reason]
    $State.message = @{ 'cancelled' = 'cancelled while waiting for its slot'; 'caller-gone' = 'its caller ended while it waited for its slot'
        'deadline' = "never got the $($Spec.slot) slot before its time ran out" }[$reason]
    return $false
}

function Set-SlotHolders($Spec, $Held, [bool] $Holding) {
    <# Notes (or clears) who holds each slot, for status. The mutex itself is the truth; this is only its label. #>
    New-Item -ItemType Directory -Force -Path $holdersRoot | Out-Null
    foreach ($slot in $Held) {
        $path = Join-Path $holdersRoot ("{0}_{1}.json" -f $Spec.namespace, $slot.Name)
        if ($Holding) {
            Write-JsonFile $path ([ordered]@{ job = $Spec.id; pid = $PID; label = $Spec.label; order = $Spec.order; since = (Get-UtcStamp) })
        } elseif (Test-Path -LiteralPath $path) {
            if ((Read-JsonFile $path).job -eq $Spec.id) { Remove-Item -LiteralPath $path -Force }
        }
    }
}

function Get-CommandLine($Spec) {
    $extension = [IO.Path]::GetExtension($Spec.exe).ToLowerInvariant()
    if ($extension -in @('.bat', '.cmd')) {
        $shell = Join-Path $env:SystemRoot 'System32\cmd.exe'
        return ('"{0}" /d /s /c ""{1}" {2}"' -f $shell, $Spec.exe, $Spec.args)
    }
    return ('"{0}" {1}' -f $Spec.exe, $Spec.args)
}

function Watch-InJob($Job, $Spec, $Table, $State, [DateTime] $Deadline) {
    <# Supervises a started run until it ends or must stop, then waits for its children (Unreal's ShaderCompileWorkers)
       and ends any still running after the grace period. Shared services are never ended. #>
    $shared = [string[]]@($Table.sharedServices | ForEach-Object { $_ })
    $reason = ''
    while (-not $Job.WaitForMain(1000)) {
        $reason = Get-StopReason $Spec $Deadline
        if ($reason) { [void]$Job.TerminateExcept($shared); [void]$Job.WaitForMain(10000); break }
    }
    $State.exitCode = $Job.MainExitCode
    $State.state = @{ '' = 'done'; 'cancelled' = 'cancelled'; 'caller-gone' = 'cancelled'; 'deadline' = 'timed-out' }[$reason]
    $State.message = @{ '' = ''; 'cancelled' = 'cancelled while running'; 'caller-gone' = 'its caller ended, so the run was stopped'
        'deadline' = "stopped: it ran past its limit of $($Spec.runMinutes) min" }[$reason]
    $graceEnds = [DateTime]::UtcNow.AddSeconds([double]$Table.childGraceSeconds)
    while ($Job.Describe($shared).Length -gt 0 -and [DateTime]::UtcNow -lt $graceEnds) { Start-Sleep -Seconds 1 }
    $leftovers = $Job.Describe($shared)
    if ($leftovers.Length -gt 0) { [void]$Job.TerminateExcept($shared); $State.leftovers = $leftovers }
    if ($Job.HoldsShared($shared)) { $Job.KeepSharedServices(); $State.sharedServiceKept = $true }
}

function Invoke-InJob($Spec, $Table, $State, [DateTime] $Deadline) {
    $job = New-Object MuseeWorkshop.ProcessJob
    try {
        try { $job.Start((Get-CommandLine $Spec), $Spec.workDir, $Spec.log) }
        catch { $State.state = 'failed-to-start'; $State.message = "could not start: $($_.Exception.Message)"; return }
        $State.processId = $job.ProcessId
        Write-JobState $Spec.id $State
        Watch-InJob $job $Spec $Table $State $Deadline
    } finally { $job.Dispose() }
}

function Save-JobRecord($Spec, $State) {
    <# Commits the finished job's record into the Register's jobs folder (slot runs only; plain builds aren't jobs). #>
    if ($Spec.slot -eq 'NONE') { return }
    if (-not (Test-Path -LiteralPath (Join-Path $Spec.register '.git'))) { $State.message += ' (no Register to record into)'; return }
    $record = [ordered]@{
        id = $Spec.id; order = $Spec.order; label = $Spec.label; slot = $Spec.slot; slotsHeld = $State.slotsHeld
        exe = [IO.Path]::GetFileName($Spec.exe); args = $Spec.args; outcome = $State.state; exitCode = $State.exitCode
        submitted = $Spec.submitted; started = $State.started; finished = $State.finished
        queueSeconds = $State.queueSeconds; runSeconds = $State.runSeconds; message = $State.message; log = $Spec.log
    }
    $path = Join-Path (Get-JobFolder $Spec.id) 'record.json'
    Write-JsonFile $path $record
    [Environment]::SetEnvironmentVariable('MUSEE_REGISTER', $Spec.register)
    try { & (Join-Path $Spec.workshop 'register.ps1') job -FromFile $path 6>$null | Out-Null }
    catch { $State.message += " (the Register refused the record: $($_.Exception.Message))" }
}

function Invoke-JobWorker([string] $Id) {
    $spec = Read-JobSpec $Id
    if ($spec.environment) {
        foreach ($property in $spec.environment.PSObject.Properties) { [Environment]::SetEnvironmentVariable($property.Name, [string]$property.Value) }
    }
    $table = Read-SlotTable $spec.slotTable
    Use-ProcessJobType $spec.workshop
    $submitted = [DateTime]::Parse($spec.submitted).ToUniversalTime()
    $state = [ordered]@{ id = $Id; state = 'queued'; workerPid = $PID; message = ''; slotsHeld = @(); processId = 0
        exitCode = $null; started = $null; finished = $null; queueSeconds = 0; runSeconds = 0; leftovers = @() }
    Write-JobState $Id $state
    if ($spec.deadline) { $deadline = [DateTime]::Parse($spec.deadline).ToUniversalTime() }
    else { $deadline = $submitted.AddHours([double]$spec.queueHours) }
    $held = New-Object System.Collections.ArrayList
    try {
        if (Wait-ForSlot $spec $table $state $deadline $held) {
            $state.slotsHeld = @($held | ForEach-Object { $_.Name })
            Set-SlotHolders $spec $held $true
            $state.started = Get-UtcStamp
            $state.queueSeconds = [Math]::Round(([DateTime]::UtcNow - $submitted).TotalSeconds, 1)
            $state.state = 'running'; $state.message = ''
            Write-JobState $Id $state
            if (-not $spec.deadline) { $deadline = [DateTime]::UtcNow.AddMinutes([double]$spec.runMinutes) }
            $runStart = [DateTime]::UtcNow
            Invoke-InJob $spec $table $state $deadline
            $state.runSeconds = [Math]::Round(([DateTime]::UtcNow - $runStart).TotalSeconds, 1)
        }
    } finally {
        Set-SlotHolders $spec $held $false
        for ($i = $held.Count - 1; $i -ge 0; $i--) { Exit-NamedMutex $held[$i].Mutex }
        $state.finished = Get-UtcStamp
        Save-JobRecord $spec $state
        Write-JobState $Id $state
    }
}

# ---- The caller's side: run, submit, wait, status, cancel ------------------------------------------------------

function Test-WorkerAlive([string] $Id, $State) {
    $workerPid = [int]$State.workerPid
    if (-not $workerPid) {
        $launcher = Join-Path (Get-JobFolder $Id) 'launcher.json'
        if (-not (Test-Path -LiteralPath $launcher)) { return $true }
        $workerPid = [int](Read-JsonFile $launcher).workerPid
    }
    $worker = Get-Process -Id $workerPid -ErrorAction SilentlyContinue
    return [bool]($worker -and $worker.ProcessName -eq 'powershell')
}

function Get-ExitCodeFor($State) {
    switch ($State.state) {
        'done' { return [int]$State.exitCode }
        'timed-out' { return 124 }
        'queue-timeout' { return 124 }
        'failed-to-start' { return 125 }
        'worker-died' { return 126 }
        'cancelled' { return 127 }
    }
    return 259
}

function Write-JobResult([string] $Id) {
    <# Prints a finished job's outcome in plain words and returns its exit code. #>
    $state = Read-JobState $Id
    $spec = Read-JobSpec $Id
    $what = "$($spec.slot) job $Id ($($spec.label))"
    $waited = 'waited {0} for the slot' -f (Format-Duration $state.queueSeconds)
    switch ($state.state) {
        'done' { Write-Host ("{0} finished with exit code {1} after {2} of running ({3})." -f $what, $state.exitCode, (Format-Duration $state.runSeconds), $waited) }
        'worker-died' { Write-Host "$what stopped: its worker process died, so Windows released its slot and ended the run." }
        default { Write-Host ("{0} {1}: {2} ({3})." -f $what, $state.state, $state.message, $waited) }
    }
    if (@($state.leftovers).Count -gt 0 -and $state.leftovers) {
        Write-Host ('These children were still running after the grace period and were ended: ' + (@($state.leftovers) -join ', '))
    }
    if ($state.sharedServiceKept) { Write-Host 'A shared service had landed inside the job; it was left running.' }
    Write-Host "Log: $($spec.log)"
    if ($spec.tail -gt 0 -and (Test-Path -LiteralPath $spec.log)) {
        Get-Content -LiteralPath $spec.log -Tail $spec.tail | ForEach-Object { Write-Host $_ }
    }
    return (Get-ExitCodeFor $state)
}

function Wait-UnrealJob([string] $Id, [int] $Seconds) {
    <# Polls a job until it ends or the seconds run out. Returns its exit code, or 259 while it is still going. #>
    $until = [DateTime]::UtcNow.AddSeconds($Seconds)
    while ($true) {
        $state = Read-JobState $Id
        if ($state.state -in $finalStates) { return (Write-JobResult $Id) }
        if (-not (Test-WorkerAlive $Id $state)) {
            Start-Sleep -Seconds 1
            $state = Read-JobState $Id
            if ($state.state -notin $finalStates) {
                $state | Add-Member -NotePropertyName state -NotePropertyValue 'worker-died' -Force
                Write-JobState $Id $state
            }
            return (Write-JobResult $Id)
        }
        if ([DateTime]::UtcNow -gt $until) {
            $since = [DateTime]::Parse((Read-JobSpec $Id).submitted).ToUniversalTime()
            Write-Host ("Job $Id is still {0} after {1} ({2}). Call wait again." -f $state.state, (Format-Duration ([DateTime]::UtcNow - $since).TotalSeconds), $state.message)
            return 259
        }
        Start-Sleep -Milliseconds 1000
    }
}

function Show-SlotStatus {
    $table = Read-SlotTable $settings.SlotTable
    Use-ProcessJobType $PSScriptRoot
    Write-Host "Slots (namespace $($settings.Namespace)):"
    foreach ($slotName in @('GPU', 'HEAVY', 'LIGHT')) {
        foreach ($mutexName in @($table.slots.$slotName.mutexes)) {
            if (Test-MutexFree (Get-MutexName $settings.Namespace $mutexName)) { Write-Host ("  {0,-8} free" -f $mutexName); continue }
            $holderPath = Join-Path $holdersRoot ("{0}_{1}.json" -f $settings.Namespace, $mutexName)
            $holder = $null
            if (Test-Path -LiteralPath $holderPath) { $holder = Read-JsonFile $holderPath }
            if ($holder -and (Get-Process -Id $holder.pid -ErrorAction SilentlyContinue)) {
                Write-Host ("  {0,-8} busy: job {1} ({2}) since {3}" -f $mutexName, $holder.job, $holder.label, $holder.since)
            } else { Write-Host ("  {0,-8} busy: held outside the workshop (for example by with-unreal-lock.ps1)" -f $mutexName) }
        }
    }
    $available = [MuseeWorkshop.ProcessJob]::AvailableMemoryBytes() / 1GB
    $load = Get-GpuLoad
    $gpu = $(if ($null -eq $load) { 'no nvidia-smi' } else { "$load% busy" })
    Write-Host ('Memory: {0:N1} GB available (LIGHT admits above {1} GB). GPU: {2}.' -f $available, $table.slots.LIGHT.minAvailableGB, $gpu)
    Write-Host ('One Unreal at a time: {0}.' -f $(if ($table.oneUnrealAtATime) { 'yes, so GPU and LIGHT runs also hold HEAVY' } else { 'no' }))
    Show-JobList
}

function Show-JobList {
    if (-not (Test-Path -LiteralPath $jobsRoot)) { Write-Host 'No jobs yet.'; return }
    $open = @(); $closed = @()
    foreach ($folder in @(Get-ChildItem -LiteralPath $jobsRoot -Directory | Sort-Object Name)) {
        $state = Read-JobState $folder.Name
        if ($state.state -in $finalStates) { $closed += $state; continue }
        if (-not (Test-WorkerAlive $folder.Name $state)) { $closed += $state; continue }
        $open += $state
    }
    Write-Host ("Jobs waiting or running: {0}" -f $open.Count)
    foreach ($state in $open) { Write-Host ("  {0,-8} {1}  {2}" -f $state.state, $state.id, $state.message) }
    $recent = @($closed | Select-Object -Last 5)
    if ($recent.Count) { Write-Host 'Last finished:' }
    foreach ($state in $recent) {
        Write-Host ("  {0,-14} {1}  exit {2}, waited {3}, ran {4}" -f $state.state, $state.id, $state.exitCode, (Format-Duration $state.queueSeconds), (Format-Duration $state.runSeconds))
    }
}

# ---- UnrealBuildTool: builds and JSON exports of a worktree's targets --------------------------------------------

function Get-TargetRules([string] $Name) {
    switch ($Name) {
        'Editor' { return [pscustomobject]@{ Target = 'MuseeVisionEditor'; Configuration = 'Development'; Slot = 'NONE' } }
        'Game' { return [pscustomobject]@{ Target = 'MuseeVision'; Configuration = 'Development'; Slot = 'HEAVY' } }
        'VR' { return [pscustomobject]@{ Target = 'MuseeVisionVR'; Configuration = 'DebugGame'; Slot = 'HEAVY' } }
    }
}

function Get-ProjectFile([string] $Worktree) {
    if (-not $Worktree) { throw '-Slot is required: the worktree to build, for example C:\mv\wt\f-form.' }
    $project = Join-Path $Worktree "$($settings.Project)\$($settings.Project).uproject"
    if (-not (Test-Path -LiteralPath $project)) { throw "There is no $($settings.Project).uproject in $Worktree." }
    return (Resolve-Path -LiteralPath $project).Path
}

function Write-BuildSummary([string] $LogPath) {
    <# The first ten error lines of an UnrealBuildTool log and its result line, so compile output never floods. #>
    if (-not (Test-Path -LiteralPath $LogPath)) { return }
    $lines = Get-Content -LiteralPath $LogPath
    $errors = Get-BuildErrors $LogPath
    if ($errors.Count) { Write-Host ("{0} error lines; the first {1}:" -f $errors.Count, [Math]::Min(10, $errors.Count)) }
    $errors | Select-Object -First 10 | ForEach-Object { Write-Host "  $_" }
    $lines | Where-Object { $_ -match '^Result: ' } | Select-Object -Last 1 | ForEach-Object { Write-Host $_ }
}

function Invoke-Ubt([string] $SlotName, [string] $UbtArguments, [string] $What) {
    $minutes = $(if ($TimeoutMinutes -gt 0) { $TimeoutMinutes } else { 8 })
    $spec = Add-JobSpec -SlotName $SlotName -Program $settings.BuildBat -CommandArguments $UbtArguments `
        -RunMinutes $minutes -Blocking (-not $Submit) -LogPath $Log -JobLabel $What -TailLines 0
    Start-JobWorker $spec
    if ($Submit) { $script:SubmittedId = $spec.id; Write-Host "Submitted $What as job $($spec.id). Wait with: unreal.ps1 wait $($spec.id)"; return 0 }
    $code = Wait-UnrealJob $spec.id ([int]($minutes * 60) + 60)
    Write-BuildSummary $spec.log
    return $code
}

function Invoke-Build {
    $project = Get-ProjectFile $Slot
    $rules = Get-TargetRules $Target
    $ubtArguments = '{0} Win64 {1} {2} -WaitMutex' -f $rules.Target, $rules.Configuration, (ConvertTo-ArgumentString @("-Project=$project"))
    return (Invoke-Ubt $rules.Slot $ubtArguments "build $($rules.Target) $($rules.Configuration) in $Slot")
}

function Invoke-Export {
    $project = Get-ProjectFile $Slot
    $rules = Get-TargetRules $Target
    if (-not $OutputFile) { throw '-OutputFile is required: where to write the JSON.' }
    $ubtArguments = '{0} Win64 {1} {2} -Mode=JsonExport {3} -WaitMutex' -f $rules.Target, $rules.Configuration, `
        (ConvertTo-ArgumentString @("-Project=$project")), (ConvertTo-ArgumentString @("-OutputFile=$OutputFile"))
    return (Invoke-Ubt 'NONE' $ubtArguments "export the $($rules.Target) module graph from $Slot")
}

function Invoke-Run([bool] $Blocking) {
    $minutes = $TimeoutMinutes
    if ($minutes -le 0) { $minutes = $(if ($Blocking) { 8 } else { 120 }) }
    $spec = Add-JobSpec -SlotName $Slot -Program $Exe -CommandArguments $Arguments -RunMinutes $minutes -Blocking $Blocking `
        -LogPath $Log -JobLabel $Label -TailLines $Tail
    Start-JobWorker $spec
    if (-not $Blocking) {
        $script:SubmittedId = $spec.id
        Write-Host "Submitted $($spec.slot) job $($spec.id) ($($spec.label)). Wait with: unreal.ps1 wait $($spec.id)"
        return 0
    }
    return (Wait-UnrealJob $spec.id ([int]($minutes * 60) + 60))
}

# A submitted job's id is the one line written to standard output, so a calling script can capture it.
$script:SubmittedId = ''
switch ($Command) {
    'run' { exit (Invoke-Run $true) }
    'submit' { $code = Invoke-Run $false; Write-Output $script:SubmittedId; exit $code }
    'wait' { if (-not $JobId) { throw 'wait needs a job id.' }; exit (Wait-UnrealJob $JobId ([Math]::Min($TimeoutSeconds, 580))) }
    'status' { Show-SlotStatus; exit 0 }
    'cancel' {
        if (-not $JobId) { throw 'cancel needs a job id.' }
        [void](Read-JobState $JobId)
        Write-TextFile (Join-Path (Get-JobFolder $JobId) 'cancel') (Get-UtcStamp)
        Write-Host "Asked job $JobId to stop; its worker ends the run and releases the slot within a few seconds."
        exit 0
    }
    'build' { $code = Invoke-Build; if ($script:SubmittedId) { Write-Output $script:SubmittedId }; exit $code }
    'export' { exit (Invoke-Export) }
    'work' {
        try { Invoke-JobWorker $JobId }
        catch { Write-TextFile (Join-Path (Get-JobFolder $JobId) 'worker-error.txt') ($_ | Out-String); exit 1 }
        exit 0
    }
}
