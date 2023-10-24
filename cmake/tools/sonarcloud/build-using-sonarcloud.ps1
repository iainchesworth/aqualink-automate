$ErrorActionPreference = 'Stop'

# Accept preset name as the first argument or set to default
$CMAKE_PRESET = If ($args[0]) { $args[0] } else { "ci-msvc-windows-x86_64" }

# Configuration for SonarCloud
#$SONAR_TOKEN = "" # access token from SonarCloud projet creation page -Dsonar.login=XXXX: set in the environment from the CI
if (-Not $env:SONAR_TOKEN) {
    Write-Host "Error: SONAR_TOKEN environment variable is not set."
    exit 1
}

# Set default to SONAR_URL in not provided
$SONAR_URL = If ( $SONAR_URL ) { $SONAR_URL } else {"https://sonarcloud.io"}

rm $HOME/.sonar -Recurse -Force -ErrorAction SilentlyContinue
mkdir $HOME/.sonar -ErrorAction SilentlyContinue

$SONAR_SCANNER_VERSION = "4.7.0.2747"
$SONAR_SCANNER_HOME = "$HOME/.sonar/sonar-scanner-$SONAR_SCANNER_VERSION-windows"

# Download build-wrapper
$path = "$HOME/.sonar/build-wrapper-win-x86.zip"
rm build-wrapper-win-x86 -Recurse -Force -ErrorAction SilentlyContinue
rm $path -Force -ErrorAction SilentlyContinue
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
(New-Object System.Net.WebClient).DownloadFile("$SONAR_URL/static/cpp/build-wrapper-win-x86.zip", $path)
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($path, "$HOME/.sonar")
$env:Path += ";$HOME/.sonar/build-wrapper-win-x86"

# Download sonar-scanner
$path = "$HOME/.sonar/sonar-scanner-cli-4.2.0.1873-windows.zip"
rm sonar-scanner -Recurse -Force -ErrorAction SilentlyContinue
rm $path -Force -ErrorAction SilentlyContinue
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
(New-Object System.Net.WebClient).DownloadFile("https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-windows.zip", $path)
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($path, "$HOME/.sonar")
$env:Path += ";$SONAR_SCANNER_HOME\bin"

# Set up the build system
cmd.exe /c "call `"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat`" && set > %temp%\vcvars.txt"
Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
    if ($_ -match "^(.*?)=(.*)$") {
        Set-Content "env:\$($matches[1])" $matches[2]
    }
}  

# Remove existing build directory, configure and build using the CMake preset
rm build -Recurse -Force -ErrorAction SilentlyContinue
cmake --preset=$CMAKE_PRESET .

# Build inside the build-wrapper
build-wrapper-win-x86-64 --out-dir build_wrapper_output_directory cmake --build . --preset=$CMAKE_PRESET

# Run sonar scanner (here, arguments are passed through the command line but most of them can be written in the sonar-project.properties file)
$SONAR_URL_CMD_ARG = If ( $SONAR_URL ) { "-D sonar.host.url=$SONAR_URL" }
$SONAR_TOKEN_CMD_ARG = If ( $SONAR_TOKEN ) { "-D sonar.login=$SONAR_TOKEN" }
$SONAR_OTHER_ARGS = @("-X -D sonar.sources=src,build/$CMAKE_PRESET/vcpkg_installed/x64-windows/include/","-D sonar.cfamily.build-wrapper-output=build_wrapper_output_directory","-D sonar.sourceEncoding=UTF-8")
sonar-scanner.bat $SONAR_URL_CMD_ARG $SONAR_TOKEN_CMD_ARG $SONAR_OTHER_ARGS
