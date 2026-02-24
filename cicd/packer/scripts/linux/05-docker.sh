#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing Docker Engine CE"

# Add Docker GPG key and repository
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg
chmod a+r /etc/apt/keyrings/docker.gpg

CODENAME=$(lsb_release -cs)
DOCKER_REPO="https://download.docker.com/linux/ubuntu/dists/${CODENAME}"

# Verify Docker repo exists for this codename; fall back to nearest LTS if not
if ! curl -fsSL --head "${DOCKER_REPO}/Release" >/dev/null 2>&1; then
    echo "WARNING: Docker repo not available for '${CODENAME}', falling back to 'noble'"
    CODENAME="noble"
fi

echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu ${CODENAME} stable" \
    > /etc/apt/sources.list.d/docker.list

apt-get update
apt-get install -y --no-install-recommends \
    docker-ce \
    docker-ce-cli \
    containerd.io \
    docker-buildx-plugin \
    docker-compose-plugin

# Add runner user to docker group
usermod -aG docker runner

# Enable and start Docker
systemctl enable docker

echo "==> Docker $(docker --version) installed"
