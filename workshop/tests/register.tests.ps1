<#
register.tests.ps1: tests register.ps1 and status.ps1 against a scratch Register (never the real one).

It makes a throwaway git repository on branch 'register' in the given folder, then writes every kind of entry,
checks the files and commits, checks that hand edits are refused, runs six writers at once to check that ids never
collide and every write is committed, and generates STATUS.md.

  powershell -NoProfile -File workshop\tests\register.tests.ps1 -Scratch C:\mv\run-selftest\register
Prints one PASS or FAIL line per check and exits with the number of failures.
#>
param([Parameter(Mandatory)] [string] $Scratch)
$ErrorActionPreference = 'Stop'
$workshop = Split-Path -Parent $PSScriptRoot
Import-Module (Join-Path $workshop 'lib\Workshop.psm1') -Force
$script:Failures = 0

function Test-That([string] $What, [bool] $Holds, [string] $Detail = '') {
    if ($Holds) { Write-Host "PASS  $What" } else { $script:Failures++; Write-Host "FAIL  $What $Detail" }
}

function Invoke-Register([string[]] $Arguments) {
    <# Runs register.ps1 in its own PowerShell, as an agent would; returns the exit code and all its output. #>
    $script = Join-Path $workshop 'register.ps1'
    $result = Invoke-Native -FilePath 'powershell.exe' -Arguments (@('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $script) + $Arguments)
    return [pscustomobject]@{ Code = $result.ExitCode; Text = ($result.Output + $result.Error) }
}

function Get-CommitCount { return [int](Invoke-Git -Repo $Scratch -Arguments @('rev-list', '--count', 'HEAD')) }

if (Test-Path -LiteralPath $Scratch) { Remove-Item -LiteralPath $Scratch -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Scratch | Out-Null
Invoke-Native -FilePath (Get-WorkshopSettings).Git -Arguments @('init', '--quiet', '-b', 'register', $Scratch) | Out-Null
$env:MUSEE_REGISTER = $Scratch

$init = Invoke-Register @('init')
Test-That 'init lays out the ten folders and commits them' ((Test-Path "$Scratch\sessions\.gitkeep") -and (Get-CommitCount) -eq 1) $init.Text

$issued = Invoke-Register @('order', '-Id', 'F-TEST-01', '-Title', 'a test order', '-Owner', 'Form', '-Slot', 'mb1',
    '-Allowed', 'MuseeVision/Source/Systems/Form/**,workshop/tests/**', '-Profile', 'physical')
$order = Read-OrderFile $Scratch 'F-TEST-01'
Test-That 'order writes orders/F-TEST-01.md with its fields' ($order -and $order.Fields['status'] -eq 'issued' -and
    @($order.Fields['allowed paths']).Count -eq 2 -and $order.Fields['verify profile'] -eq 'physical') $issued.Text
$again = Invoke-Register @('order', '-Id', 'F-TEST-01', '-Title', 'x', '-Owner', 'Form', '-Allowed', 'a')
Test-That 'order refuses to issue the same id twice' ($again.Code -ne 0) $again.Text
[void](Invoke-Register @('order', '-Id', 'F-TEST-01', '-Update', '-Slot', 'mb2'))
Test-That 'order -Update changes only the given field' ((Read-OrderFile $Scratch 'F-TEST-01').Fields['slot'] -eq 'mb2' -and
    (Read-OrderFile $Scratch 'F-TEST-01').Fields['title'] -eq 'a test order')

[void](Invoke-Register @('mark', '-Id', 'F-TEST-01', '-Status', 'building'))
$gap = Invoke-Register @('gap', '-Order', 'F-TEST-01', '-Against', 'Light / Public/LightPlan.h', '-Needed', 'a mount frame',
    '-Considered', 'FLightMount has no frame', '-Blocked', 'the coffer lights', '-Continuing', 'the walls')
Test-That 'gap files gaps/F-TEST-01-g1.md and blocks the order' ((Test-Path "$Scratch\gaps\F-TEST-01-g1.md") -and
    (Read-OrderFile $Scratch 'F-TEST-01').Fields['status'] -eq 'blocked') $gap.Text
[void](Invoke-Register @('answer', '-Gap', 'F-TEST-01-g1', '-Triage', 'exists', '-Answer', 'Use FLightMount::Frame.'))
Test-That 'answer marks the gap answered' ((Get-Content "$Scratch\gaps\F-TEST-01-g1.md" -Raw) -match 'status: answered \(exists\)')

[void](Invoke-Register @('mark', '-Id', 'F-TEST-01', '-Status', 'returned', '-Note', 'first inspection'))
Test-That 'mark returned counts a return' ((Read-OrderFile $Scratch 'F-TEST-01').Fields['returns'] -eq '1')
$bad = Invoke-Register @('mark', '-Id', 'F-TEST-01', '-Status', 'finished')
Test-That 'mark refuses a state that does not exist' ($bad.Code -ne 0)

$inspection = Invoke-Register @('inspection', '-Order', 'F-TEST-01', '-Verdict', 'pass', '-Stamp', ('a' * 40),
    '-Text', "Form.Sealed - pass - Wall.cpp:12`n[minor] a comment narrates the code")
Test-That 'inspection files F-TEST-01-i1 and passes the order' ((Test-Path "$Scratch\inspections\F-TEST-01-i1.md") -and
    (Read-OrderFile $Scratch 'F-TEST-01').Fields['status'] -eq 'passed') $inspection.Text

[void](Invoke-Register @('handoff', '-Order', 'F-TEST-01', '-Done', 'the walls', '-Next', 'the coffers'))
Test-That 'handoff writes handoff/F-TEST-01.md' (Test-Path "$Scratch\handoff\F-TEST-01.md")

$runFile = Join-Path $Scratch '..\verify-run.json'
foreach ($profile in @('code', 'physical')) {
    Write-JsonFile $runFile ([ordered]@{ sha = ('b' * 40); order = 'F-TEST-01'; profile = $profile; green = $true; finished = (Get-UtcStamp); steps = @() })
    [void](Invoke-Register @('stamp', '-FromFile', $runFile))
}
$stamp = Read-JsonFile "$Scratch\verify\verify-$('b' * 40).json"
Test-That 'stamp keeps every run of verify on one commit' (@($stamp.runs).Count -eq 2)

$jobFile = Join-Path $Scratch '..\job.json'
Write-JsonFile $jobFile ([ordered]@{ id = '20260926-180000-heavy-0a1b'; order = 'F-TEST-01'; slot = 'HEAVY'; outcome = 'done'
    queueSeconds = 12; runSeconds = 60; finished = (Get-UtcStamp) })
[void](Invoke-Register @('job', '-FromFile', $jobFile))
Test-That 'job writes jobs/<id>.json' (Test-Path "$Scratch\jobs\20260926-180000-heavy-0a1b.json")

[void](Invoke-Register @('defect', '-Slug', 'light-streak-on-dome', '-Symptom', 'a light streak on the dome',
    '-Truth', 'the shell had a seam', '-Remedy', 'Form closes the seam (F-FORM-07)', '-Check', 'Form.Sealed'))
Test-That 'defect writes defects/<date>-light-streak-on-dome.md' (@(Get-ChildItem "$Scratch\defects" -Filter '*-light-streak-on-dome.md').Count -eq 1)

$image = Join-Path $Scratch '..\walk.png'
[IO.File]::WriteAllBytes($image, [byte[]](137, 80, 78, 71, 13, 10, 26, 10))
[void](Invoke-Register @('critique', '-Words', 'The coffers want one step deeper.', '-Images', $image, '-Routing', 'design note -> spec'))
$walk = @(Get-ChildItem "$Scratch\critique" -Directory)
Test-That 'critique keeps the owner''s words verbatim beside the images' ($walk.Count -eq 1 -and (Test-Path "$($walk[0].FullName)\walk.png") -and
    (Get-Content "$($walk[0].FullName)\walk.md" -Raw) -match '> The coffers want one step deeper\.')

[void](Invoke-Register @('gate', '-Gate', 'A', '-Text', 'The system map and the Rotunda spec.'))
[void](Invoke-Register @('gate', '-Gate', 'A', '-Decision', 'Approved, go ahead.'))
$gate = Get-Content "$Scratch\gates\A.md" -Raw
Test-That 'gate keeps the packet and quotes the decision' ($gate -match 'Rotunda spec' -and $gate -match '> Approved, go ahead\.' -and $gate -notmatch 'none yet')

[void](Invoke-Register @('session', '-Text', 'Wave 2a triaged.'))
[void](Invoke-Register @('session', '-Text', 'Wave 2b issued.'))
Test-That 'session numbers logs within a day' (@(Get-ChildItem "$Scratch\sessions" -Filter '*-2.md').Count -eq 1)

[void](Invoke-Register @('tokens', '-Id', 'F-TEST-01', '-Tokens', '1500'))
[void](Invoke-Register @('tokens', '-Id', 'F-TEST-01', '-Tokens', '500'))
Test-That 'tokens adds up' ((Read-OrderFile $Scratch 'F-TEST-01').Fields['tokens'] -eq '2000')

Add-Content -LiteralPath "$Scratch\orders\F-TEST-01.md" -Value 'edited by hand'
$refused = Invoke-Register @('session', '-Text', 'must be refused')
Test-That 'writes are refused while the Register has hand edits, naming the file' ($refused.Code -ne 0 -and $refused.Text -match 'orders/F-TEST-01.md') $refused.Text
Invoke-Git -Repo $Scratch -Arguments @('checkout', '--', 'orders/F-TEST-01.md') | Out-Null

$before = Get-CommitCount
$script = Join-Path $workshop 'register.ps1'
$writers = foreach ($i in 1..6) {
    Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass',
        '-File', "`"$script`"", 'gap', '-Order', 'F-TEST-01', '-Against', "`"Form / writer $i`"", '-Needed', 'n', '-Considered', 'c',
        '-Blocked', 'b', '-Continuing', 'c')
}
$writers | ForEach-Object { $_.WaitForExit() }
$gaps = @(Get-ChildItem "$Scratch\gaps" -Filter 'F-TEST-01-g*.md')
$status = Invoke-Git -Repo $Scratch -Arguments @('status', '--porcelain')
Test-That 'six writers at once: seven distinct gap ids, one commit each, nothing left uncommitted' ($gaps.Count -eq 7 -and
    (Get-CommitCount) -eq ($before + 6) -and -not $status) "gaps=$($gaps.Count) commits=$((Get-CommitCount) - $before)"

$statusRun = Invoke-Native -FilePath 'powershell.exe' -Arguments @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $workshop 'status.ps1'))
$page = Get-Content "$Scratch\STATUS.md" -Raw
Test-That 'status.ps1 writes STATUS.md with the order, its gaps and today''s queue' ($statusRun.ExitCode -eq 0 -and $page -match '\| F-TEST-01 \|' -and
    $page -match 'F-TEST-01-g7' -and $page -match 'HEAVY: 1 job\(s\)' -and $page -match 'bbbbbbbb green \(physical\)') ($statusRun.Output + $statusRun.Error)

Write-Host ("{0} failures." -f $script:Failures)
exit $script:Failures
