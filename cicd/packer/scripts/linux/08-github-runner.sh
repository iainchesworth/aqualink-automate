#!/usr/bin/env bash
set -euo pipefail

RUNNER_VERSION="${GITHUB_RUNNER_VERSION:-2.321.0}"
RUNNER_HOME="/home/runner"
RUNNER_DIR="${RUNNER_HOME}/actions-runner"

echo "==> Installing GitHub Actions runner v${RUNNER_VERSION} (ephemeral mode)"

mkdir -p "${RUNNER_DIR}"

# Download runner
RUNNER_URL="https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz"
curl -fsSL "${RUNNER_URL}" | tar xz -C "${RUNNER_DIR}"

# Install runner dependencies
"${RUNNER_DIR}/bin/installdependencies.sh"

# Fix ownership
chown -R runner:runner "${RUNNER_DIR}"

# ---------------------------------------------------------------------------
# Ephemeral runner supervisor
# ---------------------------------------------------------------------------
# Each VM runs ONE job per registration on a pristine workspace, then recycles:
# wipe the work dir + transient cruft, re-register with a freshly-minted token,
# run the next job. This guarantees every build starts clean and that GitHub can
# never hand a runner a second job in a dirty state. The persistent vcpkg/ccache
# caches on /data/cache are preserved (only size-capped) so CI stays fast.
#
# Replaces the old persistent `svc.sh install` service + one-shot
# `github-runner-register.service`. The runner now mints its own registration
# tokens from an on-box credential (so it can re-register every cycle without an
# external orchestrator), so the per-VM guestinfo gains `runner_pat` in place of
# the old short-lived `runner_token`.
cat > "${RUNNER_DIR}/run-ephemeral.sh" <<'SUPERVISOR'
#!/usr/bin/env bash
# GitHub Actions ephemeral runner supervisor — see 08-github-runner.sh.
#
# Runs as the unprivileged `runner` user (systemd User=runner); the few
# privileged reset steps use passwordless sudo. Configuration is read from
# vSphere guestinfo, set per-VM at deploy time (see cicd/packer/README.md):
#   guestinfo.runner_repo    https://github.com/<owner>/<repo>
#   guestinfo.runner_pat     fine-grained PAT, repo "Administration: read+write"
#   guestinfo.runner_name    (optional; defaults to the hostname)
#   guestinfo.runner_labels  (optional; defaults to self-hosted,linux,x64)
set -uo pipefail

RUNNER_DIR="/home/runner/actions-runner"
DATA="/data"
WORK="${DATA}/work"
CACHE="${DATA}/cache"

# Cache size caps (the data disk is ~20 GB shared between work/ and cache/).
CCACHE_MAX="${CCACHE_MAX:-4G}"
VCPKG_ARCHIVES_CAP_GB="${VCPKG_ARCHIVES_CAP_GB:-4}"

# Deterministic PATH for jobs: ccache shims first, then CMake (/usr/local/bin)
# and the apt toolchain (/usr/bin). Mirrors what `svc.sh install` used to capture.
export PATH="/usr/lib/ccache:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

log() { echo "$(date '+%Y-%m-%d %H:%M:%S') $*"; }
# Read a guestinfo key. Falls back to sudo so a setup where the non-root runner
# user can't reach the vmtoolsd RPC still registers (rather than silently
# defaulting). An unset key returns empty either way -> the caller's default applies.
gi() {
    local v
    v="$(vmtoolsd --cmd "info-get guestinfo.$1" 2>/dev/null | tr -d '\r')"
    [ -z "$v" ] && v="$(sudo -n vmtoolsd --cmd "info-get guestinfo.$1" 2>/dev/null | tr -d '\r')"
    printf '%s' "$v"
}

prune_dir_to_cap() {
    local dir="$1" cap_gb="$2"
    [ -d "$dir" ] || return 0
    local cap=$(( cap_gb * 1024 * 1024 ))   # KiB
    local used
    used=$(du -sk "$dir" 2>/dev/null | cut -f1)
    [ -n "${used:-}" ] || return 0
    [ "$used" -le "$cap" ] && return 0
    log "pruning $dir (${used}KiB > ${cap}KiB cap), least-recently-used first"
    find "$dir" -type f -printf '%A@ %p\n' 2>/dev/null | sort -n | cut -d' ' -f2- | \
    while IFS= read -r f; do
        used=$(du -sk "$dir" 2>/dev/null | cut -f1)
        [ "${used:-0}" -le "$cap" ] && break
        rm -f "$f" 2>/dev/null || true
    done
}

prune_vcpkg_cache() {
    # Host (Ubuntu-25.04) binary cache and the release glibc-2.36 (bookworm) cache
    # — both persist on the data volume and would otherwise grow unbounded.
    prune_dir_to_cap "${CACHE}/vcpkg/archives" "$VCPKG_ARCHIVES_CAP_GB"
    prune_dir_to_cap "${CACHE}/vcpkg-bookworm" "$VCPKG_ARCHIVES_CAP_GB"
}

reset_workspace() {
    log "resetting workspace + transient cruft"
    # Wipe the entire work dir; config.sh recreates it clean.
    sudo rm -rf "${WORK:?}" 2>/dev/null || true
    mkdir -p "$WORK"
    # Drop the last job's Docker images/containers/volumes/build cache.
    docker system prune -af --volumes >/dev/null 2>&1 \
        || sudo docker system prune -af --volumes >/dev/null 2>&1 || true
    # Transient temp dirs.
    sudo rm -rf /tmp/* /var/tmp/* >/dev/null 2>&1 || true
    # Preserve caches but bound their size.
    command -v ccache >/dev/null 2>&1 && ccache --max-size="$CCACHE_MAX" >/dev/null 2>&1 || true
    prune_vcpkg_cache
    # Reclaim freed blocks so the thin vmdk stays near actual usage.
    sudo fstrim -av >/dev/null 2>&1 || true
}

REPO_URL="$(gi runner_repo)"
RUNNER_PAT="$(gi runner_pat)"
NAME="$(gi runner_name)";     NAME="${NAME:-$(hostname)}"
LABELS="$(gi runner_labels)"; LABELS="${LABELS:-self-hosted,linux,x64}"

if [ -z "${REPO_URL:-}" ] || [ -z "${RUNNER_PAT:-}" ]; then
    log "FATAL: missing guestinfo.runner_repo or guestinfo.runner_pat; sleeping 5m"
    sleep 300
    exit 1
fi

# owner/repo slug for the REST API.
OWNER_REPO="$(printf '%s' "$REPO_URL" | sed -E 's#^https?://github.com/##; s#/+$##; s#\.git$##')"

# Unique hostname (keeps cloned VMs from colliding on a DHCP client-id).
sudo hostnamectl set-hostname "$NAME" 2>/dev/null || true

# Refuse to run if the data volume isn't mounted — otherwise work/ and cache/
# would silently land on the OS disk and reintroduce the exhaustion bug.
if ! mountpoint -q "$DATA"; then
    log "FATAL: ${DATA} is not mounted; refusing to run"
    sleep 60
    exit 1
fi

cd "$RUNNER_DIR" || { log "FATAL: ${RUNNER_DIR} missing"; exit 1; }
log "ephemeral supervisor started for ${OWNER_REPO} as '${NAME}' [${LABELS}]"

while true; do
    reset_workspace

    log "minting registration token"
    TOKEN="$(curl -fsSL -X POST \
        -H "Authorization: Bearer ${RUNNER_PAT}" \
        -H "Accept: application/vnd.github+json" \
        -H "X-GitHub-Api-Version: 2022-11-28" \
        "https://api.github.com/repos/${OWNER_REPO}/actions/runners/registration-token" \
        | jq -r '.token // empty')"

    if [ -z "${TOKEN:-}" ]; then
        log "could not obtain a registration token; retrying in 30s"
        sleep 30
        continue
    fi

    # Clear any stale local config from a previous unclean exit so config.sh
    # starts fresh; --replace re-claims the same runner name on the server.
    rm -f "${RUNNER_DIR}/.runner" "${RUNNER_DIR}/.credentials" \
          "${RUNNER_DIR}/.credentials_rsaparams" 2>/dev/null || true

    log "configuring ephemeral runner"
    if ! ./config.sh \
            --url "$REPO_URL" \
            --token "$TOKEN" \
            --name "$NAME" \
            --labels "$LABELS" \
            --work "$WORK" \
            --ephemeral \
            --replace \
            --unattended; then
        log "config.sh failed; retrying in 30s"
        sleep 30
        continue
    fi

    log "running one job"
    ./run.sh || true   # with --ephemeral, run.sh exits after a single job

    log "job complete; recycling"
done
SUPERVISOR
chmod +x "${RUNNER_DIR}/run-ephemeral.sh"
chown runner:runner "${RUNNER_DIR}/run-ephemeral.sh"

# systemd unit: run the supervisor as the runner user on boot, restart forever.
cat > /etc/systemd/system/github-runner.service <<'UNIT'
[Unit]
Description=GitHub Actions ephemeral runner supervisor
After=network-online.target vmtoolsd.service docker.service data.mount
Wants=network-online.target
Requires=data.mount

[Service]
Type=simple
User=runner
Group=runner
WorkingDirectory=/home/runner/actions-runner
ExecStart=/home/runner/actions-runner/run-ephemeral.sh
Restart=always
RestartSec=10
# Jobs can run long; never time the supervisor out.
TimeoutStartSec=0

[Install]
WantedBy=multi-user.target
UNIT

systemctl daemon-reload
systemctl enable github-runner.service

echo "==> GitHub Actions runner v${RUNNER_VERSION} installed to ${RUNNER_DIR}"
echo "==> Ephemeral supervisor enabled (github-runner.service); reads guestinfo.runner_repo + guestinfo.runner_pat"
