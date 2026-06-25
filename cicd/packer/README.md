# Self-Hosted GitHub Actions Runners

Packer templates for building self-hosted GitHub Actions runner VMs on VMware vSphere (ESXi). These runners provide persistent build caches, faster CI times, and full control over the build environment.

## Architecture

| Runner | Base OS | CPUs | RAM | Disk | Pre-installed toolchain |
|--------|---------|------|-----|------|------------------------|
| Linux | Ubuntu 25.04 | 32 | 48 GB | 150 GB | GCC 15, Clang/LLVM 21, CMake, Ninja, Docker, ccache, SonarCloud build-wrapper |
| Windows | Windows Server 2022 | 32 | 48 GB | 150 GB | VS 2022 Build Tools (MSVC v143), CMake, Ninja, NSIS, ccache |

macOS CI stays on GitHub-hosted `macos-latest` runners.

## Prerequisites

- [Packer](https://www.packer.io/) 1.9+
- VMware vSphere / ESXi with vCenter access
- `xorriso`, `curl`, and `sha256sum` (available in WSL/Linux) for the Ubuntu repack step
- Internet access — the OS ISOs are **downloaded on demand** at build time rather than
  stored in the repo:
  - Ubuntu 25.04 Server source → fetched + repacked with autoinstall by `repack-iso.sh`
  - Windows Server 2022 evaluation → fetched directly by Packer from its default `iso_url`

## Configuration

### 1. Copy the example files

```bash
cd cicd/packer
cp variables.pkrvars.hcl.example variables.pkrvars.hcl
cp secrets.auto.pkrvars.hcl.example secrets.auto.pkrvars.hcl
```

### 2. Edit `variables.pkrvars.hcl`

Fill in your vSphere connection details and runner version. ISO sources are **not** set
here — each template defaults its own `iso_url` (Ubuntu → the local repack output,
Windows → the Microsoft eval download), so this shared file can't accidentally feed one
OS's ISO to the other build:

```hcl
vcenter_server     = "vcenter.example.com"
vcenter_username   = "administrator@vsphere.local"
vcenter_datacenter = "Datacenter"
vcenter_cluster    = "Cluster"
vcenter_datastore  = "datastore1"
vcenter_network    = "VM Network"
vcenter_folder     = "Templates"

github_runner_version = "2.321.0"
```

### 3. Edit `secrets.auto.pkrvars.hcl`

```hcl
vcenter_password = "your-vcenter-password"
ssh_password     = "packer"
winrm_password   = "Packer@2024!"
```

Both `*.pkrvars.hcl` files are gitignored. Only the `.example` files are committed.

## Building Templates

Initialize the Packer plugins (first time only):

```bash
cd cicd/packer
packer init linux-runner.pkr.hcl
packer init windows-runner.pkr.hcl
```

### Linux

Download the upstream Ubuntu source and repack it with autoinstall (run in WSL/Linux —
needs `xorriso`/`curl`/`sha256sum`). With no arguments it fetches the default Ubuntu
25.04 source from old-releases, verifies its SHA256, caches it under `ISOs/`, and writes
`ISOs/ubuntu-25.04-autoinstall.iso`:

```bash
./repack-iso.sh
```

Then build — no `-var iso_url=` needed, the template defaults to the repack output:

```bash
packer build -force \
  -var-file=variables.pkrvars.hcl \
  -var-file=secrets.auto.pkrvars.hcl \
  linux-runner.pkr.hcl
```

### Windows

Packer downloads the eval ISO itself (into `packer_cache/`) from the default `iso_url`,
so there is nothing to pre-stage:

```bash
packer build -force \
  -var-file=variables.pkrvars.hcl \
  -var-file=secrets.auto.pkrvars.hcl \
  windows-runner.pkr.hcl
```

> If Microsoft refreshes the eval build, the bundled `iso_checksum` will mismatch and the
> download fails fast — update `iso_url`/`iso_checksum` in `windows-runner.pkr.hcl` (or
> override with `-var`) to match the current build.

This creates two VM templates in vCenter:
- `tpl-github-runner-linux`
- `tpl-github-runner-windows`

## Deploying Runners

### 1. Clone VMs from templates

In vCenter, clone each template to a new VM. Name them as you like (e.g. `github-runner-linux`, `github-runner-windows`).

### 2. Generate a registration token

```bash
gh api repos/{owner}/aqualink-automate/actions/runners/registration-token \
  --method POST -q '.token'
```

### 3. Set guestinfo variables

Before booting, set these guestinfo properties on each VM (via vCenter > VM > Configure > vApp Options, or using `govc`):

| Variable | Value |
|----------|-------|
| `guestinfo.runner_token` | Registration token from step 2 |
| `guestinfo.runner_repo` | `https://github.com/{owner}/aqualink-automate` |
| `guestinfo.runner_name` | e.g. `linux-runner` or `windows-runner` |
| `guestinfo.runner_labels` | e.g. `self-hosted,linux,x64` or `self-hosted,windows,x64` |

> These key names must match exactly what the auto-registration scripts read
> (`scripts/linux/08-github-runner.sh`, `scripts/windows/05-github-runner.ps1`):
> `guestinfo.runner_token`, `guestinfo.runner_repo`, `guestinfo.runner_name`,
> `guestinfo.runner_labels`. A mismatch makes auto-register read empty values and
> silently skip (the most common "runner never registered" cause).

Using `govc`:

```bash
govc vm.change -vm github-runner-linux \
  -e guestinfo.runner_token="AXXXXXXXXXXXX" \
  -e guestinfo.runner_repo="https://github.com/{owner}/aqualink-automate" \
  -e guestinfo.runner_name="linux-runner" \
  -e guestinfo.runner_labels="self-hosted,linux,x64"
```

### 4. Boot the VMs

On first boot, the auto-registration service reads the guestinfo variables and registers the runner with GitHub. A `.registered` marker file prevents re-registration on subsequent reboots.

- **Linux**: systemd service (`github-runner-register.service`) runs at boot
- **Windows**: Scheduled task (`GitHubRunnerAutoRegister`) runs at startup

### 4b. (Optional) Ephemeral mode for heavy/contended jobs

Persistent runners accumulate state between jobs — stale `_work` trees, orphaned
compiler processes (`cc1plus`/`cl.exe`/`mspdbsrv`), and thin-disk balloon. On a
single oversubscribed ESXi host this is the main cause of the flaky deaths in the
heavy code-scanning jobs (CodeQL / SonarCloud / MSVC analysis), where a job is
reported "cancelled" mid-build with orphaned processes left behind.

**Ephemeral mode** makes each runner take exactly one job, then re-register fresh,
so every job starts on a pristine runner. It is **opt-in**: set one extra guestinfo
variable and the auto-register script switches modes (no template change needed —
the loop is already baked in).

| Variable | Value |
|----------|-------|
| `guestinfo.runner_pat` | A fine-grained PAT with **Administration: read & write** on this repo (used to mint a fresh single-use registration token per job) |

```bash
govc vm.change -vm github-runner-windows \
  -e guestinfo.runner_pat="github_pat_XXXXXXXX" \
  -e guestinfo.runner_repo="https://github.com/{owner}/aqualink-automate" \
  -e guestinfo.runner_name="windows-runner" \
  -e guestinfo.runner_labels="self-hosted,windows,x64"
```

When `guestinfo.runner_pat` is set, `guestinfo.runner_token` is **not** required
(the loop mints its own tokens). Without the PAT, the runner stays in the default
persistent (`--runasservice`) mode using `runner_token` as before.

> **Security tradeoff:** the PAT is stored on the runner VM (`.ephemeral-env`,
> chmod 600, on Linux). Scope it to *only* this repo with the minimum
> `Administration: write` permission, and rotate it if a VM is decommissioned. If
> that tradeoff is unacceptable, keep persistent mode and instead rely on the
> serialized scan ordering (CodeQL → SonarCloud → MSVC) in
> `.github/workflows/automated-codescanning.yml`, which already ensures only one
> instrumented build runs at a time on the single host.

**Roll out on ONE runner first** and watch a full scan cycle before converting the
fleet — the ephemeral scripts have not been exercised end-to-end against live
vSphere from this change.

### 5. Enable in workflows

Set these **repository variables** in GitHub (Settings > Variables > Actions):

| Variable | Value |
|----------|-------|
| `RUNNER_LINUX` | `["self-hosted","linux","x64"]` |
| `RUNNER_WINDOWS` | `["self-hosted","windows","x64"]` |

Workflows automatically use self-hosted runners when these variables are set. Remove the variables to fall back to GitHub-hosted runners.

## What's Installed

### Linux Runner

| Script | Packages |
|--------|----------|
| `01-base-packages.sh` | build-essential, ca-certificates, curl, git, gpg, jq, pkg-config, tar, unzip, wget, zip; enables `discard` (continuous TRIM) on the root fs **and** sets `fstrim.timer` to hourly so the thin vmdk stays lean under churny CI |
| `02-gcc-toolchain.sh` | gcc-15, g++-15, gcov-15, update-alternatives symlinks |
| `03-llvm-toolchain.sh` | clang-21, clang-tidy-21, lld-21, libc++-21-dev from apt.llvm.org |
| `04-cmake-ninja.sh` | CMake 3.31.6 (Kitware binary), ninja-build |
| `05-docker.sh` | Docker Engine CE + buildx plugin |
| `06-dev-tools.sh` | python3, gcovr, autoconf, automake, libtool |
| `07-ccache.sh` | ccache (5 GB max, compression enabled) |
| `08-github-runner.sh` | GitHub Actions runner agent + auto-registration service |
| `09-sonarcloud-tools.sh` | SonarCloud build-wrapper for Linux |
| `10-cleanup.sh` | apt clean, log truncation, `fstrim` (UNMAP) free-space reclaim to keep the thin vmdk small |

### Windows Runner

| Script | Packages |
|--------|----------|
| `01-base-config.ps1` | TLS 1.2, Chocolatey, Git |
| `02-vs-buildtools.ps1` | VS 2022 Build Tools (MSVC v143, Windows SDK 22621, MSBuild, ASan) |
| `03-cmake-ninja.ps1` | CMake + Ninja via Chocolatey |
| `04-choco-packages.ps1` | NSIS, 7zip, ccache (5 GB, compression enabled) |
| `05-github-runner.ps1` | GitHub Actions runner agent + auto-registration scheduled task |
| `06-cleanup.ps1` | DISM cleanup, temp removal |

## Maintenance

### Runner agent updates

The GitHub Actions runner agent auto-updates itself. No manual intervention needed.

### Rebuilding templates

When toolchain versions change, update the provisioning scripts and rebuild (re-run
`./repack-iso.sh` first for Linux — it reuses the cached source if present):

```bash
packer build -force -var-file=variables.pkrvars.hcl -var-file=secrets.auto.pkrvars.hcl linux-runner.pkr.hcl
```

Then deploy new VMs from the updated template and re-register.

### Reclaiming disk space

The ISOs are transient — fetched on demand and regenerated on the next build. Free the
disk between builds by emptying `ISOs/` and `packer_cache/` (both gitignored):

```powershell
./clean-isos.ps1 -WhatIf   # preview what would be removed
./clean-isos.ps1           # reclaim the space
```

WSL/bash equivalent:

```bash
rm -rf ISOs/* packer_cache/*
```

### Fallback to GitHub-hosted

Remove the `RUNNER_LINUX` and `RUNNER_WINDOWS` repository variables. Workflows automatically fall back to `ubuntu-latest` and `windows-latest`.

## File Structure

```
cicd/packer/
  linux-runner.pkr.hcl              # Packer template — Ubuntu 25.04
  windows-runner.pkr.hcl            # Packer template — Windows Server 2022
  variables.pkrvars.hcl.example     # vSphere variables (copy and fill in)
  secrets.auto.pkrvars.hcl.example  # Passwords (copy and fill in)
  repack-iso.sh                     # Download + repack the Ubuntu source ISO (autoinstall)
  clean-isos.ps1                    # Delete ISOs/ + packer_cache/ to reclaim disk
  http/
    linux/
      meta-data                     # cloud-init metadata
      user-data                     # cloud-init autoinstall config
    windows/
      autounattend.xml              # Windows unattended install
      install-vmtools.ps1           # VMware Tools silent installer
  scripts/
    linux/                          # Linux provisioning scripts (01-10)
    windows/                        # Windows provisioning scripts (01-06)
  ISOs/                             # Downloaded/repacked ISOs (gitignored; cleared by clean-isos.ps1)
  packer_cache/                     # Packer download cache (gitignored; cleared by clean-isos.ps1)
```
