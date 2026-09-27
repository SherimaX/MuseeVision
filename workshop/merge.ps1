<#
merge.ps1: merges one inspected order onto renaissance, keeping renaissance green (BUILD_PLAN 4.3).

In the integration worktree, and never touching the order's branch there (it is checked out in its own slot):
  1. squashes the order's branch tip into one commit on top of renaissance, without touching any working tree
     (git merge-tree), and detaches the integration worktree at that candidate;
  2. runs verify on the candidate (-Full: editor, game and VR), with the rules of the renaissance tip;
  3. only if the candidate is green, fast-forwards renaissance to it (refusing if renaissance moved meanwhile);
  4. verifies renaissance again, now with the rules the merge brought; if it is red, reverts the merge commit, says
     so, and returns the order.
The order must have passed inspection. One merge runs at a time (mutex Global\MuseeMerge). Its result is written
to the order file: merged with the commit, or returned with the first errors.

  merge.ps1 -Order <id> [-Background]
  merge.ps1 -Order <id> -Into <branch> -Worktree <path>     (the tools' own tests use a throwaway branch and worktree)
A merge outlasts one tool call (two full verifies), so agents pass -Background and follow with unreal.ps1 wait <id>.
Exit code 0 when merged, 1 when not.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $Order,
    [string] $Into = 'renaissance',
    [string] $Worktree = '',
    [switch] $Background
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
$settings = Get-WorkshopSettings
if (-not $Worktree) { $Worktree = $settings.Integration }
$Worktree = Get-FullPath $Worktree
$register = Join-Path $PSScriptRoot 'register.ps1'
$verify = Join-Path $PSScriptRoot 'verify.ps1'

function Submit-Merge {
    $forwarded = ConvertTo-ArgumentString @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath, '-Order', $Order, '-Into', $Into, '-Worktree', $Worktree)
    $id = & (Join-Path $PSScriptRoot 'unreal.ps1') submit -Slot NONE -Exe 'powershell.exe' -Args $forwarded -TimeoutMinutes 480 `
        -Order $Order -Label "merge $Order into $Into" -Tail 30 6>$null
    Write-Host "The merge of $Order runs in the background as job $id. Follow it with: unreal.ps1 wait $id"
    Write-Output $id
}

function Get-MergeOrder {
    $entry = Read-OrderFile $settings.Register $Order
    if (-not $entry) { throw "There is no order $Order in the Register." }
    if ($entry.Fields['status'] -ne 'passed') {
        throw "Refused: $Order is '$($entry.Fields['status'])'. An order merges only after its inspection has passed."
    }
    return $entry
}

function Set-IntegrationReady {
    <# The integration worktree is clean and on the target branch (a merge that died mid-way left it detached). #>
    $changes = @(Get-GitLines (Invoke-Git -Repo $Worktree -Arguments @('status', '--porcelain')))
    if ($changes.Count) { throw "Refused: $Worktree has uncommitted changes:`n  $(($changes | Select-Object -First 10) -join "`n  ")" }
    $branch = (Invoke-Git -Repo $Worktree -Arguments @('symbolic-ref', '-q', '--short', 'HEAD') -AllowFailure).Output.Trim()
    if (-not $branch) { Invoke-Git -Repo $Worktree -Arguments @('checkout', '--quiet', $Into) | Out-Null; return }
    if ($branch -ne $Into) { throw "Refused: $Worktree is on '$branch', not '$Into'." }
}

function Get-SquashMessage($Entry, [string] $Before, [string] $Tip) {
    $subjects = @(Get-GitLines (Invoke-Git -Repo $Worktree -Arguments @('log', '--reverse', '--format=- %s', "$Before..$Tip")))
    $trailers = @(Get-GitLines (Invoke-Git -Repo $Worktree -Arguments @('log', '--format=%(trailers:key=Co-Authored-By)', "$Before..$Tip")) |
        Select-Object -Unique)
    $lines = @("$($Entry.Fields['owner']): $($Entry.Fields['title']) ($Order)", '',
        "Squashed from $($Entry.Fields['branch']) at $($Tip.Substring(0, 10)); verified before and after the merge by workshop/merge.ps1.", '') +
        $subjects
    if ($trailers.Count) { $lines += @('') + $trailers }
    return (($lines -join "`n") + "`n")
}

function Build-Candidate($Entry, [string] $Before, [string] $Tip) {
    <# The squash commit on top of the target, made without touching a working tree; $null when it conflicts. #>
    $merged = Invoke-Git -Repo $Worktree -Arguments @('merge-tree', '--write-tree', '--name-only', $Before, $Tip) -AllowFailure
    $lines = @(Get-GitLines $merged.Output)
    if ($merged.ExitCode -ne 0) {
        Write-Host "$Order does not merge cleanly onto $Into; rebase it in its slot. Conflicting paths:"
        $lines | Select-Object -Skip 1 | Where-Object { $_ -notmatch '^(Auto-merging|CONFLICT)' } | Select-Object -First 20 | ForEach-Object { Write-Host "  $_" }
        return $null
    }
    $messageFile = Join-Path $settings.RunRoot "merge-$Order.txt"
    Write-TextFile $messageFile (Get-SquashMessage $Entry $Before $Tip)
    return (Invoke-Git -Repo $Worktree -Arguments @('commit-tree', $lines[0], '-p', $Before, '-F', $messageFile)).Trim()
}

function Invoke-VerifyStep([string] $Commit, $Entry, [string] $When) {
    <# Runs verify on the integration worktree and checks the Register holds a green stamp for exactly this commit. #>
    Write-Host "Verifying $When ($($Commit.Substring(0, 10)))..."
    & $verify -Slot $Worktree -Order $Order -Profile $Entry.Fields['verify profile'] -Full -Base $Into
    $green = ($LASTEXITCODE -eq 0)
    $stampPath = Join-Path $settings.Register "verify\verify-$Commit.json"
    if ($green -and (Test-Path -LiteralPath $stampPath)) {
        $last = @((Read-JsonFile $stampPath).runs)[-1]
        $green = [bool]$last.green -and $last.sha -eq $Commit
    } elseif ($green) { $green = $false; Write-Host "No stamp for $Commit was found in the Register, so the merge doesn't count it as green." }
    return $green
}

function Get-FirstErrors([string] $Commit) {
    $stampPath = Join-Path $settings.Register "verify\verify-$Commit.json"
    if (-not (Test-Path -LiteralPath $stampPath)) { return 'verify wrote no stamp' }
    $last = @((Read-JsonFile $stampPath).runs)[-1]
    $errors = @($last.steps | ForEach-Object { @($_.errors) } | Where-Object { $_ } | Select-Object -First 3)
    return ($errors -join '; ')
}

function Undo-Merge([string] $Candidate, $Entry) {
    <# Reverts the merge commit on the target branch with an owner-first message, and returns the order. #>
    Invoke-Git -Repo $Worktree -Arguments @('revert', '--no-commit', $Candidate) | Out-Null
    $message = "$($Entry.Fields['owner']): revert $Order; $Into was red after the merge`n`nReverted by workshop/merge.ps1: $(Get-FirstErrors $Candidate)`n"
    $messageFile = Join-Path $settings.RunRoot "merge-$Order-revert.txt"
    Write-TextFile $messageFile $message
    Invoke-Git -Repo $Worktree -Arguments @('commit', '--quiet', '-F', $messageFile) | Out-Null
    $revert = Invoke-Git -Repo $Worktree -Arguments @('rev-parse', 'HEAD')
    & $register mark -Id $Order -Status returned -Note "reverted: $Into was red after the merge ($(Get-FirstErrors $Candidate))" 6>$null | Out-Null
    Write-Host "$Into was RED after the merge, so the merge was reverted by $($revert.Substring(0, 10)) and $Order was returned."
}

function Invoke-Merge {
    $entry = Get-MergeOrder
    Set-IntegrationReady
    $before = Invoke-Git -Repo $Worktree -Arguments @('rev-parse', $Into)
    $tip = Invoke-Git -Repo $Worktree -Arguments @('rev-parse', $entry.Fields['branch'])
    $contained = Invoke-Git -Repo $Worktree -Arguments @('merge-base', '--is-ancestor', $tip, $before) -AllowFailure
    if ($contained.ExitCode -eq 0) { Write-Host "$Order's branch is already part of $Into; nothing to merge."; return 0 }
    $candidate = Build-Candidate $entry $before $tip
    if (-not $candidate) { return 1 }
    Invoke-Git -Repo $Worktree -Arguments @('checkout', '--quiet', '--detach', $candidate) | Out-Null
    if (-not (Invoke-VerifyStep $candidate $entry 'the merge candidate')) {
        Invoke-Git -Repo $Worktree -Arguments @('checkout', '--quiet', $Into) | Out-Null
        & $register mark -Id $Order -Status returned -Note "merge candidate $($candidate.Substring(0, 10)) was red: $(Get-FirstErrors $candidate)" 6>$null | Out-Null
        Write-Host "The merge candidate was RED, so $Into was not moved and $Order was returned."
        return 1
    }
    $moved = Invoke-Git -Repo $Worktree -Arguments @('update-ref', "refs/heads/$Into", $candidate, $before) -AllowFailure
    if ($moved.ExitCode -ne 0) {
        Invoke-Git -Repo $Worktree -Arguments @('checkout', '--quiet', $Into) | Out-Null
        Write-Host "$Into moved while $Order was being verified, so nothing was merged. Run the merge again."
        return 1
    }
    Invoke-Git -Repo $Worktree -Arguments @('checkout', '--quiet', $Into) | Out-Null
    Write-Host "$Into fast-forwarded to $($candidate.Substring(0, 10))."
    if (-not (Invoke-VerifyStep $candidate $entry "$Into after the merge")) { Undo-Merge $candidate $entry; return 1 }
    & $register mark -Id $Order -Status merged -Sha $candidate -Note "merged onto $Into as $($candidate.Substring(0, 10))" 6>$null | Out-Null
    Write-Host "MERGED: $Order is on $Into as $($candidate.Substring(0, 10)), green before and after."
    return 0
}

if ($Background) { Submit-Merge; exit 0 }
$mutex = Enter-NamedMutex -Name 'Global\MuseeMerge' -TimeoutMilliseconds 1000
if ($null -eq $mutex) { Write-Host 'Another merge is running; merges go one at a time. Try again when it ends.'; exit 1 }
try { $code = Invoke-Merge } finally { Exit-NamedMutex $mutex }
exit $code
