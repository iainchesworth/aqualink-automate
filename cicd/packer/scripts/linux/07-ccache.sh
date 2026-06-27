#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing and configuring ccache"

apt-get install -y --no-install-recommends ccache

# Configure ccache. cache_dir is pinned to the persistent data volume
# (/data/cache/ccache) so the cache survives the per-job workspace wipe and
# lives off the OS disk. max_size caps it at 4 GB (ccache evicts LRU internally)
# so it can't crowd out the work area on the shared ~20 GB data disk. The data
# volume is created by 00-data-volume.sh, which also symlinks ~/.cache ->
# /data/cache; the explicit cache_dir keeps ccache correct even if that symlink
# is ever absent.
RUNNER_HOME="/home/runner"
mkdir -p "${RUNNER_HOME}/.config/ccache"
cat > "${RUNNER_HOME}/.config/ccache/ccache.conf" <<'EOF'
cache_dir = /data/cache/ccache
max_size = 3G
compression = true
compression_level = 6
EOF
chown -R runner:runner "${RUNNER_HOME}/.config/ccache"
install -d -o runner -g runner /data/cache/ccache

# Add ccache to PATH for all users
echo 'export PATH="/usr/lib/ccache:$PATH"' > /etc/profile.d/ccache.sh

echo "==> ccache $(ccache --version | head -1) installed (3GB max on /data/cache, compression on)"
