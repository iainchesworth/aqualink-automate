# Self-Hosted GitHub Actions Runners

Packer templates for building self-hosted GitHub Actions runner VMs on VMware vSphere (ESXi). These runners provide persistent build caches, faster CI times, and full control over the build environment.

## Architecture

| Runner | Base OS | CPUs | RAM | Disk | Pre-installed toolchain |
|--------|---------|------|-----|------|------------------------|
| Linux | Ubuntu 24.04 LTS | 8 | 12 GB | 100 GB | GCC 14, Clang/LLVM 21, CMake, Ninja, Docker, ccache, SonarCloud build-wrapper |
| Windows | Windows Server 2022 | 8 | 12 GB | 150 GB | VS 2022 Build Tools (MSVC v143), CMake, Ninja, NSIS, ccache |

macOS CI stays on GitHub-hosted `macos-latest` runners.

## Prerequisites

- [Packer](https://www.packer.io/) 1.9+
- VMware vSphere / ESXi with vCenter access
- OS installation ISOs:
  - Ubuntu 24.04 LTS Server (repacked with autoinstall for unattended install)
  - Windows Server 2022

## Configuration

### 1. Copy the example files

```bash
cd cicd/packer
cp variables.pkrvars.hcl.example variables.pkrvars.hcl
cp secrets.auto.pkrvars.hcl.example secrets.auto.pkrvars.hcl
```

### 2. Edit `variables.pkrvars.hcl`

Fill in your vSphere connection details, ISO paths, and runner version:

```hcl
vcenter_server     = "vcenter.example.com"
vcenter_username   = "administrator@vsphere.local"
vcenter_datacenter = "Datacenter"
vcenter_cluster    = "Cluster"
vcenter_datastore  = "datastore1"
vcenter_network    = "VM Network"
vcenter_folder     = "Templates"

iso_url = "ISOs/ubuntu-24.04.4-autoinstall.iso"

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

Build the VM templates:

```bash
# Linux runner
packer build -force \
  -var-file=variables.pkrvars.hcl \
  -var-file=secrets.auto.pkrvars.hcl \
  -var iso_url="ISOs/ubuntu-24.04.4-autoinstall.iso" \
  linux-runner.pkr.hcl

# Windows runner
packer build -force \
  -var-file=variables.pkrvars.hcl \
  -var-file=secrets.auto.pkrvars.hcl \
  -var iso_url="ISOs/windows-server-2022.iso" \
  windows-runner.pkr.hcl
```

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
| `guestinfo.github.token` | Registration token from step 2 |
| `guestinfo.github.repository` | `https://github.com/{owner}/aqualink-automate` |
| `guestinfo.github.runner_name` | e.g. `linux-runner` or `windows-runner` |
| `guestinfo.github.runner_labels` | e.g. `self-hosted,linux,x64` or `self-hosted,windows,x64` |

Using `govc`:

```bash
govc vm.change -vm github-runner-linux \
  -e guestinfo.github.token="AXXXXXXXXXXXX" \
  -e guestinfo.github.repository="https://github.com/{owner}/aqualink-automate" \
  -e guestinfo.github.runner_name="linux-runner" \
  -e guestinfo.github.runner_labels="self-hosted,linux,x64"
```

### 4. Boot the VMs

On first boot, the auto-registration service reads the guestinfo variables and registers the runner with GitHub. A `.registered` marker file prevents re-registration on subsequent reboots.

- **Linux**: systemd service (`github-runner-register.service`) runs at boot
- **Windows**: Scheduled task (`GitHubRunnerAutoRegister`) runs at startup

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
| `01-base-packages.sh` | build-essential, ca-certificates, curl, git, gpg, jq, pkg-config, tar, unzip, wget, zip |
| `02-gcc-toolchain.sh` | gcc-15, g++-15, gcov-15, update-alternatives symlinks |
| `03-llvm-toolchain.sh` | clang-21, clang-tidy-21, lld-21, libc++-21-dev from apt.llvm.org |
| `04-cmake-ninja.sh` | CMake 3.31.6 (Kitware binary), ninja-build |
| `05-docker.sh` | Docker Engine CE + buildx plugin |
| `06-dev-tools.sh` | python3, gcovr, autoconf, automake, libtool |
| `07-ccache.sh` | ccache (5 GB max, compression enabled) |
| `08-github-runner.sh` | GitHub Actions runner agent + auto-registration service |
| `09-sonarcloud-tools.sh` | SonarCloud build-wrapper for Linux |
| `10-cleanup.sh` | apt clean, log truncation, free space zeroing |

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

When toolchain versions change, update the provisioning scripts and rebuild:

```bash
packer build -force -var-file=variables.pkrvars.hcl -var-file=secrets.auto.pkrvars.hcl linux-runner.pkr.hcl
```

Then deploy new VMs from the updated template and re-register.

### Fallback to GitHub-hosted

Remove the `RUNNER_LINUX` and `RUNNER_WINDOWS` repository variables. Workflows automatically fall back to `ubuntu-latest` and `windows-latest`.

## File Structure

```
cicd/packer/
  linux-runner.pkr.hcl              # Packer template — Ubuntu 24.04
  windows-runner.pkr.hcl            # Packer template — Windows Server 2022
  variables.pkrvars.hcl.example     # vSphere variables (copy and fill in)
  secrets.auto.pkrvars.hcl.example  # Passwords (copy and fill in)
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
  ISOs/                             # OS ISOs (gitignored)
  packer_cache/                     # Packer download cache (gitignored)
```
