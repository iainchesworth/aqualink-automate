#!/usr/bin/env bash
set -euo pipefail

RUNNER_VERSION="${GITHUB_RUNNER_VERSION:-2.321.0}"
RUNNER_HOME="/home/runner"
RUNNER_DIR="${RUNNER_HOME}/actions-runner"

echo "==> Installing GitHub Actions runner v${RUNNER_VERSION}"

mkdir -p "${RUNNER_DIR}"

# Download runner
RUNNER_URL="https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz"
curl -fsSL "${RUNNER_URL}" | tar xz -C "${RUNNER_DIR}"

# Install runner dependencies
"${RUNNER_DIR}/bin/installdependencies.sh"

# Fix ownership
chown -R runner:runner "${RUNNER_DIR}"

# Create registration helper script (for manual use)
cat > "${RUNNER_DIR}/register-runner.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: $0 <TOKEN> <REPO_URL> [NAME] [LABELS]"
    echo "  TOKEN    - Runner registration token"
    echo "  REPO_URL - Repository URL (https://github.com/owner/repo)"
    echo "  NAME     - Runner name (default: hostname)"
    echo "  LABELS   - Comma-separated labels (default: self-hosted,linux,x64)"
    exit 1
fi

TOKEN="$1"
REPO_URL="$2"
NAME="${3:-$(hostname)}"
LABELS="${4:-self-hosted,linux,x64}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

"${SCRIPT_DIR}/config.sh" \
    --url "${REPO_URL}" \
    --token "${TOKEN}" \
    --name "${NAME}" \
    --labels "${LABELS}" \
    --unattended \
    --replace

# Install and start as a service
sudo "${SCRIPT_DIR}/svc.sh" install runner
sudo "${SCRIPT_DIR}/svc.sh" start
SCRIPT
chmod +x "${RUNNER_DIR}/register-runner.sh"

# Create auto-registration script that reads from vSphere guestinfo
cat > "${RUNNER_DIR}/auto-register.sh" <<'AUTOREGISTER'
#!/usr/bin/env bash
set -euo pipefail

RUNNER_DIR="/home/runner/actions-runner"
MARKER="${RUNNER_DIR}/.registered"
LOG="/var/log/runner-auto-register.log"

exec >> "$LOG" 2>&1
echo "$(date): Auto-register started"

# Skip if already registered
if [ -f "$MARKER" ]; then
    echo "Runner already registered, skipping"
    exit 0
fi

# Read guestinfo variables (set on the VM in vSphere)
get_guestinfo() {
    vmtoolsd --cmd "info-get guestinfo.$1" 2>/dev/null || echo ""
}

REPO_URL=$(get_guestinfo "runner_repo")
TOKEN=$(get_guestinfo "runner_token")
NAME=$(get_guestinfo "runner_name")
LABELS=$(get_guestinfo "runner_labels")
PAT=$(get_guestinfo "runner_pat")

# Fall back to defaults
NAME="${NAME:-$(hostname)}"
LABELS="${LABELS:-self-hosted,linux,x64}"

# Need the repo, plus EITHER a one-shot registration token (persistent mode) OR a PAT
# (ephemeral mode, which mints its own tokens each loop).
if [ -z "$REPO_URL" ] || { [ -z "$TOKEN" ] && [ -z "$PAT" ]; }; then
    echo "Missing guestinfo.runner_repo, or neither runner_token nor runner_pat set — skipping auto-register"
    exit 0
fi

# Give this clone a unique hostname matching its runner name. Together with the
# reset machine-id (10-cleanup.sh) this stops cloned VMs from presenting an
# identical DHCP client-id and colliding on the same DHCP lease.
if [ -n "$NAME" ]; then
    echo "Setting hostname to '${NAME}'"
    hostnamectl set-hostname "${NAME}" || true
fi

cd "${RUNNER_DIR}"

if [ -n "${PAT}" ]; then
    # Ephemeral mode: one job per registration, then re-register fresh -> every job
    # runs on a pristine runner (no accumulated _work/_temp corruption, orphaned
    # compiler processes, or thin-disk balloon between jobs). ephemeral-loop.sh and
    # its service are baked into the template; here we just persist the per-clone
    # config and start the loop.
    echo "Ephemeral mode: starting the ephemeral runner loop for '${NAME}'"
    cat > "${RUNNER_DIR}/.ephemeral-env" <<ENVV
REPO_URL=${REPO_URL}
OWNER_REPO=${REPO_URL#https://github.com/}
RUNNER_NAME=${NAME}
RUNNER_LABELS=${LABELS}
RUNNER_PAT=${PAT}
ENVV
    chmod 600 "${RUNNER_DIR}/.ephemeral-env"
    chown runner:runner "${RUNNER_DIR}/.ephemeral-env"
    systemctl enable --now github-runner-ephemeral.service
else
    # Persistent mode (default): register once with the one-shot token, run as a service.
    echo "Registering persistent runner '${NAME}' for ${REPO_URL}"
    sudo -u runner ./config.sh \
        --url "${REPO_URL}" \
        --token "${TOKEN}" \
        --name "${NAME}" \
        --labels "${LABELS}" \
        --unattended \
        --replace
    ./svc.sh install runner
    ./svc.sh start
fi

touch "$MARKER"
echo "$(date): Runner setup complete"
AUTOREGISTER
chmod +x "${RUNNER_DIR}/auto-register.sh"

# Ephemeral runner loop (used ONLY when guestinfo.runner_pat is set; auto-register.sh
# enables the service below in that case). One job per registration, then re-register
# fresh, so every job runs on a pristine runner.
cat > "${RUNNER_DIR}/ephemeral-loop.sh" <<'LOOP'
#!/usr/bin/env bash
set -uo pipefail
RUNNER_DIR=/home/runner/actions-runner
cd "$RUNNER_DIR"
# shellcheck disable=SC1091
[ -f "$RUNNER_DIR/.ephemeral-env" ] && source "$RUNNER_DIR/.ephemeral-env"
if [ -z "${RUNNER_PAT:-}" ] || [ -z "${OWNER_REPO:-}" ]; then
    echo "ephemeral-loop: missing .ephemeral-env config; idling"; sleep 300; exit 1
fi
while true; do
    # Mint a fresh single-use registration token (the PAT needs Administration:write).
    TOKEN=$(curl -fsSL -X POST \
        -H "Authorization: Bearer ${RUNNER_PAT}" \
        -H "Accept: application/vnd.github+json" \
        -H "X-GitHub-Api-Version: 2022-11-28" \
        "https://api.github.com/repos/${OWNER_REPO}/actions/runners/registration-token" \
        | grep -oP '"token"\s*:\s*"\K[^"]+' || true)
    if [ -z "${TOKEN:-}" ]; then
        echo "ephemeral-loop: could not mint a registration token; retrying in 30s"
        sleep 30; continue
    fi
    ./config.sh --url "$REPO_URL" --token "$TOKEN" --ephemeral \
        --name "$RUNNER_NAME" --labels "$RUNNER_LABELS" --unattended --replace \
        || { echo "ephemeral-loop: config failed; retrying in 30s"; sleep 30; continue; }
    # Runs exactly ONE job, then exits and de-registers (ephemeral).
    ./run.sh || true
    # Reset state before the next registration: stale workdirs + orphaned build procs.
    rm -rf "$RUNNER_DIR"/_work/* 2>/dev/null || true
    pkill -u runner -f 'cc1plus|cc1|lto1|ninja' 2>/dev/null || true
done
LOOP
chmod +x "${RUNNER_DIR}/ephemeral-loop.sh"
chown runner:runner "${RUNNER_DIR}/ephemeral-loop.sh"

cat > /etc/systemd/system/github-runner-ephemeral.service <<'UNIT'
[Unit]
Description=GitHub Actions Ephemeral Runner Loop
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=runner
WorkingDirectory=/home/runner/actions-runner
ExecStart=/home/runner/actions-runner/ephemeral-loop.sh
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
UNIT
# Intentionally NOT enabled here — auto-register.sh enables it only when
# guestinfo.runner_pat is set (ephemeral mode).

# Install systemd service for auto-registration on first boot
cat > /etc/systemd/system/github-runner-register.service <<'UNIT'
[Unit]
Description=GitHub Actions Runner Auto-Registration
After=network-online.target vmtoolsd.service
Wants=network-online.target
ConditionPathExists=!/home/runner/actions-runner/.registered

[Service]
Type=oneshot
ExecStart=/home/runner/actions-runner/auto-register.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
UNIT
systemctl daemon-reload
systemctl enable github-runner-register.service

echo "==> GitHub Actions runner v${RUNNER_VERSION} installed to ${RUNNER_DIR}"
echo "==> Auto-registration service enabled (reads guestinfo.runner_repo + guestinfo.runner_token)"
