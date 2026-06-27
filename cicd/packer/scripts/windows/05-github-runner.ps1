#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

$RunnerVersion = $env:GITHUB_RUNNER_VERSION
if (-not $RunnerVersion) { $RunnerVersion = "2.321.0" }

$RunnerDir = "C:\actions-runner"

Write-Host "==> Installing GitHub Actions runner v${RunnerVersion} (ephemeral mode)"

New-Item -ItemType Directory -Path $RunnerDir -Force | Out-Null

# Download runner
$runnerUrl = "https://github.com/actions/runner/releases/download/v${RunnerVersion}/actions-runner-win-x64-${RunnerVersion}.zip"
$runnerZip = "$env:TEMP\actions-runner.zip"
Invoke-WebRequest -Uri $runnerUrl -OutFile $runnerZip -UseBasicParsing
Expand-Archive -Path $runnerZip -DestinationPath $RunnerDir -Force
Remove-Item $runnerZip -Force

# ---------------------------------------------------------------------------
# Ephemeral runner supervisor
# ---------------------------------------------------------------------------
# Each VM runs ONE job per registration on a pristine workspace, then recycles:
# wipe D:\work + transient cruft, re-register with a freshly-minted token, run the
# next job. Guarantees every build starts clean and that GitHub can never hand a
# runner a second job in a dirty state. The persistent vcpkg/ccache caches on
# D:\cache are preserved (only size-capped) so CI stays fast.
#
# Replaces the old persistent `--runasservice` registration + GitHubRunnerAutoRegister
# task. The runner mints its own registration tokens from an on-box credential
# (so it can re-register every cycle without an external orchestrator), so the
# per-VM guestinfo gains `runner_pat` in place of the old short-lived `runner_token`.
$supervisor = @'
# GitHub Actions ephemeral runner supervisor — see 05-github-runner.ps1.
# Runs as the `runner` account (a local admin) via the GitHubRunnerEphemeral
# scheduled task. Configuration is read from vSphere guestinfo, set per-VM at
# deploy time (see cicd/packer/README.md):
#   guestinfo.runner_repo    https://github.com/<owner>/<repo>
#   GitHub App (preferred) — all three required:
#     guestinfo.runner_app_id               the App's ID
#     guestinfo.runner_app_installation_id  the App's installation ID on the repo
#     guestinfo.runner_app_private_key      the App PEM, base64-encoded (base64 -w0)
#   guestinfo.runner_pat     PAT fallback (repo "Administration: read+write")
#   guestinfo.runner_name    (optional; defaults to the computer name)
#   guestinfo.runner_labels  (optional; defaults to self-hosted,windows,x64)
$ErrorActionPreference = "Continue"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$RunnerDir = "C:\actions-runner"
$Work  = "D:\work"
$Cache = "D:\cache"
$CcacheMax = "3G"
$VcpkgArchivesCapGB = 3
$VcpkgDownloadsCapGB = 2

function Log($m) { Write-Host ("{0} {1}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $m) }

# Read a guestinfo key. Start-Process preserves the single-argument semantics of
# --cmd "info-get guestinfo.xxx" that PowerShell's & operator would split.
function Get-GuestInfo($key) {
    try {
        $vmtoolsd = "C:\Program Files\VMware\VMware Tools\vmtoolsd.exe"
        $tmp = [System.IO.Path]::GetTempFileName()
        $proc = Start-Process -FilePath $vmtoolsd `
            -ArgumentList "--cmd `"info-get guestinfo.$key`"" `
            -Wait -PassThru -NoNewWindow -RedirectStandardOutput $tmp 2>$null
        if ($proc.ExitCode -eq 0) {
            $r = Get-Content $tmp -Raw -ErrorAction SilentlyContinue
            Remove-Item $tmp -Force -ErrorAction SilentlyContinue
            if ($r) { return $r.Trim() }
        }
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
    } catch {}
    return ""
}

function Prune-DirToCap($dir, $capGB) {
    if (-not (Test-Path $dir)) { return }
    $cap = $capGB * 1GB
    $files = Get-ChildItem -Path $dir -Recurse -File -ErrorAction SilentlyContinue | Sort-Object LastAccessTime
    $total = ($files | Measure-Object Length -Sum).Sum
    if (-not $total -or $total -le $cap) { return }
    Log ("pruning $dir ({0:N1} GB > {1} GB cap), least-recently-used first" -f ($total / 1GB), $capGB)
    foreach ($f in $files) {
        if ($total -le $cap) { break }
        $total -= $f.Length
        Remove-Item $f.FullName -Force -ErrorAction SilentlyContinue
    }
}

function Prune-VcpkgCache {
    Prune-DirToCap "$Cache\vcpkg\archives"  $VcpkgArchivesCapGB
    Prune-DirToCap "$Cache\vcpkg\downloads" $VcpkgDownloadsCapGB
}

function Reset-Workspace {
    Log "resetting workspace + transient cruft"
    # Wipe the entire work dir; config.cmd recreates it clean.
    if (Test-Path $Work) { Remove-Item -Path $Work -Recurse -Force -ErrorAction SilentlyContinue }
    New-Item -ItemType Directory -Path $Work -Force | Out-Null
    # Transient temp dirs.
    Remove-Item "$env:TEMP\*", "C:\Windows\Temp\*" -Recurse -Force -ErrorAction SilentlyContinue
    # The runner's _diag logs live on the OS disk and accumulate per job; the
    # previous job's are already uploaded, so clear them to bound OS-disk growth.
    Remove-Item "$RunnerDir\_diag\*" -Recurse -Force -ErrorAction SilentlyContinue
    # Preserve caches but bound their size.
    & ccache --max-size $CcacheMax 2>$null | Out-Null
    Prune-VcpkgCache
    # Reclaim freed blocks (TRIM/UNMAP) so the thin vmdk stays near actual usage.
    Optimize-Volume -DriveLetter D -ReTrim -ErrorAction SilentlyContinue
}

# --- token minting ---------------------------------------------------------
function Get-OpenSsl {
    foreach ($p in @("C:\Program Files\Git\usr\bin\openssl.exe",
                     "C:\Program Files\Git\mingw64\bin\openssl.exe")) {
        if (Test-Path $p) { return $p }
    }
    return "openssl"   # fall back to PATH
}

function ConvertTo-B64Url([byte[]]$bytes) {
    [Convert]::ToBase64String($bytes).TrimEnd('=').Replace('+', '-').Replace('/', '_')
}

# Build a short-lived (<=10 min) RS256 App JWT. .NET Framework 4.8 can't import a
# PEM key, so sign via Git's openssl over temp files (binary-safe, no pipeline).
function New-AppJwt($appId, $keyPath) {
    $openssl = Get-OpenSsl
    $utf8 = New-Object Text.UTF8Encoding($false)
    $now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $header  = '{"alg":"RS256","typ":"JWT"}'
    $payload = '{"iat":' + ($now - 60) + ',"exp":' + ($now + 540) + ',"iss":"' + $appId + '"}'
    $unsigned = (ConvertTo-B64Url $utf8.GetBytes($header)) + "." + (ConvertTo-B64Url $utf8.GetBytes($payload))
    $uPath = Join-Path $env:TEMP "jwt_unsigned.txt"
    $sPath = Join-Path $env:TEMP "jwt_sig.bin"
    [IO.File]::WriteAllText($uPath, $unsigned, $utf8)
    & $openssl dgst -sha256 -sign $keyPath -out $sPath $uPath 2>$null
    $sig = ConvertTo-B64Url ([IO.File]::ReadAllBytes($sPath))
    Remove-Item $uPath, $sPath -Force -ErrorAction SilentlyContinue
    return "$unsigned.$sig"
}

# Obtain a runner registration token. Prefers the GitHub App (JWT -> installation
# token -> registration token); falls back to the PAT.
function Get-RegistrationToken {
    $auth = $null
    if ($script:HaveApp) {
        $keyPath = Join-Path $env:TEMP "ghapp.pem"
        try { [IO.File]::WriteAllBytes($keyPath, [Convert]::FromBase64String($AppKeyB64)) }
        catch { Log "could not decode runner_app_private_key: $($_.Exception.Message)"; return $null }
        $jwt = New-AppJwt $AppId $keyPath
        Remove-Item $keyPath -Force -ErrorAction SilentlyContinue
        try {
            $r = Invoke-RestMethod -Method Post `
                -Uri "https://api.github.com/app/installations/$AppInstallId/access_tokens" `
                -Headers @{ Authorization = "Bearer $jwt"; Accept = "application/vnd.github+json"; "X-GitHub-Api-Version" = "2022-11-28"; "User-Agent" = "aqualink-runner" }
            $auth = $r.token
        } catch { Log "App installation-token request failed: $($_.Exception.Message)"; return $null }
    } else {
        $auth = $Pat
    }
    try {
        $r2 = Invoke-RestMethod -Method Post `
            -Uri "https://api.github.com/repos/$OwnerRepo/actions/runners/registration-token" `
            -Headers @{ Authorization = "Bearer $auth"; Accept = "application/vnd.github+json"; "X-GitHub-Api-Version" = "2022-11-28"; "User-Agent" = "aqualink-runner" }
        return $r2.token
    } catch { Log "registration-token request failed: $($_.Exception.Message)"; return $null }
}

# Refuse to run if the data volume is missing, else work/cache would land on C:.
if (-not (Test-Path "D:\")) {
    Log "FATAL: D: (data volume) is missing; refusing to run"
    Start-Sleep -Seconds 60
    exit 1
}

$RepoUrl = Get-GuestInfo "runner_repo"
$Name    = Get-GuestInfo "runner_name"
$Labels  = Get-GuestInfo "runner_labels"
if (-not $Name)   { $Name = $env:COMPUTERNAME }
if (-not $Labels) { $Labels = "self-hosted,windows,x64" }

# Credentials for minting registration tokens. Preferred: a GitHub App (short-
# lived, scoped, revocable); fallback: a fine-grained PAT.
$AppId        = Get-GuestInfo "runner_app_id"
$AppInstallId = Get-GuestInfo "runner_app_installation_id"
$AppKeyB64    = Get-GuestInfo "runner_app_private_key"
$Pat          = Get-GuestInfo "runner_pat"
$HaveApp = [bool]($AppId -and $AppInstallId -and $AppKeyB64)

if (-not $RepoUrl -or (-not $HaveApp -and -not $Pat)) {
    Log "FATAL: need guestinfo.runner_repo and either the runner_app_* trio or runner_pat; sleeping 5m"
    Start-Sleep -Seconds 300
    exit 1
}

$OwnerRepo = ($RepoUrl -replace '^https?://github.com/', '' -replace '/+$', '' -replace '\.git$', '')

Set-Location $RunnerDir
Log "ephemeral supervisor started for $OwnerRepo as '$Name' [$Labels]"

while ($true) {
    Reset-Workspace

    Log "minting registration token"
    $Token = Get-RegistrationToken
    if (-not $Token) { Log "no token; retrying in 30s"; Start-Sleep -Seconds 30; continue }

    # Clear any stale local config from a previous unclean exit so config.cmd
    # starts fresh; --replace re-claims the same runner name on the server.
    Remove-Item "$RunnerDir\.runner", "$RunnerDir\.credentials", "$RunnerDir\.credentials_rsaparams" `
        -Force -ErrorAction SilentlyContinue

    Log "configuring ephemeral runner"
    & "$RunnerDir\config.cmd" --url $RepoUrl --token $Token --name $Name --labels $Labels `
        --work $Work --ephemeral --replace --unattended
    if ($LASTEXITCODE -ne 0) { Log "config.cmd failed ($LASTEXITCODE); retrying in 30s"; Start-Sleep -Seconds 30; continue }

    Log "running one job"
    & "$RunnerDir\run.cmd"   # with --ephemeral, run.cmd exits after a single job

    Log "job complete; recycling"
}
'@
$supervisor | Set-Content -Path "$RunnerDir\run-ephemeral.ps1" -Encoding UTF8

# Scheduled task: run the supervisor as the `runner` admin account at startup
# (so the runner's $HOME/.cache junction -> D: applies), restart on failure.
# Runs "whether logged on or not", which requires the account password; it is
# passed in from the Packer winrm_password var (same secret the autounattend
# already sets for this account).
$runnerPassword = $env:RUNNER_PASSWORD
if (-not $runnerPassword) { $runnerPassword = "Packer@2024!" }

$action  = New-ScheduledTaskAction -Execute "powershell.exe" `
    -Argument "-NoProfile -ExecutionPolicy Bypass -File $RunnerDir\run-ephemeral.ps1"
$trigger = New-ScheduledTaskTrigger -AtStartup
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
    -ExecutionTimeLimit ([TimeSpan]::Zero) -RestartCount 999 -RestartInterval (New-TimeSpan -Minutes 1)
# Run as `runner` (a local admin) "whether logged on or not" — that needs the
# stored password, which is the -User/-Password parameter set; -RunLevel Highest
# gives the elevation the workspace reset (Optimize-Volume) requires. Do NOT also
# pass -Principal here: it belongs to a different parameter set and combining the
# two makes Register-ScheduledTask fail to resolve a parameter set.
Register-ScheduledTask -TaskName "GitHubRunnerEphemeral" -Action $action -Trigger $trigger `
    -Settings $settings -User "runner" -Password $runnerPassword -RunLevel Highest -Force | Out-Null

Write-Host "==> GitHub Actions runner v${RunnerVersion} installed to ${RunnerDir}"
Write-Host "==> Ephemeral supervisor enabled (GitHubRunnerEphemeral task); reads guestinfo.runner_repo + guestinfo.runner_pat"
