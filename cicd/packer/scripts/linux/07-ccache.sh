#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing and configuring ccache"

apt-get install -y --no-install-recommends ccache

# Configure ccache defaults (per-user config will be in runner's home)
RUNNER_HOME="/home/runner"
mkdir -p "${RUNNER_HOME}/.config/ccache"
cat > "${RUNNER_HOME}/.config/ccache/ccache.conf" <<'EOF'
max_size = 5G
compression = true
compression_level = 6
EOF
chown -R runner:runner "${RUNNER_HOME}/.config/ccache"

# Add ccache to PATH for all users
echo 'export PATH="/usr/lib/ccache:$PATH"' > /etc/profile.d/ccache.sh

echo "==> ccache $(ccache --version | head -1) installed (5GB max, compression on)"
