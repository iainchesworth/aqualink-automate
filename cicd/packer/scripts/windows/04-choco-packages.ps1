#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "==> Installing additional packages via Chocolatey"

choco install nsis -y --no-progress
choco install 7zip -y --no-progress
choco install ccache -y --no-progress

# Refresh PATH
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# Configure ccache
Write-Host "==> Configuring ccache"
$ccacheDir = "$env:USERPROFILE\.config\ccache"
New-Item -ItemType Directory -Path $ccacheDir -Force | Out-Null
@"
max_size = 5G
compression = true
compression_level = 6
"@ | Set-Content -Path "$ccacheDir\ccache.conf" -Encoding UTF8

Write-Host "==> Additional packages installed"
