# Aqualink Automate

A service designed to seamlessly integrate Jandy/Fluidra Aqualink RS pool controllers with Smart Homes without the need for cloud services and platforms. It exposes local control options via a Web UI or via MQTT and HTTP APIs allowing you to have more intuitive control over your Aqualink RS system.

## Table Of Content

- [Features](#features)
- [Getting Started](#getting-started)
- [Development Container](#development-container)
- [API Documentation](#api-documentation)
- [Runtime Container](#runtime-container)
- [CI/CD Overview](#cicd-overview)
- [License](#license)

# Features

- Local control of your pool system, bypassing the need for cloud services
- Intuitive web-based user interface for easy control and monitoring
- Full API support (HTTP and MQTT) for custom integrations with Smart Homes
- Detailed status information of your Aqualink RS system
- 
# Getting Started

To get started, clone the repository and follow the instructions in the [INSTALL.md](INSTALL.md) file. It contains step-by-step directions to install and configure the Aqualink Automate service on your system.

# Development Container

A Dev Container configuration is provided for VS Code. It builds the `dev` target from the Dockerfile with GCC-15, clang-tidy, CMake, Ninja, and ccache pre-installed.

To get started, clone the repository with submodules and open in VS Code:

```bash
git clone --recurse-submodules https://github.com/iainchesworth/aqualink-automate.git
cd aqualink-automate
code .
```

When prompted, select **Reopen in Container**. The `postCreateCommand` initializes submodules and bootstraps vcpkg automatically. Named volumes persist ccache and vcpkg binary caches across container rebuilds.

Inside the container, use the standard CMake preset workflow:

```bash
cmake --preset config-linux-gcc
cmake --build --preset build-linux-gcc
ctest --preset test-linux-gcc
```

# API Documentation

The OpenAPI spec is served by the application at `GET /api/swagger.yaml` when the app is running.

For development, a Swagger UI service is available without needing the app running:

```bash
docker compose --profile docs up
```

This starts Swagger UI at `http://localhost:8080`, serving the spec directly from the repository via bind mount.

# Runtime Container

Build and run the production container:

```bash
docker build --target runtime -t aqualink-automate .
docker run -p 80:80 aqualink-automate
```

Defaults: HTTP on `0.0.0.0:80`, HTTPS disabled, doc-root set to `web`.

To connect a serial device:

```bash
docker run -p 80:80 --device /dev/ttyUSB0 aqualink-automate --serial-port /dev/ttyUSB0
```

To enable HTTPS, mount certificates and override the default command:

```bash
docker run -p 443:443 \
  -v /path/to/certs:/opt/aqualink-automate/ssl:ro \
  aqualink-automate \
  --address 0.0.0.0 --doc-root web
```

Or use `docker compose up` for a simpler workflow with the included `docker-compose.yml`.

# CI/CD Overview

| Workflow | Job | Trigger |
|----------|-----|---------|
| `ci.yml` | `build-and-test` (Linux, Windows, macOS) | Push to `develop`/`main`/`feature/**`/`bug/**`, PRs to `develop`/`main` |
| `ci.yml` | `docker-verify` | Same as above |
| `release.yml` | `build-packages` (Linux, Windows, macOS) | Push to `main`, version tags (`v*`) |
| `release.yml` | `docker-publish` (GHCR) | Push to `main`, version tags (`v*`) |
| `release.yml` | `github-release` | Version tags only (`v*`) |

# Changelog

You can view a comprehensive log of all changes, updates, and bug fixes in the [CHANGELOG.md](CHANGELOG.md) file. This document is updated with every release and is a great way to stay informed about the latest improvements to Aqualink Automate.

# License

The code in this repository is licensed under the GNU General Public License v3.0. See the [LICENSE.txt](LICENSE.txt) file for details.

# Getting Involved

Contributions are welcome to help the project grow and improve; the key focus areas are currently:
- New Features: There's a long list of features that are necessary to achieve v1.0.0.
- Bug Fixes: If you've noticed a bug or have suggestions of how to address one, raise an issue in the issue tracker or submit a pull-request.
Please see [CONTRIBUTING.md](CONTRIBUTING.md) for detailed instructions on how to contribute.

# Credits and References

This project has been heavily influenced by sfeakes' [AqualinkD](https://github.com/sfeakes/AqualinkD).  The original "itch" that this project intended to scratch was to make a variant of AqualinkD that works on Windows and MacOS.
