param(
    [Parameter(Mandatory = $true)]
    [string]$TaskName
)

$ErrorActionPreference = "Stop"

function Require-Path {
    param([string]$Path, [string]$Label)
    if (-not (Test-Path $Path)) {
        throw "Missing $Label at: $Path"
    }
}

function Normalize-TaskName {
    param([string]$Name)
    $base = [System.IO.Path]::GetFileNameWithoutExtension($Name)
    if ([string]::IsNullOrWhiteSpace($base)) {
        throw "Invalid task name: '$Name'"
    }
    return $base
}

function Sanitize-BranchName {
    param([string]$Name)
    $branch = $Name.ToLowerInvariant()
    $branch = $branch -replace '[^a-z0-9._/-]', '-'
    $branch = $branch -replace '-{2,}', '-'
    $branch = $branch.Trim('-')
    if ([string]::IsNullOrWhiteSpace($branch)) {
        throw "Could not derive a valid branch name from task name '$Name'"
    }
    return $branch
}

function Extract-Section {
    param(
        [string]$Text,
        [string]$StartMarker,
        [string[]]$EndMarkers
    )

    $start = $Text.IndexOf($StartMarker)
    if ($start -lt 0) {
        return $null
    }

    $contentStart = $start + $StartMarker.Length
    $endPositions = @()

    foreach ($marker in $EndMarkers) {
        $pos = $Text.IndexOf($marker, $contentStart)
        if ($pos -ge 0) {
            $endPositions += $pos
        }
    }

    $contentEnd = if ($endPositions.Count -gt 0) {
        ($endPositions | Measure-Object -Minimum).Minimum
    } else {
        $Text.Length
    }

    return $Text.Substring($contentStart, $contentEnd - $contentStart).Trim()
}

function Resolve-CodexCommand {
    $cmdShim = Get-Command "codex.cmd" -ErrorAction SilentlyContinue
    if ($cmdShim -ne $null) {
        return $cmdShim.Path
    }

    $codex = Get-Command "codex" -ErrorAction SilentlyContinue
    if ($codex -ne $null) {
        return $codex.Path
    }

    throw "Could not find Codex CLI. Make sure the codex command is available on PATH."
}

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$workflowRoot = Join-Path $root "AgentWorkflows"
$contextPath = Join-Path $workflowRoot "AgentContext.context"
$todoDir = Join-Path $workflowRoot "todo"
$archiveDir = Join-Path $workflowRoot "archive"
$changelogDir = Join-Path $workflowRoot "changelogs"

Require-Path $workflowRoot "workflow root"
Require-Path $contextPath "context file"
Require-Path $todoDir "todo folder"

if (-not (Test-Path $archiveDir)) {
    New-Item -ItemType Directory -Path $archiveDir | Out-Null
}

if (-not (Test-Path $changelogDir)) {
    New-Item -ItemType Directory -Path $changelogDir | Out-Null
}

$baseTaskName = Normalize-TaskName $TaskName
$todoFileName = if ($TaskName.EndsWith(".todo")) { $TaskName } else { "$baseTaskName.todo" }
$todoPath = Join-Path $todoDir $todoFileName
$logPath = Join-Path $changelogDir "$baseTaskName.complete.log"

Require-Path $todoPath "todo file"

$context = Get-Content $contextPath -Raw
$task = Get-Content $todoPath -Raw

$prompt = @"
You are operating inside a project repository.

Read the project context first, then complete exactly one task.

PROJECT CONTEXT
$context

TASK FILE NAME
$todoFileName

TASK CONTENTS
$task

RULES
- Complete only this task.
- Make changes directly in the repository.
- Keep changes focused and minimal.
- Do not perform future tasks.
- Do not rename or move the task file yourself.
- If you cannot complete the task, say so explicitly.
- Prefer juce over other libraries if applicable.
- If other libraries are required, prefer well known ones with permissive licenses. 
- Prefer library imports automatically via package managers in cmake/vsproject files rather than manual code additions.
- Use the existing code style and patterns in the repository.

When you are done, output exactly this structured format:

=== SUMMARY ===
One short commit-style summary line, ideally under 72 characters.

=== CHANGES ===
Bullet list of concrete code and file changes.

=== DOCS ===
Short high-level changelog/documentation note for future readers.

=== CAVEATS ===
Any important follow-up notes, limitations, or risks.

=== TESTING ===
What you ran or checked. If nothing was run, say "Not run". Tests don't exist in this repo yet so this won't be common.

Do not include any extra sections before or after this format.
"@

Write-Host "Running Codex for task: $todoFileName"

$codexCommand = Resolve-CodexCommand
$previousErrorActionPreference = $ErrorActionPreference

try {
    $ErrorActionPreference = "Continue"
    $codexOutput = $prompt |
        & $codexCommand exec --color never - 2>&1 |
        ForEach-Object { $_.ToString() } |
        Out-String
    $codexExitCode = $LASTEXITCODE
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
}


if ($codexExitCode -ne 0) {
    Write-Host $codexOutput
    throw "Codex failed for task '$todoFileName' with exit code $codexExitCode"
}

$summary = Extract-Section $codexOutput "=== SUMMARY ===" @("=== CHANGES ===", "=== DOCS ===", "=== CAVEATS ===", "=== TESTING ===")
$changes = Extract-Section $codexOutput "=== CHANGES ===" @("=== DOCS ===", "=== CAVEATS ===", "=== TESTING ===")
$docs = Extract-Section $codexOutput "=== DOCS ===" @("=== CAVEATS ===", "=== TESTING ===")
$caveats = Extract-Section $codexOutput "=== CAVEATS ===" @("=== TESTING ===")
$testing = Extract-Section $codexOutput "=== TESTING ===" @()

if ([string]::IsNullOrWhiteSpace($summary) -or
    [string]::IsNullOrWhiteSpace($changes) -or
    [string]::IsNullOrWhiteSpace($docs) -or
    [string]::IsNullOrWhiteSpace($caveats) -or
    [string]::IsNullOrWhiteSpace($testing)) {
    Write-Host $codexOutput
    throw "Codex output did not match the required structured format."
}

$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$branchName = Sanitize-BranchName $baseTaskName

$structuredLog = @"
Task: $todoFileName
Completed: $timestamp
Branch: $branchName

=== SUMMARY ===
$summary

=== CHANGES ===
$changes

=== DOCS ===
$docs

=== CAVEATS ===
$caveats

=== TESTING ===
$testing

=== RAW CODEX OUTPUT ===
$codexOutput
"@

Set-Content -Path $logPath -Value $structuredLog -Encoding UTF8

$commitMessagePath = Join-Path $env:TEMP "$baseTaskName.commitmsg.txt"
$commitMessage = @"
$summary

Task: $todoFileName

Changes:
$changes

Docs:
$docs

Caveats:
$caveats

Testing:
$testing
"@
Set-Content -Path $commitMessagePath -Value $commitMessage -Encoding UTF8

$originalBranch = (& git branch --show-current 2>&1 | Out-String).Trim()
if ([string]::IsNullOrWhiteSpace($originalBranch)) {
    throw "Could not determine current git branch."
}

try {
    & git switch -c $branchName
    if ($LASTEXITCODE -ne 0) { throw "git switch -c failed" }

    & git add .
    if ($LASTEXITCODE -ne 0) { throw "git add failed" }

    & git commit -F $commitMessagePath
    if ($LASTEXITCODE -ne 0) { throw "git commit failed" }

    & git push -u origin HEAD
    if ($LASTEXITCODE -ne 0) { throw "git push failed" }

    & git switch "main"
    if ($LASTEXITCODE -ne 0) { throw "git switch main failed" }
}
catch {
    try {
        if ($originalBranch -ne "main") {
            & git switch $originalBranch | Out-Null
        }
    } catch { }
    throw
}
finally {
    if (Test-Path $commitMessagePath) {
        Remove-Item $commitMessagePath -Force -ErrorAction SilentlyContinue
    }
}

$archivePath = Join-Path $archiveDir $todoFileName
if (Test-Path $archivePath) {
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $archivePath = Join-Path $archiveDir "$baseTaskName.$stamp.todo"
}

Move-Item -Path $todoPath -Destination $archivePath

Write-Host "Task completed."
Write-Host "Changelog written: $logPath"
Write-Host "Todo archived: $archivePath"
Write-Host "Branch pushed: $branchName"
Write-Host "Current branch: main"
