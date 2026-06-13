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
  description = "Local path or URL to Ubuntu 25.04 Server ISO"
}

variable "iso_checksum" {
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
  disk_controller_type = ["pvscsi"]

  storage {
    disk_size             = 153600
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
