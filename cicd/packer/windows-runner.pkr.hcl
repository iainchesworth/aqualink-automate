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
    URL (or local path) to the Windows Server 2022 evaluation ISO. Defaults to the
    Microsoft Evaluation Center download — Packer fetches it into packer_cache/ on
    build, so no ISO is stored in the repo. If Microsoft refreshes the eval build the
    checksum below will mismatch; update iso_url and iso_checksum together.
  EOT
  default     = "https://software-static.download.prss.microsoft.com/sg/download/888969d5-f34g-4e03-ac9d-1f9786c66749/SERVER_EVAL_x64FRE_en-us.iso"
}

variable "iso_checksum" {
  type    = string
  default = "sha256:3e4fa6d8507b554856fc9ca6079cc402df11a8b79344871669f0251535255325"
}

variable "vm_name" {
  type    = string
  default = "tpl-github-runner-windows"
}

variable "github_runner_version" {
  type    = string
  default = "2.321.0"
}

variable "winrm_username" {
  type    = string
  default = "runner"
}

variable "winrm_password" {
  type      = string
  sensitive = true
}

source "vsphere-iso" "windows" {
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
  guest_os_type        = "windows2019srvNext_64Guest"
  CPUs                 = 32
  # 48 GB: match the Linux runner (24 GB OOM-killed cc1plus during parallel
  # C++ builds at -j32; host has ample free RAM, so size for headroom).
  RAM                  = 49152
  RAM_reserve_all      = false
  # Allow downtime-free CPU/RAM resizes on the running VM (no power-cycle).
  CPU_hot_plug         = true
  RAM_hot_plug         = true
  disk_controller_type = ["lsilogic-sas"]

  # Two thin disks so build junk can never fill the OS disk (the historic
  # datastore-exhaustion cause). disk0 = OS + toolchains only; disk1 = the data
  # volume (drive D:) that holds the runner work dir (wiped every job) AND the
  # persistent vcpkg/ccache caches. 00-data-volume.ps1 formats + mounts disk1 as
  # D:. autounattend.xml installs Windows to disk0 only.
  #
  # disk0 — OS + toolchains. 30 GB: Windows Server + VS 2022 Build Tools alone is
  # ~20 GB, so the OS disk can't be as small as Linux's.
  storage {
    disk_size             = 30720
    disk_thin_provisioned = true
  }
  # disk1 — data: runner work (ephemeral) + vcpkg/ccache (persistent, capped). 16 GB
  # (no Docker / no second glibc cache on Windows, unlike Linux). The OS disk can't
  # shrink below ~30 GB because VS 2022 Build Tools alone is ~20 GB, so the Windows
  # VM floors at ~46 GB total — it can't hit the 30 GB Linux target without slimming
  # the toolchain.
  storage {
    disk_size             = 16384
    disk_thin_provisioned = true
  }

  network_adapters {
    network      = var.vcenter_network
    network_card = "vmxnet3"
  }

  iso_url      = var.iso_url
  iso_checksum = var.iso_checksum

  # Mount VMware Tools ISO from ESXi host (needed for guest IP detection + vmxnet3 driver)
  iso_paths = ["[] /vmimages/tools-isoimages/windows.iso"]

  # Boot settings
  boot_wait    = "3s"
  boot_command = []

  # Floppy with autounattend.xml
  floppy_files = [
    "${path.root}/http/windows/autounattend.xml",
    "${path.root}/http/windows/install-vmtools.ps1",
  ]

  # WinRM communicator
  communicator   = "winrm"
  winrm_username = var.winrm_username
  winrm_password = var.winrm_password
  winrm_timeout  = "60m"

  # Convert to template
  convert_to_template = true

  # VM tools
  tools_upgrade_policy = true

  notes = "GitHub Actions self-hosted runner - Windows Server 2022. Built by Packer."
}

build {
  sources = ["source.vsphere-iso.windows"]

  provisioner "powershell" {
    scripts = [
      "${path.root}/scripts/windows/00-data-volume.ps1",
      "${path.root}/scripts/windows/01-base-config.ps1",
      "${path.root}/scripts/windows/02-vs-buildtools.ps1",
      "${path.root}/scripts/windows/03-cmake-ninja.ps1",
      "${path.root}/scripts/windows/04-choco-packages.ps1",
      "${path.root}/scripts/windows/05-github-runner.ps1",
      "${path.root}/scripts/windows/06-cleanup.ps1",
    ]
    environment_vars = [
      "GITHUB_RUNNER_VERSION=${var.github_runner_version}",
      # The ephemeral supervisor runs as the `runner` account via a scheduled
      # task set to run "whether logged on or not", which needs the account
      # password. Reuse the same secret the autounattend already sets.
      "RUNNER_PASSWORD=${var.winrm_password}",
    ]
  }
}
