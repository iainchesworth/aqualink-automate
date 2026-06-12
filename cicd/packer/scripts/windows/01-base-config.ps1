#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "==> Configuring base system settings"

# Disable IE Enhanced Security Configuration
$AdminKey = "HKLM:\SOFTWARE\Microsoft\Active Setup\Installed Components\{A509B1A7-37EF-4b3f-8CFC-4F3A74704073}"
$UserKey = "HKLM:\SOFTWARE\Microsoft\Active Setup\Installed Components\{A509B1A8-37EF-4b3f-8CFC-4F3A74704073}"
if (Test-Path $AdminKey) { Set-ItemProperty -Path $AdminKey -Name "IsInstalled" -Value 0 }
if (Test-Path $UserKey) { Set-ItemProperty -Path $UserKey -Name "IsInstalled" -Value 0 }

# Enable TLS 1.2
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# Allow the local 'runner' admin account to be used for remote administration
# over WinRM. Without this, UAC network-token filtering hands local accounts a
# filtered (non-admin) token over the network, so remote registration/management
# (e.g. config.cmd over WinRM) fails with "Access is denied" even with correct
# credentials. Required for the runner to be remotely (re)registerable.
New-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System' `
    -Name 'LocalAccountTokenFilterPolicy' -Value 1 -PropertyType DWord -Force | Out-Null

# Install Chocolatey
Write-Host "==> Installing Chocolatey"
Set-ExecutionPolicy Bypass -Scope Process -Force
Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Refresh PATH
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# Install Git
Write-Host "==> Installing Git"
choco install git -y --no-progress
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")

# Ensure Git bash is on the system PATH (required by GitHub Actions runner)
$sysPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
$gitBin = "C:\Program Files\Git\bin"
if ($sysPath -notmatch [regex]::Escape($gitBin)) {
    [Environment]::SetEnvironmentVariable("Path", "$sysPath;$gitBin", "Machine")
    Write-Host "==> Added Git bash to system PATH"
}

Write-Host "==> Base configuration complete"
