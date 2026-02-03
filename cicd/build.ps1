########################################################################################
#
# Windows build helper for aqualink-automate.
#
# Usage:
#   .\cicd\build.ps1                                         # default: MSVC debug
#   .\cicd\build.ps1 -Compiler clang -Type release
#   .\cicd\build.ps1 -Preset config-windows-msvc-debug
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

# Setup MSVC environment if needed
if ($CompilerName -eq "msvc") {
    if (-not $env:VSCMD_VER) {
        Write-Host "--- Setting up MSVC environment ---"
        $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $VsWhere) {
            $VsInstall = & $VsWhere -latest -property installationPath
            $VcVarsAll = Join-Path $VsInstall "VC\Auxiliary\Build\vcvarsall.bat"
            if (Test-Path $VcVarsAll) {
                cmd.exe /c "call `"$VcVarsAll`" amd64 && set > %temp%\vcvars.txt"
                Get-Content "$env:temp\vcvars.txt" | ForEach-Object {
                    if ($_ -match "^(.*?)=(.*)$") {
                        Set-Content "env:\$($matches[1])" $matches[2]
                    }
                }
            } else {
                Write-Host "Warning: vcvarsall.bat not found. Build may fail." -ForegroundColor Yellow
            }
        } else {
            Write-Host "Warning: vswhere.exe not found. Build may fail." -ForegroundColor Yellow
        }
    }
}

# Bootstrap vcpkg if needed
$VcpkgDir = Join-Path $ProjectDir "deps\vcpkg"
$VcpkgExe = Join-Path $VcpkgDir "vcpkg.exe"
if (-not (Test-Path $VcpkgExe)) {
    Write-Host "--- Bootstrapping vcpkg ---"
    $BootstrapScript = Join-Path $VcpkgDir "bootstrap-vcpkg.bat"
    if (-not (Test-Path $BootstrapScript)) {
        Write-Host "Error: vcpkg not found at $VcpkgDir. Did you clone with --recurse-submodules?" -ForegroundColor Red
        exit 1
    }
    & $BootstrapScript -disableMetrics
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
