param(
    [string]$SourceRoot = ""
)

$ErrorActionPreference = "Stop"

function Get-FullPath {
    param([string]$Path)
    return [System.IO.Path]::GetFullPath($Path)
}

function Get-ZipRelativePath {
    param(
        [string]$Base,
        [string]$Path
    )

    $baseFull = (Get-FullPath $Base).TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    $pathFull = Get-FullPath $Path

    if (-not $pathFull.StartsWith($baseFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside zip base: $pathFull"
    }

    return $pathFull.Substring($baseFull.Length).Replace("\", "/")
}

function Test-SorrDataRoot {
    param([string]$Path)

    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Container)) {
        return $false
    }

    $sorrDat = Join-Path $Path "SorR.dat"
    $systemTxt = Join-Path $Path "mod\system.txt"
    return (Test-Path -LiteralPath $sorrDat -PathType Leaf) -and
           (Test-Path -LiteralPath $systemTxt -PathType Leaf)
}

function Get-PreferenceScore {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $full = Get-FullPath $Path
    $prepared = Get-FullPath (Join-Path $RepoRoot "sorr-vita-master\data")
    $x64 = Get-FullPath (Join-Path $RepoRoot "out\x64-launch-data")
    $timingX64 = Get-FullPath (Join-Path $RepoRoot "out\timing-x64-data")
    $timing32 = Get-FullPath (Join-Path $RepoRoot "out\timing-32-data")
    $downloads = Get-FullPath (Join-Path $HOME "Downloads\SORRv52_rev550\SORRv52")

    if ($full -ieq $prepared) { return 0 }
    if ($full -ieq $x64) { return 10 }
    if ($full -ieq $timingX64) { return 20 }
    if ($full -ieq $timing32) { return 30 }
    if ($full -ieq $downloads) { return 50 }
    if ($full -like (Get-FullPath (Join-Path $RepoRoot "*"))) { return 80 }
    return 100
}

function Find-SorrDataRoots {
    param([string]$RepoRoot)

    $searchRoots = @(
        (Join-Path $RepoRoot "sorr-vita-master\data"),
        (Join-Path $RepoRoot "out\x64-launch-data"),
        (Join-Path $RepoRoot "out\timing-x64-data"),
        (Join-Path $RepoRoot "out\timing-32-data"),
        (Join-Path $HOME "Downloads\SORRv52_rev550\SORRv52"),
        $RepoRoot
    )

    $seen = @{}
    $results = @()

    foreach ($root in $searchRoots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        if (Test-SorrDataRoot $root) {
            $full = Get-FullPath $root
            if (-not $seen.ContainsKey($full)) {
                $seen[$full] = $true
                $results += $full
            }
        }

        Get-ChildItem -LiteralPath $root -Recurse -Filter SorR.dat -File -ErrorAction SilentlyContinue |
            ForEach-Object {
                $candidate = Get-FullPath $_.DirectoryName
                if (-not $seen.ContainsKey($candidate) -and (Test-SorrDataRoot $candidate)) {
                    $seen[$candidate] = $true
                    $results += $candidate
                }
            }
    }

    return $results
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Get-FullPath (Join-Path $scriptRoot "..")
$outputRoot = Get-FullPath (Join-Path $repoRoot "out\local-only")
$stagingRoot = Join-Path $outputRoot "SORR_IMPORT"
$zipPath = Join-Path $outputRoot "SORR_IMPORT.zip"

if ($SourceRoot) {
    $source = Get-FullPath $SourceRoot
    if (-not (Test-SorrDataRoot $source)) {
        throw "SourceRoot does not contain both SorR.dat and mod/system.txt: $source"
    }
    $candidates = @($source)
} else {
    $candidates = Find-SorrDataRoots $repoRoot |
        Sort-Object @{ Expression = { Get-PreferenceScore $repoRoot $_ } }, @{ Expression = { $_ } }

    if (-not $candidates -or $candidates.Count -eq 0) {
        throw "No SoRR data root found. Expected a folder containing SorR.dat and mod/system.txt."
    }

    $source = $candidates[0]
}

Write-Host "Candidate SoRR data roots:"
foreach ($candidate in $candidates) {
    $score = Get-PreferenceScore $repoRoot $candidate
    $mark = if ((Get-FullPath $candidate) -ieq (Get-FullPath $source)) { "*" } else { " " }
    Write-Host ("{0} score={1} {2}" -f $mark, $score, $candidate)
}
Write-Host "Using source root: $source"

if (-not (Test-Path -LiteralPath $outputRoot)) {
    New-Item -ItemType Directory -Path $outputRoot | Out-Null
}

if (Test-Path -LiteralPath $stagingRoot) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingRoot | Out-Null

Get-ChildItem -LiteralPath $source -Force | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination $stagingRoot -Recurse -Force
}

$requiredStaged = @(
    (Join-Path $stagingRoot "SorR.dat"),
    (Join-Path $stagingRoot "mod\system.txt")
)

foreach ($required in $requiredStaged) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required staged file missing: $required"
    }
}

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$tempZip = Join-Path ([System.IO.Path]::GetTempPath()) ("SORR_IMPORT-{0}.zip" -f $PID)
if (Test-Path -LiteralPath $tempZip) {
    Remove-Item -LiteralPath $tempZip -Force
}

$zipStream = [System.IO.File]::Open($tempZip, [System.IO.FileMode]::CreateNew)
$archive = New-Object System.IO.Compression.ZipArchive($zipStream, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    $directories = @((Get-Item -LiteralPath $stagingRoot)) +
        @(Get-ChildItem -LiteralPath $stagingRoot -Recurse -Directory -Force)
    foreach ($directory in $directories) {
        $entryName = (Get-ZipRelativePath $outputRoot $directory.FullName).TrimEnd("/") + "/"
        $archive.CreateEntry($entryName) | Out-Null
    }

    Get-ChildItem -LiteralPath $stagingRoot -Recurse -File -Force | ForEach-Object {
        $entryName = Get-ZipRelativePath $outputRoot $_.FullName
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive,
            $_.FullName,
            $entryName,
            [System.IO.Compression.CompressionLevel]::Optimal
        ) | Out-Null
    }
} finally {
    $archive.Dispose()
    $zipStream.Dispose()
}

Move-Item -LiteralPath $tempZip -Destination $zipPath -Force

$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entries = $zip.Entries | ForEach-Object { $_.FullName }
    $requiredEntries = @(
        "SORR_IMPORT/SorR.dat",
        "SORR_IMPORT/mod/system.txt"
    )

    foreach ($entry in $requiredEntries) {
        if ($entries -notcontains $entry) {
            throw "Zip missing required entry: $entry"
        }
    }

    Write-Host "Verified required zip entries:"
    foreach ($entry in $requiredEntries) {
        Write-Host "  $entry"
    }
    Write-Host ("Zip entry count: {0}" -f $entries.Count)
} finally {
    $zip.Dispose()
}

$hash = Get-FileHash -LiteralPath $zipPath -Algorithm SHA256
Write-Host "Created local-only D2 import package:"
Write-Host "  $zipPath"
Write-Host "SHA256:"
Write-Host "  $($hash.Hash)"
Write-Host "Do not commit or upload this zip."
