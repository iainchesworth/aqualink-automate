# Install VMware Tools silently from the mounted CD-ROM
$ErrorActionPreference = "SilentlyContinue"

# Find the VMware Tools setup on any CD-ROM drive
$setup = $null
foreach ($drive in (Get-PSDrive -PSProvider FileSystem)) {
    $candidate = Join-Path $drive.Root "setup64.exe"
    if (Test-Path $candidate) {
        $setup = $candidate
        break
    }
}

if (-not $setup) {
    Write-Host "VMware Tools setup64.exe not found on any drive"
    exit 1
}

Write-Host "Installing VMware Tools from $setup"
$process = Start-Process -FilePath $setup -ArgumentList '/S /v"/qn REBOOT=ReallySuppress"' -Wait -PassThru
Write-Host "VMware Tools installer exited with code $($process.ExitCode)"

# Wait for the network driver to initialize
Start-Sleep -Seconds 15
