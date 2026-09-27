<#
register.ps1: writes the Register, the book in which the state of the work is recorded (BUILD_PLAN 6, Decision D-009).

The Register is a separate worktree, C:\mv\register, on its own orphan branch 'register'. It is written only through
this script: every write takes the machine-wide mutex Global\MuseeRegister, writes one file per entry and commits it
at once, so parallel builders never collide. It refuses to write while the Register holds changes that no tool
committed (edits made by hand), and says which files.

  register.ps1 init
      Creates the Register (git worktree add --orphan -b register C:\mv\register) and its folders:
      orders gaps inspections handoff verify jobs defects critique gates sessions.
  register.ps1 order -Id F-FORM-02 -Title "<title>" -Owner Form -Slot f-form -Profile code|physical|measured
                     -Allowed <glob>,<glob> [-ReadFirst ...] [-AcceptanceTests "<path> sha256: <hex>",...] [-Build <text>]
                     [-DoneWhen <text>] [-Role "Master Builder"] [-Model <tier>] [-ContractVersion <v>|current] [-Update]
      Issues an order (orders/<id>.md, status issued). With -Update, changes only the fields given.
  register.ps1 mark -Id <order> -Status <state> [-Note <text>] [-Sha <commit>]
      Moves an order to a state: drafted issued building built verified "in inspection" passed merged blocked partial
      stale returned. Returned adds one to its returns; merged records -Sha.
  register.ps1 gap -Order <id> -Against "<system> / <header>" -Needed <text> -Considered <text> -Blocked <text>
                   -Continuing <text> [-Partial]
      Files gaps/<order>-g<k>.md and marks the order blocked (partial with -Partial).
  register.ps1 answer -Gap <order>-g<k> -Triage exists|connector-missing|contract-wrong -Answer <text>
  register.ps1 inspection -Order <id> -Verdict pass|return -Stamp <sha> (-Text <text> | -FromFile <path>)
      Files inspections/<order>-i<k>.md and marks the order passed or returned.
  register.ps1 handoff -Order <id> -Done <text> -Next <text> [-Questions <text>] [-LastVerify <text>]
  register.ps1 stamp -FromFile <verify run json>     (verify.ps1 writes verify/verify-<sha>.json this way)
  register.ps1 job -FromFile <job record json>       (unreal.ps1 writes jobs/<job>.json this way)
  register.ps1 defect -Slug <kebab-name> -Symptom <text> -Truth <text> -Remedy "<fix in the owning system, order>" -Check <text>
  register.ps1 critique -Words <owner's words, verbatim> [-FromFile <path>] [-Images <paths>] [-Routing <where each item went>]
  register.ps1 gate -Gate A|A-prime|B|C (-Text <packet> | -FromFile <path> | -Decision <owner's words, verbatim>)
  register.ps1 session (-Text <log> | -FromFile <path>)
  register.ps1 tokens -Id <order> -Tokens <count>    Adds tokens used to the order's metrics.
  register.ps1 contract-version [-Ref renaissance]   Prints the contract version (hash of every Public/ file).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateSet('init', 'order', 'mark', 'gap', 'answer', 'inspection', 'handoff', 'stamp', 'job', 'defect', 'critique',
        'gate', 'session', 'tokens', 'contract-version')] [string] $Command,
    [string] $Id = '', [string] $Order = '', [string] $Title = '', [string] $Owner = '', [string] $Slot = '',
    [string] $Profile = '', [string[]] $Allowed = @(), [string[]] $ReadFirst = @(), [string[]] $AcceptanceTests = @(),
    [string] $Build = '', [string] $DoneWhen = '', [string] $Role = '', [string] $Model = '', [string] $ContractVersion = '',
    [switch] $Update, [string] $Status = '', [string] $Note = '', [string] $Sha = '',
    [string] $Against = '', [string] $Needed = '', [string] $Considered = '', [string] $Blocked = '', [string] $Continuing = '',
    [switch] $Partial, [string] $Gap = '', [string] $Triage = '', [string] $Answer = '',
    [string] $Verdict = '', [string] $Stamp = '', [string] $Text = '', [string] $FromFile = '',
    [string] $Done = '', [string] $Next = '', [string] $Questions = '', [string] $LastVerify = '',
    [string] $Slug = '', [string] $Symptom = '', [string] $Truth = '', [string] $Remedy = '', [string] $Check = '',
    [string] $Words = '', [string[]] $Images = @(), [string] $Routing = '', [string] $Gate = '', [string] $Decision = '',
    [int] $Tokens = 0, [string] $Ref = 'renaissance'
)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
$settings = Get-WorkshopSettings
$register = $settings.Register
$folders = @('orders', 'gaps', 'inspections', 'handoff', 'verify', 'jobs', 'defects', 'critique', 'gates', 'sessions')

function Assert-Given([string] $Name, $Value) {
    if (-not $Value) { throw "$Command needs -$Name." }
}

function Get-BodyText {
    <# The entry's body: -FromFile's contents, or -Text. #>
    if ($FromFile) { return (Get-Content -LiteralPath $FromFile -Raw -Encoding UTF8) }
    return $Text
}

function Get-NextNumber([string] $Folder, [string] $Prefix) {
    <# The next free k for files named <Prefix><k>.md (or folders <Prefix><k>) in a Register folder. #>
    $highest = 0
    $path = Join-Path $register $Folder
    if (Test-Path -LiteralPath $path) {
        foreach ($item in Get-ChildItem -LiteralPath $path) {
            $name = [IO.Path]::GetFileNameWithoutExtension($item.Name)
            if ($name.StartsWith($Prefix) -and $name.Substring($Prefix.Length) -match '^\d+$') {
                $highest = [Math]::Max($highest, [int]$name.Substring($Prefix.Length))
            }
        }
    }
    return $highest + 1
}

function Write-Done([string[]] $Paths) {
    Write-Host ("Recorded in the Register: " + ($Paths -join ', '))
}

# ---- init ----------------------------------------------------------------------------------------------------------

function Initialize-Register {
    if (-not (Test-RegisterPresent $register)) {
        if (Test-Path -LiteralPath $register) { throw "$register exists but isn't a git worktree; move it aside first." }
        Invoke-Git -Repo $settings.Integration -Arguments @('worktree', 'add', '--orphan', '-b', 'register', $register) | Out-Null
        Write-Host "Created the Register worktree at $register on the orphan branch 'register'."
    }
    $branch = (Invoke-Git -Repo $register -Arguments @('symbolic-ref', '--short', 'HEAD')).Trim()
    if ($branch -ne 'register') { throw "$register is on branch '$branch', not 'register'." }
    if (Test-Path -LiteralPath (Join-Path $register 'README.md')) { Write-Host 'The Register is already laid out.'; return }
    $paths = Invoke-RegisterWrite -Register $register -Message 'Register: the book opens' -Compose {
        $entries = [ordered]@{ 'README.md' = (Get-RegisterReadme) }
        foreach ($folder in $folders) { $entries["$folder/.gitkeep"] = '' }
        $entries
    }
    Write-Done $paths
}

function Get-RegisterReadme {
    return @'
# The Register

The state of the work on Musee Vision: orders, gaps, inspections, handoffs, verify stamps, Unreal jobs, defects, the
owner's critiques, gate packets and Architect session logs. Knowledge (decisions, defects, reviews) lives in the
project's Journal; the Register holds the process (Decision D-009).

It is written only through `workshop/register.ps1` (and the tools that call it), one file per entry, each committed
at once under the machine-wide mutex `Global\MuseeRegister`. Nothing here is edited by hand; `STATUS.md` is
generated by `workshop/status.ps1`.

| Folder | One file per |
|---|---|
| `orders/<id>.md` | work order: status, contract version, slot, allowed paths, history |
| `gaps/<id>-g<k>.md` | gap a builder filed, and the Architect's answer |
| `inspections/<id>-i<k>.md` | inspection |
| `handoff/<id>.md` | order's handoff: done, next, open questions, last verify |
| `verify/verify-<sha>.json` | commit verified, with every run of verify on it |
| `jobs/<job>.json` | Unreal job: queue wait, run time, outcome |
| `defects/<date>-<slug>.md` | defect: symptom, broken truth, fix, the check that now prevents it |
| `critique/<date>/` | owner walk: images, the owner's words verbatim, where each item went |
| `gates/<gate>.md` | gate packet and the owner's decisions, quoted |
| `sessions/<date>-<n>.md` | Architect session log |
'@
}

# ---- Orders ----------------------------------------------------------------------------------------------------------

function Split-List([string[]] $Items) {
    <# Lists arrive as one comma-separated string when a script is run with -File; either form is accepted. #>
    return @($Items | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ })
}

function Get-OrderFields {
    <# The order fields given on the command line, in the order file's own names. #>
    $given = [ordered]@{}
    if ($Title) { $given['title'] = $Title }
    if ($Owner) { $given['owner'] = $Owner }
    if ($Slot) { $given['slot'] = $Slot }
    if ($Role) { $given['role'] = $Role }
    if ($Model) { $given['model'] = $Model }
    if ($Profile) {
        if ($Profile -notin @('code', 'physical', 'measured')) { throw '-Profile must be code, physical or measured.' }
        $given['verify profile'] = $Profile
    }
    if ($ContractVersion) {
        $version = $ContractVersion
        if ($version -eq 'current') { $version = Get-ContractVersion $settings.Integration $Ref }
        $given['contract version'] = $version
    }
    if ($Allowed.Count) { $given['allowed paths'] = Split-List $Allowed }
    if ($AcceptanceTests.Count) { $given['acceptance tests (locked)'] = Split-List $AcceptanceTests }
    if ($ReadFirst.Count) { $given['read first'] = Split-List $ReadFirst }
    if ($Build) { $given['build'] = $Build }
    if ($DoneWhen) { $given['done when'] = $DoneWhen }
    return $given
}

function Save-Order {
    Assert-Given 'Id' $Id
    if ($Id -notmatch '^[A-Z][A-Z0-9]*(-[A-Z0-9]+)+$') { throw "Order ids look like F-FORM-02 (got '$Id')." }
    $given = Get-OrderFields
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: order $Id" -Compose {
        $existing = Read-OrderFile $register $Id
        if ($existing -and -not $Update) { throw "Order $Id already exists; pass -Update to change it." }
        if (-not $existing -and $Update) { throw "There is no order $Id to update." }
        if ($existing) {
            foreach ($key in $given.Keys) { $existing.Fields[$key] = $given[$key] }
            $existing.History.Add("$(Get-LocalStamp) - updated: $($given.Keys -join ', ')")
            $order = $existing
        } else {
            $required = [ordered]@{ 'title' = 'Title'; 'owner' = 'Owner'; 'allowed paths' = 'Allowed' }
            foreach ($key in $required.Keys) { if (-not $given.Contains($key)) { throw "A new order needs -$($required[$key])." } }
            $fields = [ordered]@{ id = $Id; title = $given['title']; owner = $given['owner']; branch = "work/$Id"; status = 'issued'
                'verify profile' = 'code'; 'contract version' = 'none'; returns = '0'; tokens = '0' }
            foreach ($key in $given.Keys) { $fields[$key] = $given[$key] }
            $order = [pscustomobject]@{ Fields = $fields; History = (New-Object System.Collections.Generic.List[string]); Notes = '' }
            $order.History.Add("$(Get-LocalStamp) - issued")
        }
        [ordered]@{ "orders/$Id.md" = (ConvertTo-OrderText $order) }
    }
    Write-Done $paths
}

function Set-OrderState($OrderEntry, [string] $State, [string] $Why) {
    <# Moves an order to a state, counting returns, and notes it in the order's history. #>
    if ($State -notin (Get-OrderStates)) { throw "'$State' isn't an order state. The states are: $((Get-OrderStates) -join ', ')." }
    $OrderEntry.Fields['status'] = $State
    if ($State -eq 'returned') { $OrderEntry.Fields['returns'] = [string]([int]$OrderEntry.Fields['returns'] + 1) }
    $line = "$(Get-LocalStamp) - $State"
    if ($Why) { $line += " - $Why" }
    $OrderEntry.History.Add($line)
}

function Set-OrderMark {
    Assert-Given 'Id' $Id
    Assert-Given 'Status' $Status
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: order $Id $Status" -Compose {
        $entry = Read-OrderFile $register $Id
        if (-not $entry) { throw "There is no order $Id in the Register." }
        Set-OrderState $entry $Status $Note
        if ($Status -eq 'merged' -and $Sha) { $entry.Fields['merged'] = $Sha }
        [ordered]@{ "orders/$Id.md" = (ConvertTo-OrderText $entry) }
    }
    Write-Done $paths
}

function Add-OrderTokens {
    Assert-Given 'Id' $Id
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: order $Id tokens" -Compose {
        $entry = Read-OrderFile $register $Id
        if (-not $entry) { throw "There is no order $Id in the Register." }
        $entry.Fields['tokens'] = [string]([long]$entry.Fields['tokens'] + $Tokens)
        [ordered]@{ "orders/$Id.md" = (ConvertTo-OrderText $entry) }
    }
    Write-Done $paths
}

# ---- Gaps, answers, inspections, handoffs ----------------------------------------------------------------------------

function Add-Gap {
    foreach ($name in @('Order', 'Against', 'Needed', 'Considered', 'Blocked', 'Continuing')) { Assert-Given $name (Get-Variable $name -ValueOnly) }
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: gap from $Order" -Compose {
        $gapId = "$Order-g$(Get-NextNumber 'gaps' "$Order-g")"
        $text = @(
            "# $gapId - against $Against", '',
            "order: $Order", "against: $Against", 'status: open', "filed: $(Get-LocalStamp)", '',
            "needed: $Needed", "considered: $Considered", "blocked: $Blocked", "continuing: $Continuing", ''
        ) -join "`n"
        $entries = [ordered]@{ "gaps/$gapId.md" = $text }
        $entry = Read-OrderFile $register $Order
        if ($entry -and $entry.Fields['status'] -ne 'merged') {
            Set-OrderState $entry $(if ($Partial) { 'partial' } else { 'blocked' }) "gap $gapId"
            $entries["orders/$Order.md"] = ConvertTo-OrderText $entry
        }
        $entries
    }
    Write-Done $paths
}

function Add-GapAnswer {
    Assert-Given 'Gap' $Gap; Assert-Given 'Answer' $Answer
    if ($Triage -notin @('exists', 'connector-missing', 'contract-wrong')) { throw '-Triage must be exists, connector-missing or contract-wrong.' }
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: answer to $Gap" -Compose {
        $path = Join-Path $register "gaps\$Gap.md"
        if (-not (Test-Path -LiteralPath $path)) { throw "There is no gap $Gap in the Register." }
        $text = (Get-Content -LiteralPath $path -Raw -Encoding UTF8) -replace '(?m)^status: open$', "status: answered ($Triage)"
        $text = $text.TrimEnd() + "`n`n## Answer - $(Get-LocalStamp) - $Triage`n`n$Answer`n"
        [ordered]@{ "gaps/$Gap.md" = $text }
    }
    Write-Done $paths
}

function Add-Inspection {
    Assert-Given 'Order' $Order; Assert-Given 'Stamp' $Stamp
    if ($Verdict -notin @('pass', 'return')) { throw '-Verdict must be pass or return.' }
    $body = Get-BodyText
    Assert-Given 'Text or -FromFile' $body
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: inspection of $Order ($Verdict)" -Compose {
        $inspectionId = "$Order-i$(Get-NextNumber 'inspections' "$Order-i")"
        $text = "# $inspectionId`n`nverdict: $Verdict`nverify stamp: $Stamp`nfiled: $(Get-LocalStamp)`n`n$($body.Trim())`n"
        $entries = [ordered]@{ "inspections/$inspectionId.md" = $text }
        $entry = Read-OrderFile $register $Order
        if ($entry) {
            Set-OrderState $entry $(if ($Verdict -eq 'pass') { 'passed' } else { 'returned' }) "inspection $inspectionId"
            $entries["orders/$Order.md"] = ConvertTo-OrderText $entry
        }
        $entries
    }
    Write-Done $paths
}

function Save-Handoff {
    Assert-Given 'Order' $Order; Assert-Given 'Done' $Done; Assert-Given 'Next' $Next
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: handoff for $Order" -Compose {
        $text = @("# Handoff - $Order", '', "updated: $(Get-LocalStamp)", '', '## Done', '', $Done, '', '## Next', '', $Next, '',
            '## Open questions', '', $(if ($Questions) { $Questions } else { 'none' }), '',
            '## Last verify', '', $(if ($LastVerify) { $LastVerify } else { 'not run yet' }), '') -join "`n"
        [ordered]@{ "handoff/$Order.md" = $text }
    }
    Write-Done $paths
}

# ---- Stamps and jobs (written by verify.ps1 and unreal.ps1) -----------------------------------------------------------

function Save-Stamp {
    Assert-Given 'FromFile' $FromFile
    $run = Read-JsonFile $FromFile
    if ($run.sha -notmatch '^[0-9a-f]{40}$') { throw 'A verify run needs the full 40-character sha of the commit it verified.' }
    $verdict = $(if ($run.green) { 'green' } else { 'red' })
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: verify $($run.sha.Substring(0, 10)) $($run.profile) $verdict" -Compose {
        $relative = "verify/verify-$($run.sha).json"
        $path = Join-Path $register $relative
        $runs = @()
        if (Test-Path -LiteralPath $path) { $runs = @((Read-JsonFile $path).runs) }
        $runs += $run
        [ordered]@{ $relative = (ConvertTo-Json -InputObject ([ordered]@{ sha = $run.sha; runs = $runs }) -Depth 12) }
    }
    Write-Done $paths
}

function Save-JobRecord {
    Assert-Given 'FromFile' $FromFile
    $record = Read-JsonFile $FromFile
    if ($record.id -notmatch '^[0-9]{8}-[0-9]{6}-[a-z]+-[0-9a-f]{4}$') { throw "'$($record.id)' isn't a job id." }
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: job $($record.id) $($record.outcome)" -Compose {
        [ordered]@{ "jobs/$($record.id).json" = (ConvertTo-Json -InputObject $record -Depth 12) }
    }
    Write-Done $paths
}

# ---- Defects, critiques, gates, sessions -------------------------------------------------------------------------

function Add-Defect {
    foreach ($name in @('Slug', 'Symptom', 'Truth', 'Remedy', 'Check')) { Assert-Given $name (Get-Variable $name -ValueOnly) }
    if ($Slug -notmatch '^[a-z0-9]+(-[a-z0-9]+)*$') { throw "-Slug is a kebab-case name, like light-streak-on-dome (got '$Slug')." }
    $date = (Get-Date).ToString('yyyy-MM-dd')
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: defect $date-$Slug" -Compose {
        $relative = "defects/$date-$Slug.md"
        if (Test-Path -LiteralPath (Join-Path $register $relative)) { throw "Defect $date-$Slug is already recorded." }
        $text = "# $date-$Slug`n`nsymptom: $Symptom`nbroken truth: $Truth`nfix: $Remedy`ncheck that now prevents it: $Check`n"
        [ordered]@{ $relative = $text }
    }
    Write-Done $paths
}

function Add-Critique {
    $spoken = $(if ($FromFile) { Get-Content -LiteralPath $FromFile -Raw -Encoding UTF8 } else { $Words })
    Assert-Given 'Words (the owner''s words, verbatim)' $spoken
    $Images = Split-List $Images
    foreach ($image in $Images) { if (-not (Test-Path -LiteralPath $image)) { throw "There is no image $image." } }
    $date = (Get-Date).ToString('yyyy-MM-dd')
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: critique $date" -Compose {
        $folder = $date
        $number = 1
        while (Test-Path -LiteralPath (Join-Path $register "critique\$folder")) { $number++; $folder = "$date-$number" }
        $entries = [ordered]@{}
        $names = @()
        foreach ($image in $Images) {
            $name = [IO.Path]::GetFileName($image); $names += $name
            $entries["critique/$folder/$name"] = @{ From = (Resolve-Path -LiteralPath $image).Path }
        }
        $quoted = (($spoken.Trim() -split "`r?`n") | ForEach-Object { "> $_" }) -join "`n"
        $text = "# Owner walk - $folder`n`n## The owner's words`n`n$quoted`n`n## Images`n`n" +
            $(if ($names.Count) { ($names | ForEach-Object { "- $_" }) -join "`n" } else { 'none' }) +
            "`n`n## Where each item went`n`n$(if ($Routing) { $Routing } else { 'not yet routed' })`n"
        $entries["critique/$folder/walk.md"] = $text
        $entries
    }
    Write-Done $paths
}

function Save-Gate {
    Assert-Given 'Gate' $Gate
    $name = @{ 'A' = 'A'; "A'" = 'A-prime'; 'A-prime' = 'A-prime'; 'B' = 'B'; 'C' = 'C' }[$Gate]
    if (-not $name) { throw '-Gate must be A, A-prime, B or C.' }
    $packet = Get-BodyText
    if (-not $packet -and -not $Decision) { throw 'gate needs the packet (-Text or -FromFile) or the owner''s -Decision.' }
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: gate $name" -Compose {
        $path = Join-Path $register "gates\$name.md"
        $decisions = ''
        $body = "(no packet yet)"
        if (Test-Path -LiteralPath $path) {
            $existing = Get-Content -LiteralPath $path -Raw -Encoding UTF8
            $split = $existing -split '(?m)^## The owner''s decisions\s*$', 2
            $body = ($split[0] -replace '^# Gate [^\n]*\n', '').Trim()
            if ($split.Count -gt 1 -and $split[1].Trim() -ne 'none yet') { $decisions = $split[1].Trim() }
        }
        if ($packet) { $body = $packet.Trim() }
        if ($Decision) {
            $quoted = (($Decision.Trim() -split "`r?`n") | ForEach-Object { "> $_" }) -join "`n"
            $decisions = ($decisions + "`n`n### $(Get-LocalStamp)`n`n$quoted").Trim()
        }
        [ordered]@{ "gates/$name.md" = "# Gate $name`n`n$body`n`n## The owner's decisions`n`n$(if ($decisions) { $decisions } else { 'none yet' })`n" }
    }
    Write-Done $paths
}

function Add-Session {
    $body = Get-BodyText
    Assert-Given 'Text or -FromFile' $body
    $date = (Get-Date).ToString('yyyy-MM-dd')
    $paths = Invoke-RegisterWrite -Register $register -Message "Register: session log $date" -Compose {
        $sessionId = "$date-$(Get-NextNumber 'sessions' "$date-")"
        [ordered]@{ "sessions/$sessionId.md" = "# Architect session $sessionId`n`n$($body.Trim())`n" }
    }
    Write-Done $paths
}

switch ($Command) {
    'init' { Initialize-Register }
    'order' { Save-Order }
    'mark' { Set-OrderMark }
    'gap' { Add-Gap }
    'answer' { Add-GapAnswer }
    'inspection' { Add-Inspection }
    'handoff' { Save-Handoff }
    'stamp' { Save-Stamp }
    'job' { Save-JobRecord }
    'defect' { Add-Defect }
    'critique' { Add-Critique }
    'gate' { Save-Gate }
    'session' { Add-Session }
    'tokens' { Add-OrderTokens }
    'contract-version' { Write-Output (Get-ContractVersion $settings.Integration $Ref) }
}
