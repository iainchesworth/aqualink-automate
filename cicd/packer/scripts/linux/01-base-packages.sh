#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing base packages"

apt-get update
apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    curl \
    git \
    gnupg \
    jq \
    libssl-dev \
    linux-libc-dev \
    lsb-release \
    pkg-config \
    software-properties-common \
    tar \
    unzip \
    wget \
    zip

echo "==> Base packages installed"
