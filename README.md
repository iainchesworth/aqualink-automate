# Aqualink Automate

A service designed to seamlessly integrate Jandy/Fluidra Aqualink RS pool controllers with Smart Homes without the need for cloud services and platforms. It exposes local control options via a Web UI or via MQTT and HTTP APIs allowing you to have more intuitive control over your Aqualink RS system.

## Table Of Content

- [Features](#features)
- [Getting Started](#getting-started)
- [Development](#development)
- [CI/CD Overview](#cicd-overview)
- [License](#license)

# Features

- Local control of your pool system, bypassing the need for cloud services
- Intuitive web-based user interface for easy control and monitoring
- Full API support (HTTP and MQTT) for custom integrations with Smart Homes
- **Matter** bridge for Apple Home / Google Home / Alexa / SmartThings (on by default; scan a QR to pair) — see [docs/MATTER.md](docs/MATTER.md)
- Detailed status information of your Aqualink RS system
- 
# Getting Started

See [INSTALL.md](INSTALL.md) for complete instructions covering pre-built binaries, building from source, dev containers, and Docker deployment.

Quick start:

```bash
git clone --recurse-submodules https://github.com/iainchesworth/aqualink-automate.git
cd aqualink-automate

# Linux/macOS
./cicd/build.sh

# Windows (PowerShell)
.\cicd\build.ps1
```

The build scripts validate dependencies, bootstrap vcpkg, and run configure/build/test automatically.

# Development

A VS Code **Dev Container** is provided with GCC 15, Clang 21, CMake, Ninja, and ccache pre-installed. Open the repository in VS Code and select **Reopen in Container**. See [INSTALL.md](INSTALL.md#development-container) for details.

For native development, the build scripts handle everything:
- `cicd/build.sh` — Linux and macOS (GCC or Clang)
- `cicd/build.ps1` — Windows (MSVC or Clang)

# CI/CD Overview

## Workflows

| Workflow | Job | Trigger |
|----------|-----|---------|
| `ci.yml` | Build and test (Linux, Windows, macOS) | Push to `main`/`feature/**`/`bug/**`, PRs to `develop`/`main` |
| `ci.yml` | Docker verification | Same as above |
| `automated-codescanning.yml` | CodeQL analysis | Push to `main`/`feature/**`/`bug/**`, PRs to `develop`/`main`, weekly schedule |
| `automated-codescanning.yml` | SonarCloud scan | Same as above |
| `automated-codescanning.yml` | MSVC code analysis | Same as above |
| `release.yml` | Package (Linux, Windows, macOS) | Push to `main`, version tags (`v*`) |
| `release.yml` | Docker publish (GHCR) | Push to `main`, version tags (`v*`) |
| `release.yml` | GitHub Release | Version tags only (`v*`) |

## Runner Configuration

Workflows support both GitHub-hosted and self-hosted runners. By default, GitHub-hosted runners are used (`ubuntu-latest`, `windows-latest`, `macos-latest`).

To use self-hosted runners, set these **repository variables** (Settings > Variables > Actions):

| Variable | Value | Example |
|----------|-------|---------|
| `RUNNER_LINUX` | JSON runner label array | `["self-hosted","linux","x64"]` |
| `RUNNER_WINDOWS` | JSON runner label array | `["self-hosted","windows","x64"]` |

macOS CI always runs on `macos-latest` (GitHub-hosted). If the variables are unset or the runners go offline, workflows automatically fall back to GitHub-hosted runners.

See [cicd/packer/README.md](cicd/packer/README.md) for instructions on provisioning self-hosted runner VMs.

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
