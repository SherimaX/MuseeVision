<#
status.ps1: generates the Register's STATUS.md from its records (BUILD_PLAN 5.4). Nobody writes STATUS.md by hand.

It reads every order file, gap, inspection, verify stamp and Unreal job record in the Register and writes one page:
each order's state, returns, open gaps, findings, last verify, Unreal queue wait and run time, tokens and age; the
gaps waiting for the Architect; and today's Unreal queue. The page is committed like any other Register entry.

  status.ps1            writes and commits STATUS.md, then prints its path and a one-line summary
  status.ps1 -Print     prints the page without writing it
#>
[CmdletBinding()]
param([switch] $Print)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
$settings = Get-WorkshopSettings
$register = $settings.Register
if (-not (Test-RegisterPresent $register)) { throw "There is no Register at $register yet. Create it with: register.ps1 init" }

function Get-RecordFiles([string] $Folder, [string] $Filter) {
    $path = Join-Path $register $Folder
    if (-not (Test-Path -LiteralPath $path)) { return @() }
    return @(Get-ChildItem -LiteralPath $path -Filter $Filter -File)
}

function Get-HistoryTime([string] $Entry) {
    <# The time at the start of an order history line ("2026-09-26 18:05 - building"), or $null. #>
    if ($Entry -match '^(\d{4}-\d\d-\d\d \d\d:\d\d)') { return [DateTime]::ParseExact($Matches[1], 'yyyy-MM-dd HH:mm', $null) }
    return $null
}

function Format-Hours([TimeSpan] $Span) {
    if ($Span.TotalDays -ge 1) { return '{0:N1} d' -f $Span.TotalDays }
    if ($Span.TotalHours -ge 1) { return '{0:N1} h' -f $Span.TotalHours }
    return '{0} min' -f [int]$Span.TotalMinutes
}

function Get-BuildTime($Order) {
    <# Wall time from the first 'building' to the last 'built' (or to now while it is still being built). #>
    $start = $null; $end = $null
    foreach ($entry in $Order.History) {
        $time = Get-HistoryTime $entry
        if (-not $start -and $entry -match ' - building') { $start = $time }
        if ($entry -match ' - built') { $end = $time }
    }
    if (-not $start) { return '' }
    if (-not $end -or $end -lt $start) { $end = Get-Date }
    return Format-Hours ($end - $start)
}

function Get-OrderRows($Jobs, $Stamps, $Gaps, $Findings) {
    $rows = @()
    foreach ($file in (Get-RecordFiles 'orders' '*.md')) {
        $order = ConvertFrom-OrderText (Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8)
        $id = $order.Fields['id']
        $ownJobs = @($Jobs | Where-Object { $_.order -eq $id })
        $queue = ($ownJobs | Measure-Object -Property queueSeconds -Sum).Sum
        $run = ($ownJobs | Measure-Object -Property runSeconds -Sum).Sum
        $ownRuns = @($Stamps | Where-Object { $_.order -eq $id })
        $sameProfile = @($ownRuns | Where-Object { $_.profile -eq $order.Fields['verify profile'] })
        if ($sameProfile.Count) { $ownRuns = $sameProfile }
        $stamp = @($ownRuns | Select-Object -Last 1)
        $verify = ''
        if ($stamp.Count) { $verify = '{0} {1} ({2})' -f $stamp[0].sha.Substring(0, 8), $(if ($stamp[0].green) { 'green' } else { 'red' }), $stamp[0].profile }
        $issued = $null
        if ($order.History.Count) { $issued = Get-HistoryTime $order.History[0] }
        $rows += [pscustomobject]@{
            Id = $id; Title = $order.Fields['title']; Status = $order.Fields['status']; Slot = $order.Fields['slot']
            Profile = $order.Fields['verify profile']; Returns = $order.Fields['returns']
            OpenGaps = @($Gaps | Where-Object { $_.Order -eq $id -and $_.Open }).Count
            Findings = $(if ($Findings.ContainsKey($id)) { $Findings[$id] } else { '' })
            Verify = $verify; Queue = [TimeSpan]::FromSeconds([double]$queue); Run = [TimeSpan]::FromSeconds([double]$run)
            Tokens = $order.Fields['tokens']; Age = $(if ($issued) { Format-Hours ((Get-Date) - $issued) } else { '' })
            Building = Get-BuildTime $order
        }
    }
    return $rows
}

function Get-GapRecords {
    foreach ($file in (Get-RecordFiles 'gaps' '*.md')) {
        $text = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8
        $fields = @{}
        foreach ($line in ($text -split "`r?`n")) { if ($line -match '^(order|against|status|filed|needed): (.*)$') { $fields[$Matches[1]] = $Matches[2] } }
        [pscustomobject]@{ Id = $file.BaseName; Order = $fields['order']; Against = $fields['against']; Open = ($fields['status'] -eq 'open')
            Filed = $fields['filed']; Needed = $fields['needed'] }
    }
}

function Get-FindingCounts {
    <# Blocker and major findings per order, counted over all its inspections ("[blocker]" and "[major]" lines). #>
    $counts = @{}
    foreach ($file in (Get-RecordFiles 'inspections' '*.md')) {
        if ($file.BaseName -notmatch '^(.+)-i\d+$') { continue }
        $order = $Matches[1]
        $text = Get-Content -LiteralPath $file.FullName -Raw -Encoding UTF8
        $blockers = ([regex]::Matches($text, '\[blocker\]')).Count
        $majors = ([regex]::Matches($text, '\[major\]')).Count
        $prior = $(if ($counts.ContainsKey($order)) { $counts[$order] } else { @(0, 0) })
        $counts[$order] = @(($prior[0] + $blockers), ($prior[1] + $majors))
    }
    $text = @{}
    foreach ($order in $counts.Keys) { $text[$order] = '{0} blocker, {1} major' -f $counts[$order][0], $counts[$order][1] }
    return $text
}

function Get-StampRuns {
    <# Every verify run in the Register, oldest first. #>
    $runs = foreach ($file in (Get-RecordFiles 'verify' 'verify-*.json')) {
        foreach ($run in @((Read-JsonFile $file.FullName).runs)) { $run }
    }
    return @($runs | Sort-Object { [string]$_.finished })
}

function Get-QueueSummary($Jobs) {
    $today = (Get-Date).Date
    $lines = @()
    $recent = @($Jobs | Where-Object { $_.finished -and ([DateTime]::Parse($_.finished).ToLocalTime().Date -eq $today) })
    if (-not $recent.Count) { return @('No Unreal jobs finished today.') }
    foreach ($group in ($recent | Group-Object slot)) {
        $wait = $group.Group | Measure-Object -Property queueSeconds -Sum -Maximum
        $run = ($group.Group | Measure-Object -Property runSeconds -Sum).Sum
        $lines += '- {0}: {1} job(s); queue wait {2} in all, longest {3}; Unreal ran {4}.' -f $group.Name, $group.Count,
            (Format-Hours ([TimeSpan]::FromSeconds($wait.Sum))), (Format-Hours ([TimeSpan]::FromSeconds($wait.Maximum))),
            (Format-Hours ([TimeSpan]::FromSeconds($run)))
    }
    return $lines
}

function Get-StatusPage {
    $jobs = @(Get-RecordFiles 'jobs' '*.json' | ForEach-Object { Read-JsonFile $_.FullName })
    $gaps = @(Get-GapRecords)
    $rows = @(Get-OrderRows $jobs @(Get-StampRuns) $gaps (Get-FindingCounts))
    $lines = @('# Status', '', "Generated by workshop/status.ps1 at $(Get-LocalStamp) from the Register's records. Nobody writes it by hand.", '',
        '## Orders', '')
    if ($rows.Count) {
        $lines += '| Order | Title | Status | Slot | Profile | Returns | Open gaps | Findings | Last verify | Unreal wait | Unreal run | Tokens | Building | Age |'
        $lines += '|---|---|---|---|---|---|---|---|---|---|---|---|---|---|'
        foreach ($row in $rows) {
            $lines += '| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} | {10} | {11} | {12} | {13} |' -f $row.Id, $row.Title,
                $row.Status, $row.Slot, $row.Profile, $row.Returns, $row.OpenGaps, $row.Findings, $row.Verify,
                (Format-Hours $row.Queue), (Format-Hours $row.Run), $row.Tokens, $row.Building, $row.Age
        }
    } else { $lines += 'No orders yet.' }
    $lines += @('', '## Gaps waiting for the Architect', '')
    $open = @($gaps | Where-Object { $_.Open })
    if ($open.Count) { foreach ($gap in $open) { $lines += "- **$($gap.Id)** (filed $($gap.Filed)), against $($gap.Against): $($gap.Needed)" } }
    else { $lines += 'None.' }
    $lines += @('', "## Today's Unreal queue", '') + @(Get-QueueSummary $jobs)
    return [pscustomobject]@{ Text = (($lines -join "`n") + "`n"); Orders = $rows.Count; OpenGaps = $open.Count }
}

$page = Get-StatusPage
if ($Print) { Write-Output $page.Text; exit 0 }
$paths = Invoke-RegisterWrite -Register $register -Message 'Register: STATUS' -Compose { [ordered]@{ 'STATUS.md' = $page.Text } }
Write-Host ("Wrote {0}: {1} orders, {2} gaps waiting for the Architect." -f (Join-Path $register 'STATUS.md'), $page.Orders, $page.OpenGaps)
