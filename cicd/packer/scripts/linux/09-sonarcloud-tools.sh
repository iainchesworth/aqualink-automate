#!/usr/bin/env bash
set -euo pipefail

echo "==> Installing SonarCloud build-wrapper"

WRAPPER_DIR="/opt/sonarcloud"
mkdir -p "${WRAPPER_DIR}"

curl -fsSL "https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip" -o /tmp/build-wrapper.zip
unzip -q /tmp/build-wrapper.zip -d "${WRAPPER_DIR}"
rm /tmp/build-wrapper.zip

# Symlink to PATH
ln -sf "${WRAPPER_DIR}/build-wrapper-linux-x86/build-wrapper-linux-x86-64" /usr/local/bin/build-wrapper-linux-x86-64

echo "==> SonarCloud build-wrapper installed to ${WRAPPER_DIR}"
