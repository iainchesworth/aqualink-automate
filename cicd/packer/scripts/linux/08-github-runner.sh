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

# Fall back to defaults
NAME="${NAME:-$(hostname)}"
LABELS="${LABELS:-self-hosted,linux,x64}"

if [ -z "$REPO_URL" ] || [ -z "$TOKEN" ]; then
    echo "Missing guestinfo.runner_repo or guestinfo.runner_token, skipping auto-register"
    exit 0
fi

echo "Registering runner '${NAME}' for ${REPO_URL}"

cd "${RUNNER_DIR}"
sudo -u runner ./config.sh \
    --url "${REPO_URL}" \
    --token "${TOKEN}" \
    --name "${NAME}" \
    --labels "${LABELS}" \
    --unattended \
    --replace

# Install and start the runner service
./svc.sh install runner
./svc.sh start

touch "$MARKER"
echo "$(date): Runner registered and started successfully"
AUTOREGISTER
chmod +x "${RUNNER_DIR}/auto-register.sh"

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
