#Requires -RunAsAdministrator
$ErrorActionPreference = "Stop"

Write-Host "==> Installing Visual Studio 2022 Build Tools"

$installerUrl = "https://aka.ms/vs/17/release/vs_buildtools.exe"
$installerPath = "$env:TEMP\vs_buildtools.exe"

Invoke-WebRequest -Uri $installerUrl -OutFile $installerPath -UseBasicParsing

# Install MSVC v143 toolset, Windows SDK, MSBuild, and ASan
$args = @(
    "--quiet",
    "--wait",
    "--norestart",
    "--nocache",
    "--add", "Microsoft.VisualStudio.Workload.VCTools",
    "--add", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
    "--add", "Microsoft.VisualStudio.Component.Windows11SDK.22621",
    "--add", "Microsoft.Component.MSBuild",
    "--add", "Microsoft.VisualStudio.Component.VC.ASAN",
    "--includeRecommended"
)

Write-Host "==> Running VS Build Tools installer (this may take a while)..."
$process = Start-Process -FilePath $installerPath -ArgumentList $args -Wait -PassThru -NoNewWindow
if ($process.ExitCode -ne 0 -and $process.ExitCode -ne 3010) {
    throw "VS Build Tools installation failed with exit code $($process.ExitCode)"
}

Remove-Item $installerPath -Force -ErrorAction SilentlyContinue

Write-Host "==> Visual Studio 2022 Build Tools installed"
