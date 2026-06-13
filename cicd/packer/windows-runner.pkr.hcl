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
  description = "Local path or URL to Windows Server 2022 ISO"
}

variable "iso_checksum" {
  type    = string
  default = "none"
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
  disk_controller_type = ["lsilogic-sas"]

  storage {
    disk_size             = 153600
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
      "${path.root}/scripts/windows/01-base-config.ps1",
      "${path.root}/scripts/windows/02-vs-buildtools.ps1",
      "${path.root}/scripts/windows/03-cmake-ninja.ps1",
      "${path.root}/scripts/windows/04-choco-packages.ps1",
      "${path.root}/scripts/windows/05-github-runner.ps1",
      "${path.root}/scripts/windows/06-cleanup.ps1",
    ]
    environment_vars = [
      "GITHUB_RUNNER_VERSION=${var.github_runner_version}",
    ]
  }
}
