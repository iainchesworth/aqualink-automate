#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "==> Cleaning up for template compression"

# Clean Windows component store
Write-Host "==> Running DISM cleanup"
& dism.exe /Online /Cleanup-Image /StartComponentCleanup /ResetBase 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "DISM cleanup returned $LASTEXITCODE (non-fatal, continuing)"
}

# Remove temp files
Write-Host "==> Removing temporary files"
Remove-Item -Path "$env:TEMP\*" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item -Path "C:\Windows\Temp\*" -Recurse -Force -ErrorAction SilentlyContinue

# Clear Windows Update cache
Stop-Service -Name wuauserv -Force -ErrorAction SilentlyContinue
Remove-Item -Path "C:\Windows\SoftwareDistribution\Download\*" -Recurse -Force -ErrorAction SilentlyContinue
Start-Service -Name wuauserv -ErrorAction SilentlyContinue

# Optimize volume (defrag + trim for thin-provisioned VMDK)
Write-Host "==> Optimizing volume"
Optimize-Volume -DriveLetter C -ReTrim -ErrorAction SilentlyContinue

Write-Host "==> Cleanup complete"
