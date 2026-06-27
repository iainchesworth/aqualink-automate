packer {
  required_plugins {
    vsphere = {
      version = "~> 1"
      source  = "github.com/hashicorp/vsphere"
    }
  }
}

variable "vcenter_server" {
  type = string
}

variable "vcenter_username" {
  type = string
}

variable "vcenter_password" {
  type      = string
  sensitive = true
}

variable "vcenter_insecure" {
  type    = bool
  default = true
}

variable "vcenter_datacenter" {
  type = string
}

variable "vcenter_cluster" {
  type = string
}

variable "vcenter_datastore" {
  type = string
}

variable "vcenter_network" {
  type = string
}

variable "vcenter_folder" {
  type    = string
  default = "Templates"
}

variable "iso_url" {
  type        = string
  description = <<-EOT
    Path to the autoinstall Ubuntu ISO that Packer boots. Produced by repack-iso.sh,
    which downloads the upstream Ubuntu source on demand (and caches it under ISOs/)
    rather than keeping it in the repo. Run ./repack-iso.sh before building.
  EOT
  default     = "ISOs/ubuntu-25.04-autoinstall.iso"
}

variable "iso_checksum" {
  # Locally generated artifact (repack-iso.sh output) — nothing to verify against.
  type    = string
  default = "none"
}

variable "vm_name" {
  type    = string
  default = "tpl-github-runner-linux"
}

variable "github_runner_version" {
  type    = string
  default = "2.331.0"
}

variable "ssh_password" {
  type      = string
  sensitive = true
}

source "vsphere-iso" "ubuntu" {
  # vSphere connection
  vcenter_server      = var.vcenter_server
  username            = var.vcenter_username
  password            = var.vcenter_password
  insecure_connection = var.vcenter_insecure
  datacenter          = var.vcenter_datacenter
  cluster             = var.vcenter_cluster
  datastore           = var.vcenter_datastore
  folder              = var.vcenter_folder

  # VM settings
  vm_name              = var.vm_name
  guest_os_type        = "ubuntu64Guest"
  firmware             = "efi"
  CPUs                 = 32
  # 48 GB: 24 GB OOM-killed cc1plus during parallel C++ builds (heavy TUs +
  # LTO at -j32 exceed ~0.75 GB/core, silently dropping the runner offline).
  # The host has ample free RAM, so size for comfortable headroom at full -j32.
  RAM                  = 49152
  RAM_reserve_all      = false
  # Allow downtime-free CPU/RAM resizes on the running VM (no power-cycle).
  CPU_hot_plug         = true
  RAM_hot_plug         = true
  disk_controller_type = ["pvscsi"]

  # Two thin disks so build junk can never fill the OS disk (the historic
  # datastore-exhaustion cause). disk0 = OS + toolchains only; disk1 = the data
  # volume that holds the runner work dir (wiped every job) AND the persistent
  # vcpkg/ccache caches. 00-data-volume.sh formats + mounts disk1 at /data, and
  # the autoinstall (http/linux/user-data) pins the OS install to the *smallest*
  # disk so it never lands on the data disk.
  #
  # disk0 — OS + toolchains (GCC/Clang/Docker/CMake ~10 GB used; 12 GB leaves headroom).
  storage {
    disk_size             = 12288
    disk_thin_provisioned = true
  }
  # disk1 — data: runner work (ephemeral) + vcpkg/ccache (persistent, capped) +
  # Docker's data-root (release builds pull gcc:15-bookworm/debian:bookworm here,
  # off the OS disk; see 05-docker.sh). 18 GB keeps the VM at a ~30 GB total: the
  # supervisor caps each cache at 3 GB and prunes downloads, leaving ~8-9 GB for the
  # work tree + transient Docker images. Tight for release builds — bump this one
  # line if a from-scratch all-deps build ever runs out of room.
  storage {
    disk_size             = 18432
    disk_thin_provisioned = true
  }

  network_adapters {
    network      = var.vcenter_network
    network_card = "vmxnet3"
  }

  # Use repacked ISO with autoinstall baked in (see repack-iso.sh)
  iso_url      = var.iso_url
  iso_checksum = var.iso_checksum

  # No boot_command needed — GRUB auto-boots with autoinstall parameter
  boot_wait    = "1s"
  boot_command = []

  # SSH communicator
  ssh_username         = "runner"
  ssh_password         = var.ssh_password
  ssh_timeout          = "30m"
  ssh_handshake_attempts = 100

  # Convert to template
  convert_to_template = true

  # VM tools
  tools_upgrade_policy = true

  notes = "GitHub Actions self-hosted runner - Ubuntu 25.04. Built by Packer."
}

build {
  sources = ["source.vsphere-iso.ubuntu"]

  provisioner "shell" {
    scripts = [
      "${path.root}/scripts/linux/00-data-volume.sh",
      "${path.root}/scripts/linux/01-base-packages.sh",
      "${path.root}/scripts/linux/02-gcc-toolchain.sh",
      "${path.root}/scripts/linux/03-llvm-toolchain.sh",
      "${path.root}/scripts/linux/04-cmake-ninja.sh",
      "${path.root}/scripts/linux/05-docker.sh",
      "${path.root}/scripts/linux/06-dev-tools.sh",
      "${path.root}/scripts/linux/07-ccache.sh",
      "${path.root}/scripts/linux/08-github-runner.sh",
      "${path.root}/scripts/linux/09-sonarcloud-tools.sh",
      "${path.root}/scripts/linux/10-cleanup.sh",
    ]
    environment_vars = [
      "DEBIAN_FRONTEND=noninteractive",
      "GITHUB_RUNNER_VERSION=${var.github_runner_version}",
    ]
    execute_command = "echo 'packer' | sudo -S env {{ .Vars }} bash '{{ .Path }}'"
  }
}
