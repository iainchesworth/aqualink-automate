########################################################################################
#
# Environment variables....
#
#    # SONAR_TOKEN -> the API key that enables access to the project
#    # SONAR_URL   -> sets the URL for the SonarQube server
#    # SONAR_DEBUG -> adds the "--debug" flag for debug output during processing
#
# Use the following to set an environment variable in the console:
#
#    PS C:\Repos\my-project>$env:SONAR_TOKEN = 'mytokenvaluegoeshere'
#
# Run this script using the following:
#
#    PS C:\Repos\my-project>cicd\windows-build-with-coverage-sonarcloud.ps1
#
########################################################################################

# Set error action preference to stop execution on error
$ErrorActionPreference = 'Stop'

# Helper function to write error messages and exit
Function Write-ErrorAndExit {
    Param ([string]$Message)
    Write-Host "Error: $Message" -ForegroundColor Red
    exit 1
}

# Accept preset name as the first argument or set to default
$CMAKE_PRESET = If ($args[0]) { $args[0] } else { "config-windows-msvc" }

# Derive the build preset from the configure preset
$CMAKE_BUILD_PRESET = $CMAKE_PRESET -replace '^config-', 'build-'

# Validate SONAR_TOKEN environment variable
if (-Not $env:SONAR_TOKEN) {
    Write-ErrorAndExit "SONAR_TOKEN environment variable is not set."
}

# Set SONAR_URL to default if not provided
$SONAR_URL = If ($env:SONAR_URL) { $env:SONAR_URL } else {"https://sonarcloud.io"}

# Remove and create .sonar directory
$sonarHome = "$HOME/.sonar"
Remove-Item $sonarHome -Recurse -Force -ErrorAction SilentlyContinue
New-Item -Path $sonarHome -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

$SONAR_SCANNER_VERSION = "5.0.1.3006"
$SONAR_SCANNER_HOME = "$sonarHome/sonar-scanner-$SONAR_SCANNER_VERSION-windows"

# Function to download and extract files
Function DownloadAndExtract {
    Param ([string]$url, [string]$destinationPath)
    Remove-Item $destinationPath -Recurse -Force -ErrorAction SilentlyContinue
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $webClient = New-Object System.Net.WebClient
    $webClient.DownloadFile($url, $destinationPath)
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($destinationPath, $sonarHome)
}

# Download and set up build-wrapper and sonar-scanner
DownloadAndExtract "$SONAR_URL/static/cpp/build-wrapper-win-x86.zip" "$sonarHome/build-wrapper-win-x86.zip"
$env:Path += ";$sonarHome/build-wrapper-win-x86"
DownloadAndExtract "https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-windows.zip" "$sonarHome/sonar-scanner-cli-$SONAR_SCANNER_VERSION-windows.zip"
$env:Path += ";$SONAR_SCANNER_HOME\bin"

# Helper function to determine if the system is 64-bit
Function Is64BitSystem {
    return [Environment]::Is64BitOperatingSystem
}

# Helper function to check if vcvars has been set in the current session
Function IsVcvarsSet {
    return -not [string]::IsNullOrWhiteSpace($env:VSCMD_VER)
}

# Function to setup build environment
Function SetupBuildEnvironment {
    $VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-Not (Test-Path $VsWhere)) {
        Write-ErrorAndExit "vswhere.exe not found. Ensure Visual Studio is installed."
    }

    $VsInstall = & $VsWhere -latest -property installationPath
    $vcvarsFile = If (Is64BitSystem) { "vcvars64.bat" } else { "vcvars32.bat" }
    $fullPath = Join-Path $VsInstall "VC\Auxiliary\Build\$vcvarsFile"

    if (-Not (Test-Path $fullPath)) {
        Write-ErrorAndExit "The vcvars file ($vcvarsFile) does not exist at the expected location."
    }

    if (-Not (IsVcvarsSet)) {
        try {
            cmd.exe /c "call `"$fullPath`" && set > %temp%\vcvars.txt"
            Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
                if ($_ -match "^(.*?)=(.*)$") {
                    Set-Content "env:\$($matches[1])" $matches[2]
                }
            }
        } catch {
            Write-ErrorAndExit "Failed to execute vcvars file ($vcvarsFile). Error: $_"
        }
    }
}

# Call the function to setup build environment
SetupBuildEnvironment

# Build configuration and execution
cmake --fresh --preset=$CMAKE_PRESET .
build-wrapper-win-x86-64 --out-dir build_wrapper_output_directory cmake --build --preset=$CMAKE_BUILD_PRESET

# Prepare and run sonar scanner
$SONAR_DEBUG_CMD_ARG = If ($env:SONAR_DEBUG) { "--debug" } else { "" }
$SONAR_URL_CMD_ARG = If ($SONAR_URL) { "-Dsonar.host.url=$SONAR_URL" }
$SONAR_TOKEN_CMD_ARG = If ($env:SONAR_TOKEN) { "-Dsonar.login=$env:SONAR_TOKEN" }
$SONAR_OTHER_ARGS = @("-Dsonar.sources=src", "-Dsonar.tests=test", "-Dsonar.cfamily.build-wrapper-output=build_wrapper_output_directory", "-Dsonar.sourceEncoding=UTF-8")
sonar-scanner.bat $SONAR_DEBUG_CMD_ARG $SONAR_URL_CMD_ARG $SONAR_TOKEN_CMD_ARG $SONAR_OTHER_ARGS
