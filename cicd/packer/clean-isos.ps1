#requires -Version 5.1
<#
.SYNOPSIS
    Delete the on-demand Packer ISO downloads and Packer's download cache to
    reclaim disk space when not building runner templates.

.DESCRIPTION
    Empties cicd/packer/ISOs/ (the downloaded + repacked ISOs) and
    cicd/packer/packer_cache/ (Packer's own download cache). Both are gitignored
    and are fully regenerated on the next build:
      - repack-iso.sh re-downloads + repacks the Ubuntu source.
      - Packer re-downloads the Windows eval ISO from its default iso_url.

    The directories themselves are kept; only their contents are removed.

.PARAMETER WhatIf
    Show what would be removed (and the space it would reclaim) without deleting.

.EXAMPLE
    ./clean-isos.ps1 -WhatIf   # dry run
    ./clean-isos.ps1           # reclaim the disk
#>
[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$targets = @(
    (Join-Path $root 'ISOs'),
    (Join-Path $root 'packer_cache')
)

$totalBytes = 0
foreach ($dir in $targets) {
    if (-not (Test-Path -LiteralPath $dir)) {
        Write-Host "skip  (absent)       $dir"
        continue
    }

    $size = (Get-ChildItem -LiteralPath $dir -Force -Recurse -File -ErrorAction SilentlyContinue |
        Measure-Object -Property Length -Sum).Sum
    if (-not $size) { $size = 0 }
    $totalBytes += $size
    Write-Host ("clean ({0,7:N2} GB)   {1}" -f ($size / 1GB), $dir)

    # Remove the directory's contents, keeping the (gitignored) dir itself.
    Get-ChildItem -LiteralPath $dir -Force -ErrorAction SilentlyContinue | ForEach-Object {
        if ($PSCmdlet.ShouldProcess($_.FullName, 'Remove')) {
            Remove-Item -LiteralPath $_.FullName -Recurse -Force
        }
    }
}

$totalGb = [math]::Round($totalBytes / 1GB, 2)
if ($WhatIfPreference) {
    Write-Host ("`nWould reclaim ~{0:N2} GB." -f $totalGb)
} else {
    Write-Host ("`nReclaimed ~{0:N2} GB." -f $totalGb)
}
