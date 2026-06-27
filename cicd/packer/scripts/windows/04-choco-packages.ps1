#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "==> Installing additional packages via Chocolatey"

choco install nsis -y --no-progress
choco install 7zip -y --no-progress
choco install ccache -y --no-progress

# Refresh PATH
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# Configure ccache. Pin the cache onto the persistent data volume (D:\cache\ccache,
# created by 00-data-volume.ps1) and cap it at 4 GB so it survives the per-job
# workspace wipe, lives off the OS disk, and can't crowd out the work area on the
# shared ~20 GB data disk. CCACHE_DIR/CCACHE_MAXSIZE are set as *machine*
# environment variables — the most reliable way to steer ccache on Windows
# regardless of which user the runner job runs as (env wins over the config file).
Write-Host "==> Configuring ccache (cache on D:\cache\ccache, 4 GB cap)"
New-Item -ItemType Directory -Path "D:\cache\ccache" -Force | Out-Null
[Environment]::SetEnvironmentVariable("CCACHE_DIR", "D:\cache\ccache", "Machine")
[Environment]::SetEnvironmentVariable("CCACHE_MAXSIZE", "4G", "Machine")

$ccacheDir = "$env:USERPROFILE\.config\ccache"
New-Item -ItemType Directory -Path $ccacheDir -Force | Out-Null
@"
cache_dir = D:\cache\ccache
max_size = 4G
compression = true
compression_level = 6
"@ | Set-Content -Path "$ccacheDir\ccache.conf" -Encoding UTF8

Write-Host "==> Additional packages installed"
