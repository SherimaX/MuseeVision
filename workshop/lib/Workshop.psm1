# Workshop.psm1: the pieces every workshop tool shares.
#
# The workshop tools (unreal.ps1, register.ps1, status.ps1, worktrees.ps1, verify.ps1, merge.ps1) import this module
# for four things: where everything lives on this PC, running git and other programs with their output captured,
# machine-wide named mutexes, and reading and writing the Register's order files.
#
#   Import-Module (Join-Path $PSScriptRoot 'lib\Workshop.psm1') -Force
#
# Where things live can be moved for the tools' own tests through environment variables (MUSEE_INTEGRATION,
# MUSEE_REGISTER, MUSEE_RUN, MUSEE_SLOTS, MUSEE_SLOT_NAMESPACE, MUSEE_SLOT_ROOT); nothing else reads them.

Set-StrictMode -Off
$script:Utf8 = New-Object System.Text.UTF8Encoding($false)

# The owner's own checkout. No tool ever runs git there.
$script:OwnerCheckout = 'C:\Users\anan9\Desktop\Projects\Musee Vision'

function Get-EnvironmentValue([string] $Name, [string] $Default) {
    $value = [Environment]::GetEnvironmentVariable($Name)
    if ([string]::IsNullOrWhiteSpace($value)) { return $Default }
    return $value
}

function Get-WorkshopSettings {
    <# Where everything the workshop uses lives on this PC. #>
    $workshop = Split-Path -Parent $PSScriptRoot
    $git = Get-Command git.exe -ErrorAction SilentlyContinue
    $gitPath = 'C:\Program Files\Git\cmd\git.exe'
    if ($git) { $gitPath = $git.Source }
    [pscustomobject]@{
        Workshop    = $workshop
        Integration = Get-EnvironmentValue 'MUSEE_INTEGRATION' 'C:\mv\wt\integration'
        SlotRoot    = Get-EnvironmentValue 'MUSEE_SLOT_ROOT' 'C:\mv\wt'
        Register    = Get-EnvironmentValue 'MUSEE_REGISTER' 'C:\mv\register'
        RunRoot     = Get-EnvironmentValue 'MUSEE_RUN' 'C:\mv\run'
        SlotTable   = Get-EnvironmentValue 'MUSEE_SLOTS' (Join-Path $workshop 'slots.json')
        Namespace   = Get-EnvironmentValue 'MUSEE_SLOT_NAMESPACE' 'MuseeUnreal'
        Engine      = 'C:\Program Files\Epic Games\UE_5.8'
        BuildBat    = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat'
        EditorCmd   = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        Python      = 'C:\Users\anan9\miniconda3\python.exe'
        Git         = $gitPath
        Project     = 'MuseeVision'
    }
}

function Get-FullPath([string] $Path) {
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Assert-NotOwnerCheckout([string] $Path) {
    <# Refuses any path inside the owner's own checkout, which holds their uncommitted work. #>
    $full = Get-FullPath $Path
    $owner = Get-FullPath $script:OwnerCheckout
    if ($full.Equals($owner, 'OrdinalIgnoreCase') -or $full.StartsWith($owner + '\', 'OrdinalIgnoreCase')) {
        throw "Refused: $Path is the owner's own checkout. No workshop tool runs git there."
    }
}

function ConvertTo-ArgumentString([string[]] $Arguments) {
    <# Joins arguments into one Windows command line, quoting each the way CommandLineToArgvW reads it back. #>
    $parts = foreach ($argument in $Arguments) {
        if ($argument -ne '' -and $argument -notmatch '[\s"]') { $argument; continue }
        $slash = [char]'\'
        $builder = New-Object System.Text.StringBuilder
        [void]$builder.Append('"')
        $slashes = 0
        foreach ($c in $argument.ToCharArray()) {
            if ($c -eq $slash) { $slashes++; continue }
            if ($c -eq [char]'"') { [void]$builder.Append($slash, 2 * $slashes + 1) }
            elseif ($slashes) { [void]$builder.Append($slash, $slashes) }
            $slashes = 0
            [void]$builder.Append($c)
        }
        [void]$builder.Append($slash, 2 * $slashes)
        [void]$builder.Append('"')
        $builder.ToString()
    }
    return ($parts -join ' ')
}

function Invoke-Native {
    <# Runs a program to completion and returns its exit code, standard output and standard error as text. #>
    param([Parameter(Mandatory)] [string] $FilePath, [string[]] $Arguments = @(), [string] $WorkingDirectory = '')
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $FilePath
    $info.Arguments = ConvertTo-ArgumentString $Arguments
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = $script:Utf8
    $info.StandardErrorEncoding = $script:Utf8
    if ($WorkingDirectory) { $info.WorkingDirectory = $WorkingDirectory }
    $process = [System.Diagnostics.Process]::Start($info)
    $output = $process.StandardOutput.ReadToEndAsync()
    $errors = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    [pscustomobject]@{ ExitCode = $process.ExitCode; Output = $output.Result; Error = $errors.Result }
}

function Assert-GitCommandAllowed([string[]] $Arguments) {
    <# The sweeping git commands the Craft forbids are refused here, so no tool can run one by mistake. #>
    $verb = $Arguments[0]
    $rest = @($Arguments | Select-Object -Skip 1)
    $refused = $false
    switch ($verb) {
        'add'      { $refused = [bool]($rest | Where-Object { $_ -in @('-A', '--all', '.', ':/', '*') }) }
        'stash'    { $refused = $true }
        'clean'    { $refused = $true }
        'push'     { $refused = $true }
        'switch'   { $refused = $true }
        'reset'    { $refused = [bool]($rest | Where-Object { $_ -eq '--hard' }) }
        'checkout' { $refused = [bool]($rest | Where-Object { $_ -eq '.' }) }
    }
    if ($refused) { throw "Refused: 'git $($Arguments -join ' ')' is one of the sweeping commands the Craft forbids." }
}

function Invoke-Git {
    <# Runs git in one worktree (git -C <Repo>) and returns its standard output; throws with git's message on failure. #>
    param([Parameter(Mandatory)] [string] $Repo, [Parameter(Mandatory)] [string[]] $Arguments, [switch] $AllowFailure)
    Assert-NotOwnerCheckout $Repo
    Assert-GitCommandAllowed $Arguments
    $settings = Get-WorkshopSettings
    $result = Invoke-Native -FilePath $settings.Git -Arguments (@('--no-optional-locks', '-C', $Repo) + $Arguments)
    if ($AllowFailure) { return $result }
    if ($result.ExitCode -ne 0) {
        $message = ($result.Error + $result.Output).Trim()
        throw "git $($Arguments -join ' ') failed in ${Repo}: $message"
    }
    return $result.Output.TrimEnd("`r", "`n")
}

function Get-GitBlobSha256 {
    <# The SHA-256 of a file's content as git stores it at a commit (line endings as committed), or '' if absent. #>
    param([Parameter(Mandatory)] [string] $Repo, [Parameter(Mandatory)] [string] $Commit, [Parameter(Mandatory)] [string] $Path)
    $settings = Get-WorkshopSettings
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $settings.Git
    $info.Arguments = ConvertTo-ArgumentString @('--no-optional-locks', '-C', $Repo, 'cat-file', 'blob', "${Commit}:$Path")
    $info.UseShellExecute = $false; $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true; $info.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::Start($info)
    $errors = $process.StandardError.ReadToEndAsync()
    $buffer = New-Object System.IO.MemoryStream
    $process.StandardOutput.BaseStream.CopyTo($buffer)
    $process.WaitForExit()
    [void]$errors.Result
    if ($process.ExitCode -ne 0) { return '' }
    $sha = [System.Security.Cryptography.SHA256]::Create()
    return (-join ($sha.ComputeHash($buffer.ToArray()) | ForEach-Object { $_.ToString('x2') }))
}

function Get-BuildErrors([string] $LogPath) {
    <# The error lines of an UnrealBuildTool log: compiler and linker errors and UnrealBuildTool's own. #>
    if (-not (Test-Path -LiteralPath $LogPath)) { return @() }
    return @(Get-Content -LiteralPath $LogPath | Where-Object { $_ -match '(: (fatal )?error [A-Z]*\d*:)|(error LNK\d+)|(^ERROR:)|(: error:)' })
}

function Get-GitLines([string] $Text) {
    if ([string]::IsNullOrEmpty($Text)) { return @() }
    return @($Text -split "`r?`n" | Where-Object { $_ -ne '' })
}

function Enter-NamedMutex {
    <# Takes a machine-wide named mutex, waiting up to the timeout. Returns the held mutex, or $null on timeout.
       A mutex whose holder died is released by Windows and counts as taken. #>
    param([Parameter(Mandatory)] [string] $Name, [int] $TimeoutMilliseconds = 60000)
    $mutex = New-Object System.Threading.Mutex($false, $Name)
    $held = $false
    try { $held = $mutex.WaitOne($TimeoutMilliseconds) }
    catch [System.Threading.AbandonedMutexException] { $held = $true }
    if (-not $held) { $mutex.Dispose(); return $null }
    return $mutex
}

function Exit-NamedMutex($Mutex) {
    if ($null -eq $Mutex) { return }
    try { $Mutex.ReleaseMutex() } catch [System.ApplicationException] { }
    $Mutex.Dispose()
}

function Test-MutexFree([string] $Name) {
    <# True when nobody holds the named mutex (it is taken and released at once to find out). #>
    $mutex = Enter-NamedMutex -Name $Name -TimeoutMilliseconds 0
    if ($null -eq $mutex) { return $false }
    Exit-NamedMutex $mutex
    return $true
}

function Write-TextFile([string] $Path, [string] $Text) {
    <# Writes UTF-8 text without a byte-order mark, creating the folder if needed. #>
    $folder = Split-Path -Parent $Path
    if ($folder -and -not (Test-Path -LiteralPath $folder)) { New-Item -ItemType Directory -Force -Path $folder | Out-Null }
    [System.IO.File]::WriteAllText($Path, $Text, $script:Utf8)
}

function Write-JsonFile([string] $Path, $Value) {
    <# Writes a value as JSON, replacing the file in one step so readers never see half a file. #>
    $part = "$Path.part"
    Write-TextFile $part (ConvertTo-Json -InputObject $Value -Depth 12)
    if (Test-Path -LiteralPath $Path) { [System.IO.File]::Replace($part, $Path, [NullString]::Value) }
    else { [System.IO.File]::Move($part, $Path) }
}

function Read-JsonFile([string] $Path) {
    <# Reads a JSON file, retrying briefly in case a writer is replacing it at that moment. #>
    for ($attempt = 0; $attempt -lt 5; $attempt++) {
        try { return (Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json) }
        catch { Start-Sleep -Milliseconds 200 }
    }
    throw "Could not read $Path as JSON."
}

function Get-UtcStamp { return [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ') }

function Get-LocalStamp { return (Get-Date).ToString('yyyy-MM-dd HH:mm') }

# ---- Order files: orders/<id>.md in the Register ----------------------------------------------------------------
# An order file is a block of "key: value" lines (a list value is "key:" followed by "  - item" lines), then
# "## History" with one "- <time> - <state> - <note>" line per change, then any free text under "## Notes".

$script:OrderFields = @(
    'id', 'title', 'owner', 'branch', 'slot', 'status', 'role', 'model', 'verify profile', 'contract version',
    'allowed paths', 'acceptance tests (locked)', 'read first', 'build', 'done when', 'returns', 'tokens', 'merged'
)
$script:OrderLists = @('allowed paths', 'acceptance tests (locked)', 'read first')
$script:OrderStates = @(
    'drafted', 'issued', 'building', 'built', 'verified', 'in inspection', 'passed', 'merged',
    'blocked', 'partial', 'stale', 'returned'
)

function Get-OrderStates { return $script:OrderStates }

function ConvertFrom-OrderText([string] $Text) {
    <# Reads an order file's text into an ordered table of fields, a history list and the notes. #>
    $fields = [ordered]@{}
    $history = New-Object System.Collections.Generic.List[string]
    $notes = New-Object System.Collections.Generic.List[string]
    $section = 'fields'
    $listKey = $null
    foreach ($line in ($Text -split "`r?`n")) {
        if ($line -match '^## History') { $section = 'history'; continue }
        if ($line -match '^## Notes') { $section = 'notes'; continue }
        if ($section -eq 'history') { if ($line -match '^- (.+)$') { $history.Add($Matches[1]) }; continue }
        if ($section -eq 'notes') { $notes.Add($line); continue }
        if ($listKey -and $line -match '^\s+- (.*)$') { $fields[$listKey] += , $Matches[1].Trim(); continue }
        if ($line -match '^([a-z][a-z ()]*[a-z)]):\s*(.*)$') {
            $key = $Matches[1]; $value = $Matches[2].Trim()
            if ($key -in $script:OrderLists) { $fields[$key] = @(); $listKey = $key; if ($value) { $fields[$key] += , $value } }
            else { $fields[$key] = $value; $listKey = $null }
        }
    }
    [pscustomobject]@{ Fields = $fields; History = $history; Notes = (($notes -join "`n").Trim()) }
}

function ConvertTo-OrderText($Order) {
    <# Writes an order back to text in the canonical field order, keeping any field the tools don't know. #>
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# $($Order.Fields['id']) - $($Order.Fields['title'])")
    $lines.Add('')
    $keys = @($script:OrderFields) + @($Order.Fields.Keys | Where-Object { $_ -notin $script:OrderFields })
    foreach ($key in $keys) {
        if (-not $Order.Fields.Contains($key)) { continue }
        $value = $Order.Fields[$key]
        if ($key -in $script:OrderLists) {
            $lines.Add("${key}:")
            foreach ($item in @($value)) { if ($item) { $lines.Add("  - $item") } }
        } else { $lines.Add("${key}: $value") }
    }
    $lines.Add('')
    $lines.Add('## History')
    foreach ($entry in $Order.History) { $lines.Add("- $entry") }
    if ($Order.Notes) { $lines.Add(''); $lines.Add('## Notes'); $lines.Add($Order.Notes) }
    return (($lines -join "`n") + "`n")
}

function Read-OrderFile([string] $Register, [string] $Id) {
    <# Reads orders/<id>.md from the Register, or returns $null when the order isn't there. #>
    $path = Join-Path $Register "orders\$Id.md"
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    return ConvertFrom-OrderText (Get-Content -LiteralPath $path -Raw -Encoding UTF8)
}

# ---- Writing the Register: one entry per file, each write serialised and committed at once -----------------------

function Test-RegisterPresent([string] $Register) {
    return (Test-Path -LiteralPath (Join-Path $Register '.git'))
}

function Get-RegisterHandEdits([string] $Register) {
    <# Files in the Register that changed without a commit. The tools commit every write at once, so any such file
       was edited by hand (or a write was interrupted), and the tools refuse to write over it. #>
    $status = Invoke-Git -Repo $Register -Arguments @('status', '--porcelain', '--untracked-files=all')
    return @(Get-GitLines $status)
}

function Invoke-RegisterWrite {
    <# Writes Register entries under the machine-wide Register mutex and commits them as one commit.
       $Compose runs inside the lock (so ids like <order>-g<k> never collide) and returns an ordered table of
       Register-relative path -> text, or -> @{ From = <file to place there> } for images and other binary files. #>
    param([Parameter(Mandatory)] [string] $Register, [Parameter(Mandatory)] [string] $Message,
          [Parameter(Mandatory)] [scriptblock] $Compose)
    if (-not (Test-RegisterPresent $Register)) { throw "There is no Register at $Register yet. Create it with: register.ps1 init" }
    $mutex = Enter-NamedMutex -Name 'Global\MuseeRegister' -TimeoutMilliseconds 120000
    if ($null -eq $mutex) { throw 'Another tool held the Register for two minutes; nothing was written. Try again.' }
    try {
        $edits = Get-RegisterHandEdits $Register
        if ($edits.Count) {
            $list = ($edits | Select-Object -First 10) -join "`n  "
            throw "Refused: the Register has changes no tool committed, so someone edited it by hand:`n  $list`nCommit them deliberately (git -C $Register commit) or restore them, then try again."
        }
        $entries = & $Compose $Register
        $paths = @()
        foreach ($relative in $entries.Keys) {
            $target = Join-Path $Register $relative
            $content = $entries[$relative]
            if ($content -is [hashtable]) {
                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
                [System.IO.File]::Copy($content.From, $target, $true)
            } else { Write-TextFile $target $content }
            $paths += ($relative -replace '\\', '/')
        }
        Invoke-Git -Repo $Register -Arguments (@('add', '--') + $paths) | Out-Null
        Invoke-Git -Repo $Register -Arguments (@('commit', '--quiet', '-m', $Message, '--') + $paths) | Out-Null
        return $paths
    } finally { Exit-NamedMutex $mutex }
}

function Get-ContractVersion([string] $Repo, [string] $Ref) {
    <# The contract version at a commit: a short hash over every file under the project's Source/**/Public folders
       (BUILD_PLAN 4.2). Two commits with the same public headers have the same contract version. #>
    $tree = Invoke-Git -Repo $Repo -Arguments @('ls-tree', '-r', $Ref, '--', 'MuseeVision/Source')
    $public = @(Get-GitLines $tree | Where-Object { $_ -match '/Public/' })
    $bytes = $script:Utf8.GetBytes(($public -join "`n"))
    $sha = [System.Security.Cryptography.SHA1]::Create()
    $hash = -join ($sha.ComputeHash($bytes) | ForEach-Object { $_.ToString('x2') })
    return $hash.Substring(0, 12)
}

Export-ModuleMember -Function *
