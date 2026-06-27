#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

# Set up the dedicated data volume (disk1, drive D:) that keeps build junk off
# the OS disk — the historic cause of the runners filling the datastore. The data
# volume holds two areas:
#   D:\work   — the runner work dir (checkout + build tree); wiped every job by
#               the ephemeral supervisor (see 05-github-runner.ps1).
#   D:\cache  — persistent vcpkg binary cache + ccache; survives the wipe,
#               size-capped by the supervisor so it can't grow without bound.
# The runner profile's .cache is junctioned here so vcpkg ($HOME/.cache/vcpkg)
# lands on D: automatically; ccache is pointed at D: via the CCACHE_DIR machine
# environment variable (see 04-choco-packages.ps1).

Write-Host "==> Setting up the data volume (runner work + persistent caches)"

$DataLabel  = "RunnerData"
$DataLetter = "D"

# During the Packer build the install/tools ISOs occupy CD-ROM drive letters and
# one of them is usually D:. Vacate D: (and any other optical letter that would
# clash) by pushing optical drives to the top of the alphabet, so the data
# partition can take a stable D:. (At runtime on a deployed clone there are no
# ISOs, so D: is naturally free and persists via the disk signature.)
$park = 'Z'
foreach ($vol in Get-CimInstance Win32_Volume -Filter "DriveType = 5" -ErrorAction SilentlyContinue) {
    if ($vol.DriveLetter) {
        try {
            Set-CimInstance -InputObject $vol -Property @{ DriveLetter = "${park}:" } -ErrorAction Stop
            Write-Host "==> Moved optical drive $($vol.DriveLetter) -> ${park}:"
            $park = [char]([int][char]$park - 1)
        } catch {
            Write-Host "WARNING: could not move optical drive $($vol.DriveLetter): $($_.Exception.Message)"
        }
    }
}

# Find the data disk: the raw, uninitialized disk (disk0 is already partitioned
# for Windows; disk1 is RAW at build time).
$dataDisk = Get-Disk | Where-Object { $_.PartitionStyle -eq 'RAW' } | Sort-Object Number | Select-Object -First 1

if ($dataDisk) {
    Write-Host "==> Initializing data disk #$($dataDisk.Number)"
    if ($dataDisk.IsOffline)  { Set-Disk -Number $dataDisk.Number -IsOffline $false }
    if ($dataDisk.IsReadOnly) { Set-Disk -Number $dataDisk.Number -IsReadOnly $false }
    Initialize-Disk -Number $dataDisk.Number -PartitionStyle GPT
    $part = New-Partition -DiskNumber $dataDisk.Number -UseMaximumSize
    Format-Volume -Partition $part -FileSystem NTFS -NewFileSystemLabel $DataLabel -Confirm:$false | Out-Null
    Set-Partition -DiskNumber $dataDisk.Number -PartitionNumber $part.PartitionNumber -NewDriveLetter $DataLetter
} else {
    # Idempotent re-run: the labelled volume already exists — make sure it's D:.
    $existing = Get-Volume -FileSystemLabel $DataLabel -ErrorAction SilentlyContinue
    if (-not $existing) { throw "Could not find a raw data disk to initialize" }
    if ($existing.DriveLetter -ne $DataLetter) {
        $p = Get-Partition | Where-Object { $_.AccessPaths -contains "$($existing.DriveLetter):\" } | Select-Object -First 1
        if ($p) { Set-Partition -InputObject $p -NewDriveLetter $DataLetter }
    }
    Write-Host "==> Data volume already present; ensured drive ${DataLetter}:"
}

# Lay out work/ (wiped each job) and cache/ (persistent).
New-Item -ItemType Directory -Path "${DataLetter}:\work"  -Force | Out-Null
New-Item -ItemType Directory -Path "${DataLetter}:\cache" -Force | Out-Null

# Redirect the runner profile's .cache onto the persistent cache area via a
# directory junction so vcpkg ($HOME/.cache/vcpkg, used by the self-hosted
# _build.yml step) lands on D: with no workflow change.
$runnerCache = "C:\Users\runner\.cache"
if (Test-Path $runnerCache) {
    Copy-Item -Path "$runnerCache\*" -Destination "${DataLetter}:\cache" -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path $runnerCache -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Junction -Path $runnerCache -Target "${DataLetter}:\cache" | Out-Null

Write-Host "==> Data volume ready: ${DataLetter}:\work (ephemeral) + ${DataLetter}:\cache (persistent); C:\Users\runner\.cache -> ${DataLetter}:\cache"
