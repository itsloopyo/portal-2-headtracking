#!/usr/bin/env pwsh
#Requires -Version 5.1
<#
.SYNOPSIS
    Release workflow for Portal 2 Head Tracking.

.DESCRIPTION
    1. Validate semver + git state.
    2. Regenerate CHANGELOG.md from conventional commits (via
       cameraunlock-core/powershell/ReleaseWorkflow.psm1).
    3. Bump the version in src/version.h and scripts/install.cmd.
    4. Build the x86 release.
    5. Commit the version + changelog as "Release v<version>".
    6. Create annotated tag v<version> and push it; CI picks up the tag and
       publishes the GitHub release artifacts.

.EXAMPLE
    pixi run release 1.0.0
    pixi run release patch
    pixi run release nightly
#>
param(
    [Parameter(Position = 0)]
    [string]$Version = '',
    # Ship a release even when there are no user-facing commits since the last
    # tag (writes a maintenance changelog entry instead of aborting).
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$scriptDir     = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir    = Split-Path -Parent $scriptDir
$versionPath   = Join-Path $projectDir 'src\version.h'
$installCmdPath = Join-Path $projectDir 'scripts\install.cmd'
$changelogPath = Join-Path $projectDir 'CHANGELOG.md'

Import-Module (Join-Path $projectDir 'cameraunlock-core\powershell\ReleaseWorkflow.psm1') -Force

function Get-ModVersion {
    $content = Get-Content $versionPath -Raw
    if ($content -match 'HEADTRACKING_VERSION_STRING\s+"([^"]+)"') { return $Matches[1] }
    throw "Could not read HEADTRACKING_VERSION_STRING from $versionPath"
}

function Set-ModVersion {
    param([string]$NewVersion)
    $parts = $NewVersion.Split('.')
    # ReadAllText/WriteAllText so the file's existing line endings survive.
    $raw = [System.IO.File]::ReadAllText($versionPath)
    $raw = [regex]::Replace($raw, '(?m)^(#define HEADTRACKING_VERSION_MAJOR\s+)\d+', "`${1}$($parts[0])")
    $raw = [regex]::Replace($raw, '(?m)^(#define HEADTRACKING_VERSION_MINOR\s+)\d+', "`${1}$($parts[1])")
    $raw = [regex]::Replace($raw, '(?m)^(#define HEADTRACKING_VERSION_PATCH\s+)\d+', "`${1}$($parts[2])")
    $raw = [regex]::Replace($raw, 'HEADTRACKING_VERSION_STRING\s+"[^"]+"', "HEADTRACKING_VERSION_STRING `"$NewVersion`"")
    [System.IO.File]::WriteAllText($versionPath, $raw)
}

# Mirrors New-ChangelogFromCommits' insertion so a -Force maintenance entry
# lands in the same place with the same shape.
function Add-MaintenanceChangelogEntry {
    param([string]$Path, [string]$NewVersion)
    $date = Get-Date -Format 'yyyy-MM-dd'
    $entry = "## [$NewVersion] - $date`n`n### Changed`n`n- Maintenance release (no user-facing changes).`n`n"
    $changelog = Get-Content $Path -Raw
    if ($changelog -match '(?s)(# Changelog.*?)(## \[)') {
        $changelog = $changelog -replace '(?s)(# Changelog.*?\n\n)', "`$1$entry"
    } else {
        $changelog = $changelog -replace '(?s)(# Changelog.*?\n)', "`$1$entry"
    }
    $changelog = $changelog.TrimEnd() + "`n"
    Set-Content $Path $changelog -NoNewline
}

Write-Host ''
Write-Host '=== Portal 2 Head Tracking Release ===' -ForegroundColor Cyan
Write-Host ''

$current = Get-ModVersion

if ([string]::IsNullOrWhiteSpace($Version)) {
    Write-Host "Current version: $current" -ForegroundColor Yellow
    Write-Host 'Usage: pixi run release <major|minor|patch|nightly|X.Y.Z>'
    exit 0
}

if ($Version -eq 'nightly') {
    & (Join-Path $scriptDir 'release-nightly.ps1')
    exit $LASTEXITCODE
}

try {
    $Version = Resolve-ReleaseVersion -Argument $Version -CurrentVersion $current
} catch {
    Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

$tag = "v$Version"

$branch = git rev-parse --abbrev-ref HEAD
if ($branch -ne 'main') {
    Write-Host "Must be on main branch to release (currently on '$branch')" -ForegroundColor Red
    exit 1
}
if (-not (Test-CleanGitStatus)) {
    Write-Host 'Working tree has uncommitted changes - commit or stash first.' -ForegroundColor Red
    git status --short
    exit 1
}
if (Test-GitTagExists -Tag $tag) {
    Write-Host "Tag '$tag' already exists." -ForegroundColor Red
    exit 1
}

Write-Host "Current version: $current" -ForegroundColor Gray
Write-Host "New version:     $Version" -ForegroundColor Green
Write-Host ''

# Step 1 - changelog first, because it is the gate that can fail. Generating it
# before mutating any version file means an abort leaves the tree clean rather
# than stranding a half-applied bump with no tag.
Write-Host 'Generating CHANGELOG from commits...' -ForegroundColor Cyan
$hasTags = git tag -l 2>$null
if (-not $hasTags) {
    $date = Get-Date -Format 'yyyy-MM-dd'
    Set-Content $changelogPath "# Changelog`n`n## [$Version] - $date`n`nFirst release.`n"
} else {
    try {
        New-ChangelogFromCommits -ChangelogPath $changelogPath -Version $Version `
            -ArtifactPaths @('src/', 'cameraunlock-core', 'scripts/')
    } catch {
        if (-not $Force) {
            Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
            Write-Host 'No user-facing changes to release. Re-run with -Force for a maintenance release.' -ForegroundColor Yellow
            exit 1
        }
        Write-Host 'No user-facing commits since last tag - writing maintenance entry (-Force).' -ForegroundColor Yellow
        Add-MaintenanceChangelogEntry -Path $changelogPath -NewVersion $Version
    }
}

# Step 2 - bump src/version.h and install.cmd's MOD_VERSION. The latter is what
# the installer writes into the user's .headtracking-state.json, and the
# packager stamps launcher-manifest.json from version.h, so these two are the
# only places a version lives.
Write-Host "Updating src/version.h to $Version..." -ForegroundColor Cyan
Set-ModVersion -NewVersion $Version

Write-Host "Updating scripts/install.cmd MOD_VERSION to $Version..." -ForegroundColor Cyan
$installRaw = [System.IO.File]::ReadAllText($installCmdPath)
if ($installRaw -notmatch 'set "MOD_VERSION=[^"]+"') {
    throw "MOD_VERSION line not found in $installCmdPath"
}
$installRaw = [regex]::Replace($installRaw, 'set "MOD_VERSION=[^"]+"', "set `"MOD_VERSION=$Version`"")
[System.IO.File]::WriteAllText($installCmdPath, $installRaw)

# Step 3 - build
Write-Host 'Building release (x86)...' -ForegroundColor Cyan
Push-Location $projectDir
try {
    pixi run build-release
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
} finally {
    Pop-Location
}

# Step 4 - commit named files only, so build artifacts cannot sweep in
Write-Host 'Committing version + changelog...' -ForegroundColor Cyan
git add $versionPath $changelogPath $installCmdPath
git diff --cached --quiet
if ($LASTEXITCODE -eq 0) {
    Write-Host 'No version/changelog changes - tagging existing HEAD.' -ForegroundColor Yellow
} else {
    git commit -m "Release v$Version"
    if ($LASTEXITCODE -ne 0) { throw 'Commit failed' }
}

# Step 5 - tag + push
Write-Host "Creating tag $tag..." -ForegroundColor Cyan
git tag -a $tag -m "Release $tag"
git push origin main
git push origin $tag

Write-Host ''
Write-Host "Release $tag pushed - CI will build and publish artifacts." -ForegroundColor Green
