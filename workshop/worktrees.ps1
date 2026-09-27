<#
worktrees.ps1: makes, moves and removes builder slots (BUILD_PLAN 5.1).

A slot is a persistent git worktree at C:\mv\wt\<slot> with a cone sparse checkout of MuseeVision, docs and workshop
and core.longpaths set before anything is checked out. Slots persist so each keeps its own warm Unreal build; an
order moves into a slot on its own branch, work/<order-id>, cut from the renaissance tip. The integration slot is
never touched here, the owner's checkout never, and no branch is ever deleted.

  worktrees.ps1 create -Slot <name> -Order <id> [-From renaissance]
      Makes C:\mv\wt\<name> on work/<id>: a new branch from the tip of -From, or the order's existing branch (a
      returned order resumes where it stopped).
  worktrees.ps1 reset -Slot <name> -Order <id> [-From renaissance]
      Moves an existing, clean slot to work/<id> the same way, keeping its build products (so it compiles warm).
  worktrees.ps1 remove -Slot <name>
      Removes a clean slot's worktree and folder. Its branch stays.
  worktrees.ps1 list
      Every slot with its branch, commit and whether it has uncommitted changes.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)] [ValidateSet('create', 'reset', 'remove', 'list')] [string] $Command,
    [string] $Slot = '',
    [string] $Order = '',
    [string] $From = 'renaissance'
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
$settings = Get-WorkshopSettings
$repository = $settings.Integration
$sparse = @('MuseeVision', 'docs', 'workshop')

function Get-SlotPath {
    if ($Slot -notmatch '^[a-z0-9][a-z0-9-]*$') { throw "-Slot is a short lowercase name like mb1 or f-form (got '$Slot')." }
    if ($Slot -eq 'integration') { throw 'The integration slot is the merge tool''s; worktrees.ps1 never touches it.' }
    $path = Join-Path $settings.SlotRoot $Slot
    Assert-NotOwnerCheckout $path
    return $path
}

function Get-OrderBranch {
    if ($Order -notmatch '^[A-Z][A-Z0-9]*(-[A-Z0-9]+)+$') { throw "-Order is an order id like F-FORM-02 (got '$Order')." }
    return "work/$Order"
}

function Test-BranchExists([string] $Branch) {
    $result = Invoke-Git -Repo $repository -Arguments @('show-ref', '--verify', '--quiet', "refs/heads/$Branch") -AllowFailure
    return ($result.ExitCode -eq 0)
}

function Assert-SlotClean([string] $Path) {
    <# A slot moves or goes only when it has nothing uncommitted: tracked changes or untracked files that git sees. #>
    $changes = @(Get-GitLines (Invoke-Git -Repo $Path -Arguments @('status', '--porcelain')))
    if ($changes.Count) {
        $list = ($changes | Select-Object -First 10) -join "`n  "
        throw "Refused: $Path has uncommitted work:`n  $list`nThe builder commits or the Architect decides; nothing is thrown away here."
    }
}

function Set-SlotShape([string] $Path) {
    <# Long paths and the cone sparse checkout, set on this worktree alone. #>
    Invoke-Git -Repo $Path -Arguments @('config', '--worktree', 'core.longpaths', 'true') | Out-Null
    Invoke-Git -Repo $Path -Arguments (@('sparse-checkout', 'set', '--cone') + $sparse) | Out-Null
}

function Get-SlotSummary([string] $Path) {
    $head = Invoke-Git -Repo $Path -Arguments @('log', '-1', '--format=%h %s')
    $branch = Invoke-Git -Repo $Path -Arguments @('rev-parse', '--abbrev-ref', 'HEAD')
    return "$Path is on $branch at $head"
}

function Add-SlotWorktree {
    $path = Get-SlotPath
    $branch = Get-OrderBranch
    if (Test-Path -LiteralPath $path) { throw "$path already exists; use reset to move it to another order." }
    if (Test-BranchExists $branch) {
        Invoke-Git -Repo $repository -Arguments @('worktree', 'add', '--no-checkout', $path, $branch) | Out-Null
        $how = "the order's existing branch $branch"
    } else {
        $tip = Invoke-Git -Repo $repository -Arguments @('rev-parse', '--short', $From)
        Invoke-Git -Repo $repository -Arguments @('worktree', 'add', '--no-checkout', '-b', $branch, $path, $From) | Out-Null
        $how = "a new branch $branch from the tip of $From ($tip)"
    }
    Invoke-Git -Repo $path -Arguments @('config', '--worktree', 'core.sparseCheckout', 'true') | Out-Null
    Set-SlotShape $path
    Invoke-Git -Repo $path -Arguments @('checkout', $branch) | Out-Null
    Write-Host "Created the slot on $how."
    Write-Host (Get-SlotSummary $path)
}

function Reset-SlotWorktree {
    $path = Get-SlotPath
    $branch = Get-OrderBranch
    if (-not (Test-Path -LiteralPath (Join-Path $path '.git'))) { throw "There is no slot at $path; use create." }
    Assert-SlotClean $path
    Set-SlotShape $path
    if (Test-BranchExists $branch) {
        Invoke-Git -Repo $path -Arguments @('checkout', $branch) | Out-Null
        Write-Host "Moved the slot to the order's existing branch $branch."
    } else {
        Invoke-Git -Repo $path -Arguments @('checkout', '-b', $branch, $From) | Out-Null
        Write-Host "Moved the slot to a new branch $branch from the tip of $From."
    }
    Write-Host (Get-SlotSummary $path)
}

function Remove-SlotWorktree {
    $path = Get-SlotPath
    if (-not (Test-Path -LiteralPath (Join-Path $path '.git'))) { throw "There is no slot at $path." }
    Assert-SlotClean $path
    $branch = Invoke-Git -Repo $path -Arguments @('rev-parse', '--abbrev-ref', 'HEAD')
    $removed = Invoke-Git -Repo $repository -Arguments @('worktree', 'remove', $path) -AllowFailure
    if ($removed.ExitCode -ne 0) {
        # Only ignored build products (Binaries, Intermediate, Saved) can remain after the clean check above.
        Invoke-Git -Repo $repository -Arguments @('worktree', 'remove', '--force', $path) | Out-Null
    }
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
    Write-Host "Removed the slot $path. Its branch $branch is kept."
}

function Show-Slots {
    $listing = Get-GitLines (Invoke-Git -Repo $repository -Arguments @('worktree', 'list', '--porcelain'))
    $entries = @(); $entry = $null
    foreach ($line in $listing) {
        if ($line -match '^worktree (.+)$') { $entry = [ordered]@{ Path = $Matches[1]; Branch = '(detached)'; Head = '' }; $entries += , $entry }
        elseif ($line -match '^HEAD ([0-9a-f]{7})') { $entry.Head = $Matches[1] }
        elseif ($line -match '^branch refs/heads/(.+)$') { $entry.Branch = $Matches[1] }
    }
    $root = (Get-FullPath $settings.SlotRoot).ToLowerInvariant()
    foreach ($item in $entries) {
        $full = (Get-FullPath $item.Path)
        if (-not $full.ToLowerInvariant().StartsWith($root + '\')) { continue }
        $state = $(if (@(Get-GitLines (Invoke-Git -Repo $full -Arguments @('status', '--porcelain'))).Count) { 'uncommitted changes' } else { 'clean' })
        Write-Host ("{0,-28} {1,-32} {2}  {3}" -f $full, $item.Branch, $item.Head, $state)
    }
}

switch ($Command) {
    'create' { Add-SlotWorktree }
    'reset' { Reset-SlotWorktree }
    'remove' { Remove-SlotWorktree }
    'list' { Show-Slots }
}
