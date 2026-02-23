#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

$RunnerVersion = $env:GITHUB_RUNNER_VERSION
if (-not $RunnerVersion) { $RunnerVersion = "2.321.0" }

$RunnerDir = "C:\actions-runner"

Write-Host "==> Installing GitHub Actions runner v${RunnerVersion}"

New-Item -ItemType Directory -Path $RunnerDir -Force | Out-Null

# Download runner
$runnerUrl = "https://github.com/actions/runner/releases/download/v${RunnerVersion}/actions-runner-win-x64-${RunnerVersion}.zip"
$runnerZip = "$env:TEMP\actions-runner.zip"
Invoke-WebRequest -Uri $runnerUrl -OutFile $runnerZip -UseBasicParsing
Expand-Archive -Path $runnerZip -DestinationPath $RunnerDir -Force
Remove-Item $runnerZip -Force

# Create registration helper script
$registerScript = @'
param(
    [Parameter(Mandatory=$true)]
    [string]$Token,

    [Parameter(Mandatory=$true)]
    [string]$RepoUrl,

    [string]$Name = $env:COMPUTERNAME,

    [string]$Labels = "self-hosted,windows,x64"
)

$ErrorActionPreference = "Stop"
$RunnerDir = "C:\actions-runner"

& "$RunnerDir\config.cmd" `
    --url $RepoUrl `
    --token $Token `
    --name $Name `
    --labels $Labels `
    --unattended `
    --replace `
    --runasservice
'@
$registerScript | Set-Content -Path "$RunnerDir\register-runner.ps1" -Encoding UTF8

# Create auto-registration script that reads from vSphere guestinfo
$autoRegisterScript = @'
$ErrorActionPreference = "SilentlyContinue"

$RunnerDir = "C:\actions-runner"
$Marker = "$RunnerDir\.registered"
$Log = "C:\runner-auto-register.log"

Start-Transcript -Path $Log -Append
Write-Host "$(Get-Date): Auto-register started"

# Skip if already registered
if (Test-Path $Marker) {
    Write-Host "Runner already registered, skipping"
    Stop-Transcript
    exit 0
}

# Read guestinfo variables (set on the VM in vSphere)
function Get-GuestInfo($key) {
    try {
        $vmtoolsd = "C:\Program Files\VMware\VMware Tools\vmtoolsd.exe"
        $result = & $vmtoolsd --cmd "info-get guestinfo.$key" 2>$null
        if ($LASTEXITCODE -eq 0) { return $result.Trim() }
    } catch {}
    return ""
}

$RepoUrl = Get-GuestInfo "runner_repo"
$Token   = Get-GuestInfo "runner_token"
$Name    = Get-GuestInfo "runner_name"
$Labels  = Get-GuestInfo "runner_labels"

# Fall back to defaults
if (-not $Name)   { $Name = $env:COMPUTERNAME }
if (-not $Labels) { $Labels = "self-hosted,windows,x64" }

if (-not $RepoUrl -or -not $Token) {
    Write-Host "Missing guestinfo.runner_repo or guestinfo.runner_token, skipping auto-register"
    Stop-Transcript
    exit 0
}

Write-Host "Registering runner '$Name' for $RepoUrl"

$ErrorActionPreference = "Stop"
& "$RunnerDir\config.cmd" `
    --url $RepoUrl `
    --token $Token `
    --name $Name `
    --labels $Labels `
    --unattended `
    --replace `
    --runasservice

New-Item -ItemType File -Path $Marker -Force | Out-Null
Write-Host "$(Get-Date): Runner registered and started successfully"
Stop-Transcript
'@
$autoRegisterScript | Set-Content -Path "$RunnerDir\auto-register.ps1" -Encoding UTF8

# Install scheduled task for auto-registration on first boot
$action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument "-ExecutionPolicy Bypass -File $RunnerDir\auto-register.ps1"
$trigger = New-ScheduledTaskTrigger -AtStartup
$principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
Register-ScheduledTask -TaskName "GitHubRunnerAutoRegister" -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force | Out-Null

Write-Host "==> GitHub Actions runner v${RunnerVersion} installed to ${RunnerDir}"
Write-Host "==> Auto-registration task enabled (reads guestinfo.runner_repo + guestinfo.runner_token)"
