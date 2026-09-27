<#
verify.ps1: checks one commit of one order, the same way every time, and stamps the result into the Register
(BUILD_PLAN 5.3 and 8). A builder's own claim of passing counts for nothing; this tool's stamp does.

  verify.ps1 -Slot <worktree> -Order <id> [-Profile code|physical|measured] [-Full] [-Base renaissance] [-Background]

The steps (the profile says which run; each order names its profile in its order file):
  1 compile     the editor target; with -Full also the game and VR targets (in the HEAVY slot)
  2 wiring      UnrealBuildTool's resolved module graph (-Mode=JsonExport, editor; with -Full game and VR too)
                against Docs/wiring.json, plus banned module rules and #includes (lib\wiring_check.py)
  3 ownership   the diff <Base>...HEAD touches only the order's allowed paths (from its Register order file), and
                its locked acceptance tests are unchanged; a branch cut from an older contract is warned to rebase
  4 craft       the craft lint with Craft.md's own rules (lib\craft_lint.py)
  5 tests       Unreal automation tests "Musee." headless, in the LIGHT slot
  6 physical    (physical and measured) Studio's checks, exposed as automation tests "MuseeCheck.", LIGHT slot
  7 measured    (measured) Studio's measurements, automation tests "MuseeMeasure.", rendered in the GPU slot
  8 evidence    (measured) Studio's evidence renders, automation tests "MuseeEvidence.", GPU slot
It prints one line per step and the first ten errors; full logs go to <worktree>\MuseeVision\Saved\verify\<sha>\.
The stamp, verify-<sha>.json in the Register, records every step; a tree with uncommitted changes is checked but
never stamped. Exit code 0 when green, 1 when red.

The rules are never read from the worktree under test: wiring.json and Craft.md come from the tip of -Base
(renaissance) in the integration worktree, and this script hands itself over to the copy of workshop/ at that tip
before it starts, so a builder can't weaken the checker. A full verify outlasts one tool call, so agents pass
-Background, which returns a job id at once; follow it with unreal.ps1 wait <id>.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $Slot,
    [Parameter(Mandatory)] [string] $Order,
    [ValidateSet('', 'code', 'physical', 'measured')] [string] $Profile = '',
    [switch] $Full,
    [string] $Base = 'renaissance',
    [string] $RulesRef = '',
    [switch] $Background,
    [switch] $Relayed
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
$settings = Get-WorkshopSettings
$Slot = Get-FullPath $Slot
Assert-NotOwnerCheckout $Slot
if (-not $RulesRef) { $RulesRef = $Base }
$unreal = Join-Path $PSScriptRoot 'unreal.ps1'

# ---- Running in the background, and handing over to the checker at the rules' tip ------------------------------

function Submit-Verify {
    $forwarded = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath, '-Slot', $Slot, '-Order', $Order, '-Base', $Base, '-RulesRef', $RulesRef)
    if ($Profile) { $forwarded += @('-Profile', $Profile) }
    if ($Full) { $forwarded += '-Full' }
    $id = & $unreal submit -Slot NONE -Exe 'powershell.exe' -Args (ConvertTo-ArgumentString $forwarded) -TimeoutMinutes 240 `
        -Order $Order -Label "verify $Order in $Slot" -Tail 40 6>$null
    Write-Host "Verify of $Order runs in the background as job $id. Follow it with: unreal.ps1 wait $id"
    Write-Output $id
}

function Get-CheckerCopy([string] $RulesSha) {
    <# The workshop/ folder as committed at the rules' tip, unpacked once per commit under C:\mv\run\checker. #>
    $root = Join-Path $settings.RunRoot "checker\$RulesSha"
    $script = Join-Path $root 'workshop\verify.ps1'
    if (Test-Path -LiteralPath $script) { return $script }
    $mutex = Enter-NamedMutex -Name 'Global\MuseeVerifyChecker' -TimeoutMilliseconds 120000
    try {
        if (-not (Test-Path -LiteralPath $script)) {
            $zip = "$root.zip"
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $root) | Out-Null
            Invoke-Git -Repo $settings.Integration -Arguments @('archive', '--format=zip', '-o', $zip, $RulesSha, 'workshop') | Out-Null
            Expand-Archive -LiteralPath $zip -DestinationPath $root -Force
            Remove-Item -LiteralPath $zip -Force
        }
    } finally { Exit-NamedMutex $mutex }
    return $script
}

function Invoke-RulesChecker([string] $RulesSha) {
    <# Hands over to the checker committed at the rules' tip. Returns $null when that tip has no workshop yet. #>
    $present = Invoke-Git -Repo $settings.Integration -Arguments @('cat-file', '-e', "${RulesSha}:workshop/verify.ps1") -AllowFailure
    if ($present.ExitCode -ne 0) { return $null }
    $script = Get-CheckerCopy $RulesSha
    $forwarded = @{ Slot = $Slot; Order = $Order; Base = $Base; RulesRef = $RulesRef; Relayed = $true }
    if ($Profile) { $forwarded.Profile = $Profile }
    if ($Full) { $forwarded.Full = $true }
    & $script @forwarded
    return $LASTEXITCODE
}

# ---- The steps -------------------------------------------------------------------------------------------------

function Start-Step([int] $Number, [string] $Name) {
    return [ordered]@{ step = $Number; name = $Name; result = 'pass'; seconds = 0; summary = ''; errors = @(); warnings = @(); logs = @() }
}

function Complete-Step($Step, [DateTime] $Started) {
    $Step.seconds = [Math]::Round(([DateTime]::UtcNow - $Started).TotalSeconds, 1)
    if (@($Step.errors).Count -gt 0) { $Step.result = 'fail' }
    return $Step
}

function Invoke-Compile($Context) {
    $step = Start-Step 1 'compile'; $started = [DateTime]::UtcNow
    $targets = @('Editor') + $(if ($Full) { @('Game', 'VR') } else { @() })
    $parts = @()
    foreach ($target in $targets) {
        $log = Join-Path $Context.Logs "build-$($target.ToLowerInvariant()).log"
        $begun = [DateTime]::UtcNow
        & $unreal build -Slot $Slot -Target $target -Log $log -TimeoutMinutes 60 6>$null
        $code = $LASTEXITCODE
        $step.logs += $log
        $parts += '{0} {1}s' -f $target.ToLowerInvariant(), [int]([DateTime]::UtcNow - $begun).TotalSeconds
        if ($code -ne 0) {
            $lines = @(Get-BuildErrors $log)
            if (-not $lines.Count) { $lines = @("the $target build ended with exit code $code; see $log") }
            $step.errors += @($lines | ForEach-Object { "compile ($target): $($_.Trim())" })
        }
    }
    $step.summary = $parts -join ', '
    $Context.Compiled = (@($step.errors).Count -eq 0)
    return (Complete-Step $step $started)
}

function Get-RuleFile($Context, [string] $Path) {
    <# One rule file as committed at the rules' tip, written into the logs folder; $null when the tip lacks it. #>
    $result = Invoke-Git -Repo $settings.Integration -Arguments @('show', "$($Context.RulesSha):$Path") -AllowFailure
    if ($result.ExitCode -ne 0) { return $null }
    $target = Join-Path $Context.Logs ("rules\" + (Split-Path -Leaf $Path))
    Write-TextFile $target $result.Output
    return $target
}

function Invoke-Python([string[]] $Arguments) {
    $result = Invoke-Native -FilePath $settings.Python -Arguments (@('-B') + $Arguments)
    return [pscustomobject]@{ ExitCode = $result.ExitCode; Lines = @(Get-GitLines ($result.Output + $result.Error)) }
}

function Invoke-Wiring($Context) {
    $step = Start-Step 2 'wiring'; $started = [DateTime]::UtcNow
    $wiring = Get-RuleFile $Context 'MuseeVision/Docs/wiring.json'
    if (-not $wiring) { $step.errors += "wiring: $RulesRef has no MuseeVision/Docs/wiring.json to check against"; return (Complete-Step $step $started) }
    $exports = @()
    foreach ($target in @('Editor') + $(if ($Full) { @('Game', 'VR') } else { @() })) {
        $file = Join-Path $Context.Logs "graph-$($target.ToLowerInvariant()).json"
        $log = Join-Path $Context.Logs "graph-$($target.ToLowerInvariant()).log"
        & $unreal export -Slot $Slot -Target $target -OutputFile $file -Log $log -TimeoutMinutes 30 6>$null
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $file)) { $step.errors += "wiring: UnrealBuildTool could not export the $target module graph; see $log"; continue }
        $exports += @('--export', $file)
        $step.logs += $log
    }
    if (-not $exports.Count) { return (Complete-Step $step $started) }
    $check = Invoke-Python (@((Join-Path $PSScriptRoot 'lib\wiring_check.py'), '--wiring', $wiring, '--project', (Join-Path $Slot $settings.Project),
        '--report', (Join-Path $Context.Logs 'wiring-findings.json')) + $exports)
    $step.errors += @($check.Lines | Where-Object { $_ -like 'ERROR*' } | ForEach-Object { $_ -replace '^ERROR\S*\s+', '' })
    $step.warnings += @($check.Lines | Where-Object { $_ -like 'NOTE*' } | ForEach-Object { $_ -replace '^NOTE\s+', '' })
    $step.summary = @($check.Lines | Select-Object -Last 1)[0]
    return (Complete-Step $step $started)
}

function ConvertTo-PathPattern([string] $Glob) {
    <# An allowed-path glob as a regular expression: ** crosses folders, * stays within one. #>
    $escaped = [regex]::Escape($Glob.Trim().TrimStart('/'))
    $escaped = $escaped -replace '\\\*\\\*/', '(?:.*/)?' -replace '\\\*\\\*', '.*' -replace '\\\*', '[^/]*' -replace '\\\?', '[^/]'
    return "^$escaped$"
}

function Test-AcceptanceTests($Order, $Step) {
    foreach ($entry in @($Order.Fields['acceptance tests (locked)'])) {
        if (-not $entry) { continue }
        if ($entry -notmatch '^(.+?)\s+sha256:\s*([0-9a-fA-F]{64})\s*$') { $Step.errors += "ownership: the locked test entry '$entry' has no sha256"; continue }
        $path = $Matches[1].Trim(); $expected = $Matches[2].ToLowerInvariant()
        $actual = Get-GitBlobSha256 -Repo $Slot -Commit 'HEAD' -Path $path
        if ($actual -ne $expected) { $Step.errors += "ownership: the locked acceptance test $path was changed or removed" }
    }
}

function Test-ContractCurrent($Step) {
    <# Whether the branch was cut from the current contract: the public headers at its merge base with the target
       against those at the target's tip. A branch behind the contract is warned to rebase; the merge tool squashes
       onto the tip and verifies the result, so it is never merged stale. #>
    $mergeBase = Invoke-Git -Repo $Slot -Arguments @('merge-base', $Base, 'HEAD')
    $then = Get-ContractVersion $settings.Integration $mergeBase
    $now = Get-ContractVersion $settings.Integration $Base
    if ($then -eq $now) { return }
    $moved = @(Get-GitLines (Invoke-Git -Repo $Slot -Arguments @('diff', '--name-only', $mergeBase, $Base, '--', 'MuseeVision/Source')) | Where-Object { $_ -match '/Public/' })
    $Step.warnings += "ownership: the branch is behind the contract ($then, $Base is at $now; $($moved.Count) public headers changed, first $($moved | Select-Object -First 1)); rebase before marking the order built"
}

function Invoke-Ownership($Context) {
    $step = Start-Step 3 'ownership'; $started = [DateTime]::UtcNow
    $orderEntry = $Context.OrderEntry
    if (-not $orderEntry) { $step.errors += "ownership: there is no order $Order in the Register ($($settings.Register)\orders)"; return (Complete-Step $step $started) }
    $patterns = @($orderEntry.Fields['allowed paths'] | Where-Object { $_ } | ForEach-Object { ConvertTo-PathPattern $_ })
    $changed = @(Get-GitLines (Invoke-Git -Repo $Slot -Arguments @('diff', '--name-only', '--no-renames', "$Base...HEAD")))
    $uncommitted = @(Get-GitLines (Invoke-Git -Repo $Slot -Arguments @('status', '--porcelain')) | ForEach-Object { $_.Substring(3).Trim('"') })
    $Context.Changed = @($changed + $uncommitted | Select-Object -Unique)
    foreach ($path in $Context.Changed) {
        if (-not ($patterns | Where-Object { $path -match $_ })) { $step.errors += "ownership: $path is outside the order's allowed paths" }
    }
    Test-AcceptanceTests $orderEntry $step
    Test-ContractCurrent $step
    $step.summary = '{0} files changed since {1}' -f $Context.Changed.Count, $Base
    return (Complete-Step $step $started)
}

function Invoke-Craft($Context) {
    $step = Start-Step 4 'craft'; $started = [DateTime]::UtcNow
    $craft = Get-RuleFile $Context 'MuseeVision/Docs/Craft.md'
    if (-not $craft) { $step.errors += "craft: $RulesRef has no MuseeVision/Docs/Craft.md"; return (Complete-Step $step $started) }
    $changedList = Join-Path $Context.Logs 'changed.txt'
    Write-TextFile $changedList (($Context.Changed -join "`n") + "`n")
    $lint = Invoke-Python @((Join-Path $PSScriptRoot 'lib\craft_lint.py'), '--craft', $craft, '--root', $Slot, '--project', $settings.Project,
        '--changed', $changedList, '--report', (Join-Path $Context.Logs 'craft-findings.json'))
    $step.errors += @($lint.Lines | Where-Object { $_ -like 'ERROR*' } | ForEach-Object { $_ -replace '^ERROR\S*\s+', '' })
    $step.warnings += @($lint.Lines | Where-Object { $_ -like 'WARN*' } | ForEach-Object { $_ -replace '^WARN\S*\s+', '' })
    $step.summary = @($lint.Lines | Select-Object -Last 1)[0]
    return (Complete-Step $step $started)
}

function Invoke-Automation($Context, [int] $Number, [string] $Name, [string] $Filter, [string] $SlotName, [bool] $Rendered, [bool] $Required) {
    <# Runs Unreal automation tests whose names start with the filter, in a slot, and reads the report. #>
    $step = Start-Step $Number $Name; $started = [DateTime]::UtcNow
    if (-not $Context.Compiled) { $step.result = 'skip'; $step.summary = 'not run: the editor did not compile'; return (Complete-Step $step $started) }
    $reportDir = Join-Path $Context.Logs $Name
    if (Test-Path -LiteralPath $reportDir) { Remove-Item -LiteralPath $reportDir -Recurse -Force }
    $log = Join-Path $Context.Logs "$Name.log"
    $project = Join-Path $Slot "$($settings.Project)\$($settings.Project).uproject"
    # A headless run renders nothing and serves no Android device, so it leaves out the NVIDIA rendering plugins (with
    # them loaded the editor can't start while the shared plugin folder is broken: gap T-WORKSHOP-01-g2) and the
    # Android file server (whose editor module writes a token into Config/DefaultEngine.ini, leaving the worktree
    # dirty: gap T-WORKSHOP-01-g3). A rendered run needs the NVIDIA plugins.
    $render = $(if ($Rendered) { '-RenderOffscreen' } else { '-nullrhi -DisablePlugins=DLSS,StreamlineDLSSG,StreamlineReflex,StreamlineCore,StreamlineNGXCommon,AndroidFileServer' })
    $arguments = '"{0}" {1} -unattended -nosplash -nosound -ExecCmds="Automation RunTests {2};Quit" -TestExit="Automation Test Queue Empty" -ReportExportPath="{3}" -abslog="{4}"' -f `
        $project, $render, $Filter, $reportDir, $log
    & $unreal run -Slot $SlotName -Exe $settings.EditorCmd -Args $arguments -TimeoutMinutes 30 -Order $Order -Label "verify $Order $Name" 6>$null
    $code = $LASTEXITCODE
    $step.logs += $log
    Read-AutomationReport $step (Join-Path $reportDir 'index.json') $log $Filter $code $Required
    return (Complete-Step $step $started)
}

function Read-AutomationReport($Step, [string] $Index, [string] $Log, [string] $Filter, [int] $Code, [bool] $Required) {
    <# Reads Unreal's automation report: every test that didn't succeed is an error, with its first error message.
       Unreal writes no report (and exits 255) when no test matches the filter; that is "no tests yet". #>
    if (-not (Test-Path -LiteralPath $Index)) {
        $none = (Test-Path -LiteralPath $Log) -and (Select-String -LiteralPath $Log -SimpleMatch 'No automation tests matched' -Quiet)
        if (-not $none) {
            $Step.errors += "$($Step.name): the Unreal run ended with exit code $Code and wrote no report; see $Log"
            $Step.summary = "Unreal exit code $Code, no report"
        } else {
            $message = "$($Step.name): no automation tests named $Filter* exist yet"
            if ($Required) { $Step.errors += $message } else { $Step.warnings += $message }
            $Step.summary = "no tests named $Filter*"
        }
        return
    }
    $report = Get-Content -LiteralPath $Index -Raw -Encoding UTF8 | ConvertFrom-Json
    $tests = @($report.tests)
    foreach ($test in ($tests | Where-Object { $_.state -ne 'Success' })) {
        $first = @($test.entries | Where-Object { $_.event.type -eq 'Error' } | Select-Object -First 1)
        $why = $(if ($first.Count) { $first[0].event.message } else { $test.state })
        $Step.errors += "$($Step.name): $($test.fullTestPath) $($test.state): $why"
    }
    if (-not $tests.Count) {
        if ($Required) { $Step.errors += "$($Step.name): no automation tests named $Filter* exist yet" }
        else { $Step.warnings += "$($Step.name): no automation tests named $Filter* exist yet" }
    }
    $Step.summary = '{0} tests, {1} failed' -f $tests.Count, @($tests | Where-Object { $_.state -ne 'Success' }).Count
}

# ---- The run -----------------------------------------------------------------------------------------------------

function Write-Summary($Context, $Steps, [bool] $Green, [string] $StampNote) {
    Write-Host ("verify {0} at {1} (profile {2}) in {3}; rules from {4} at {5}" -f $Order, $Context.Sha.Substring(0, 10), $Context.Profile, $Slot, $RulesRef, $Context.RulesSha.Substring(0, 10))
    foreach ($step in $Steps) {
        $label = ('{0} {1} ' -f $step.step, $step.name).PadRight(16, '.')
        $problems = $(if (@($step.errors).Count) { " - $(@($step.errors).Count) errors" } else { '' })
        Write-Host ("  {0} {1,-4} {2}{3}" -f $label, $step.result.ToUpperInvariant(), $step.summary, $problems)
    }
    $errors = @($Steps | ForEach-Object { @($_.errors) } | Where-Object { $_ })
    if ($Green) { Write-Host "GREEN." } else {
        Write-Host ("RED: {0} errors. The first {1}:" -f $errors.Count, [Math]::Min(10, $errors.Count))
        $errors | Select-Object -First 10 | ForEach-Object { Write-Host "  $_" }
    }
    Write-Host $StampNote
    Write-Host "Logs: $($Context.Logs)"
}

function Save-VerifyStamp($Context, $Steps, [bool] $Green, [DateTime] $Began) {
    <# Writes the run into the Register as verify/verify-<sha>.json; returns a line saying where, or why not. #>
    if ($Context.Dirty) { return 'Not stamped: the worktree has uncommitted changes, and a stamp is only ever for a commit.' }
    if (-not (Test-RegisterPresent $settings.Register)) { return "Not stamped: there is no Register at $($settings.Register)." }
    $run = [ordered]@{
        sha = $Context.Sha; order = $Order; profile = $Context.Profile; green = $Green; slot = $Slot
        base = $Base; baseSha = $Context.BaseSha; rulesRef = $RulesRef; rulesSha = $Context.RulesSha
        checker = $(if ($Relayed) { "workshop/ at $($Context.RulesSha)" } else { "this worktree's workshop/ ($RulesRef had none)" })
        full = [bool]$Full; started = $Began.ToString('yyyy-MM-ddTHH:mm:ssZ'); finished = (Get-UtcStamp); steps = @($Steps)
    }
    $file = Join-Path $Context.Logs 'stamp.json'
    Write-JsonFile $file $run
    & (Join-Path $PSScriptRoot 'register.ps1') stamp -FromFile $file 6>$null | Out-Null
    return "Stamp: $(Join-Path $settings.Register "verify\verify-$($Context.Sha).json")"
}

function Invoke-Verify {
    $began = [DateTime]::UtcNow
    $orderEntry = Read-OrderFile $settings.Register $Order
    $chosen = $Profile
    if (-not $chosen -and $orderEntry) { $chosen = [string]$orderEntry.Fields['verify profile'] }
    if (-not $chosen) { $chosen = 'code' }
    $sha = Invoke-Git -Repo $Slot -Arguments @('rev-parse', 'HEAD')
    $context = [pscustomobject]@{
        Sha = $sha; Profile = $chosen; OrderEntry = $orderEntry; Compiled = $false; Changed = @()
        Dirty = [bool](Get-GitLines (Invoke-Git -Repo $Slot -Arguments @('status', '--porcelain')))
        RulesSha = Invoke-Git -Repo $settings.Integration -Arguments @('rev-parse', $RulesRef)
        BaseSha = Invoke-Git -Repo $Slot -Arguments @('rev-parse', $Base)
        Logs = Join-Path $Slot "$($settings.Project)\Saved\verify\$sha"
    }
    New-Item -ItemType Directory -Force -Path $context.Logs | Out-Null
    $steps = @((Invoke-Compile $context), (Invoke-Wiring $context), (Invoke-Ownership $context), (Invoke-Craft $context))
    $steps += Invoke-Automation $context 5 'tests' 'Musee.' 'LIGHT' $false $false
    if ($chosen -in @('physical', 'measured')) { $steps += Invoke-Automation $context 6 'physical' 'MuseeCheck.' 'LIGHT' $false $true }
    if ($chosen -eq 'measured') {
        $steps += Invoke-Automation $context 7 'measured' 'MuseeMeasure.' 'GPU' $true $true
        $steps += Invoke-Automation $context 8 'evidence' 'MuseeEvidence.' 'GPU' $true $true
    }
    $green = -not ($steps | Where-Object { $_.result -ne 'pass' })
    $note = Save-VerifyStamp $context $steps $green $began
    Write-Summary $context $steps $green $note
    return $(if ($green) { 0 } else { 1 })
}

if ($Background) { Submit-Verify; exit 0 }
if (-not $Relayed) {
    $rulesSha = Invoke-Git -Repo $settings.Integration -Arguments @('rev-parse', $RulesRef)
    $handed = Invoke-RulesChecker $rulesSha
    if ($null -ne $handed) { exit $handed }
    Write-Host "$RulesRef has no workshop/ yet, so this copy of verify.ps1 checks, and the stamp says so."
}
exit (Invoke-Verify)
