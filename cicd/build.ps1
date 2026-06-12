########################################################################################
#
# Windows build helper for aqualink-automate.
#
# Usage:
#   .\cicd\build.ps1                                         # default: MSVC debug
#   .\cicd\build.ps1 -Compiler clang -Type release
#   .\cicd\build.ps1 -Preset config-windows-msvc-debug
#   .\cicd\build.ps1 -Package
#
########################################################################################

param(
    [string]$Preset = "",
    [ValidateSet("msvc", "clang", "llvm", "")]
    [string]$Compiler = "",
    [ValidateSet("debug", "release", "")]
    [string]$Type = "debug",
    [switch]$Package
)

$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir

# Default compiler
if (-not $Compiler) {
    $Compiler = "msvc"
}

# Normalize compiler name
$CompilerName = switch ($Compiler) {
    "msvc"  { "msvc" }
    "clang" { "llvm" }
    "llvm"  { "llvm" }
    default {
        Write-Host "Error: Unknown compiler '$Compiler'. Use msvc, clang, or llvm." -ForegroundColor Red
        exit 1
    }
}

# Build preset name if not explicitly provided
if (-not $Preset) {
    $Preset = switch ($Type) {
        "debug"   { "config-windows-$CompilerName-debug" }
        "release" { "config-windows-$CompilerName" }
        default {
            Write-Host "Error: Unknown build type '$Type'. Use debug or release." -ForegroundColor Red
            exit 1
        }
    }
}

# Derive build and test preset names
$BuildPreset = $Preset -replace '^config-', 'build-'
$TestPreset = $Preset -replace '^config-', 'test-'

Write-Host "=== aqualink-automate build ===" -ForegroundColor Cyan
Write-Host "  Platform:  windows"
Write-Host "  Compiler:  $CompilerName"
Write-Host "  Type:      $Type"
Write-Host "  Preset:    $Preset"
Write-Host ""

# ---- Dependency validation ----
Write-Host "--- Checking dependencies ---"
$Missing = @()

if (-not (Get-Command git -ErrorAction SilentlyContinue)) { $Missing += "git (https://git-scm.com)" }
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { $Missing += "cmake (https://cmake.org)" }
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { $Missing += "ninja (https://ninja-build.org)" }

if ($CompilerName -eq "msvc") {
    $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $VsWhere)) {
        $Missing += "Visual Studio Build Tools (https://visualstudio.microsoft.com/downloads/)"
    }
} elseif ($CompilerName -eq "llvm") {
    if (-not (Get-Command clang -ErrorAction SilentlyContinue)) { $Missing += "clang (https://releases.llvm.org)" }
    if (-not (Get-Command clang++ -ErrorAction SilentlyContinue)) { $Missing += "clang++ (https://releases.llvm.org)" }
}

if ($Missing.Count -gt 0) {
    Write-Host "Error: Missing required tools:" -ForegroundColor Red
    foreach ($tool in $Missing) {
        Write-Host "  - $tool" -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "Install with: choco install git cmake ninja visualstudio2022buildtools" -ForegroundColor Yellow
    exit 1
}

# Report versions
Write-Host "  git:    $(git --version)"
Write-Host "  cmake:  $(cmake --version | Select-Object -First 1)"
Write-Host "  ninja:  $(ninja --version)"
if ($CompilerName -eq "msvc") {
    $VsInstall = & $VsWhere -latest -property installationPath
    $VsVersion = & $VsWhere -latest -property catalog_productDisplayVersion
    Write-Host "  msvc:   Visual Studio $VsVersion ($VsInstall)"
} elseif ($CompilerName -eq "llvm") {
    Write-Host "  clang:  $(clang --version | Select-Object -First 1)"
}
Write-Host ""

# ---- Setup MSVC environment ----
if ($CompilerName -eq "msvc") {
    if (-not $env:VSCMD_VER) {
        Write-Host "--- Setting up MSVC environment ---"
        $VcVarsAll = Join-Path $VsInstall "VC\Auxiliary\Build\vcvarsall.bat"
        if (Test-Path $VcVarsAll) {
            cmd.exe /c "call `"$VcVarsAll`" amd64 && set > %temp%\vcvars.txt"
            Get-Content "$env:temp\vcvars.txt" | ForEach-Object {
                if ($_ -match "^(.*?)=(.*)$") {
                    Set-Content "env:\$($matches[1])" $matches[2]
                }
            }
        } else {
            Write-Host "Error: vcvarsall.bat not found at $VcVarsAll" -ForegroundColor Red
            exit 1
        }
    }
}

# ---- Submodules ----
$VcpkgBootstrap = Join-Path $ProjectDir "deps\vcpkg\bootstrap-vcpkg.bat"
if (-not (Test-Path $VcpkgBootstrap)) {
    Write-Host "--- Initializing submodules ---"
    git -C "$ProjectDir" submodule update --init --recursive
}

# ---- Bootstrap vcpkg ----
$VcpkgDir = Join-Path $ProjectDir "deps\vcpkg"
$VcpkgExe = Join-Path $VcpkgDir "vcpkg.exe"
if (-not (Test-Path $VcpkgExe)) {
    Write-Host "--- Bootstrapping vcpkg ---"
    & $VcpkgBootstrap -disableMetrics
}

# Configure
Write-Host "--- Configure ---"
cmake --preset="$Preset" "$ProjectDir"

# Build
Write-Host "--- Build ---"
cmake --build --preset="$BuildPreset"

# Test
Write-Host "--- Test ---"
ctest --preset="$TestPreset"

# Package (optional)
if ($Package) {
    Write-Host "--- Package ---"
    cmake --build "build/$Preset" --target pack-aqualink-automate
}

Write-Host ""
Write-Host "=== Build complete ===" -ForegroundColor Green
