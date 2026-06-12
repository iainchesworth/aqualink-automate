#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "==> Installing CMake and Ninja"

choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y --no-progress
choco install ninja -y --no-progress

# Refresh PATH
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

Write-Host "==> CMake $(cmake --version | Select-Object -First 1) installed"
Write-Host "==> Ninja $(ninja --version) installed"
