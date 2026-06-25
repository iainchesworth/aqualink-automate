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

# Read guestinfo variables (set on the VM in vSphere)
# Note: Must use Start-Process or cmd /c to preserve the single-argument
# semantics of --cmd "info-get guestinfo.xxx".  PowerShell's & operator
# splits the quoted string into separate tokens which vmtoolsd rejects.
function Get-GuestInfo($key) {
    try {
        $vmtoolsd = "C:\Program Files\VMware\VMware Tools\vmtoolsd.exe"
        $tmpFile = [System.IO.Path]::GetTempFileName()
        $proc = Start-Process -FilePath $vmtoolsd `
            -ArgumentList "--cmd `"info-get guestinfo.$key`"" `
            -Wait -PassThru -NoNewWindow `
            -RedirectStandardOutput $tmpFile 2>$null
        if ($proc.ExitCode -eq 0) {
            $result = Get-Content $tmpFile -Raw -ErrorAction SilentlyContinue
            Remove-Item $tmpFile -Force -ErrorAction SilentlyContinue
            if ($result) { return $result.Trim() }
        }
        Remove-Item $tmpFile -Force -ErrorAction SilentlyContinue
    } catch {}
    return ""
}

$RepoUrl = Get-GuestInfo "runner_repo"
$Token   = Get-GuestInfo "runner_token"
$Name    = Get-GuestInfo "runner_name"
$Labels  = Get-GuestInfo "runner_labels"
$Pat     = Get-GuestInfo "runner_pat"

# Fall back to defaults
if (-not $Name)   { $Name = $env:COMPUTERNAME }
if (-not $Labels) { $Labels = "self-hosted,windows,x64" }

# Need the repo, plus EITHER a one-shot registration token (persistent) OR a PAT
# (ephemeral mode, which mints its own tokens each loop).
if (-not $RepoUrl -or (-not $Token -and -not $Pat)) {
    Write-Host "Missing guestinfo.runner_repo, or neither runner_token nor runner_pat set; skipping auto-register"
    Stop-Transcript
    exit 0
}

if ($Pat) {
    # Ephemeral mode: one job per registration, then re-register fresh -> every job
    # runs on a pristine runner (no accumulated _work corruption or orphaned cl.exe /
    # mspdbsrv between jobs). Needs a PAT (Administration:write) to mint a fresh
    # registration token per loop. This boot-time task runs the loop forever (no
    # marker), so a reboot re-enters the loop.
    Write-Host "Ephemeral mode: starting the ephemeral runner loop for '$Name'"
    $ownerRepo = $RepoUrl -replace '^https://github.com/', ''
    while ($true) {
        try {
            $resp = Invoke-RestMethod -Method Post `
                -Uri "https://api.github.com/repos/$ownerRepo/actions/runners/registration-token" `
                -Headers @{ Authorization = "Bearer $Pat"; Accept = "application/vnd.github+json"; "X-GitHub-Api-Version" = "2022-11-28" }
            $tok = $resp.token
        } catch { Write-Host "ephemeral-loop: token mint failed: $_"; Start-Sleep -Seconds 30; continue }
        if (-not $tok) { Write-Host "ephemeral-loop: empty token; retrying in 30s"; Start-Sleep -Seconds 30; continue }
        & "$RunnerDir\config.cmd" --url $RepoUrl --token $tok --ephemeral --name $Name --labels $Labels --unattended --replace
        # Runs exactly ONE job, then exits and de-registers (ephemeral).
        & "$RunnerDir\run.cmd"
        # Reset state before the next registration.
        Remove-Item "$RunnerDir\_work\*" -Recurse -Force -ErrorAction SilentlyContinue
        Get-Process cl, mspdbsrv, vctip, ninja -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    }
} else {
    # Persistent mode (default): register once and run as a service.
    if (Test-Path $Marker) {
        Write-Host "Runner already registered (persistent), skipping"
        Stop-Transcript
        exit 0
    }
    Write-Host "Registering persistent runner '$Name' for $RepoUrl"
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
}
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
