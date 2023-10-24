#!/bin/bash

# Accept preset name as the first argument or set to default
$CMAKE_PRESET = If ($args[0]) { $args[0] } else { "ci-gcc-linux-x86_64" }

# Configuration for SonarCloud
#SONAR_TOKEN= # access token from SonarCloud project creation page -Dsonar.login=XXXX: here it is defined in the environment through the CI
if [ -z "$SONAR_TOKEN" ]; then
    echo "Error: SONAR_TOKEN environment variable is not set."
    exit 1
fi

# Set default to SONAR_HOST_URL in not provided
SONAR_HOST_URL=${SONAR_HOST_URL:-https://sonarcloud.io}

rm -rf $HOME/.sonar
mkdir $HOME/.sonar
export SONAR_SCANNER_VERSION=4.7.0.2747
export SONAR_SCANNER_HOME=$HOME/.sonar/sonar-scanner-$SONAR_SCANNER_VERSION-linux

# download sonar-scanner
curl -sSLo $HOME/.sonar/sonar-scanner.zip https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux.zip 
unzip -o $HOME/.sonar/sonar-scanner.zip -d $HOME/.sonar/
export PATH=$SONAR_SCANNER_HOME/bin:$PATH
export SONAR_SCANNER_OPTS="-server"

# download build-wrapper
curl -sSLo $HOME/.sonar/build-wrapper-linux-x86.zip https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip -o $HOME/.sonar/build-wrapper-linux-x86.zip -d $HOME/.sonar/
export PATH=$HOME/.sonar/build-wrapper-linux-x86:$PATH

# Setup the build system
rm -rf build
cmake --preset=$CMAKE_PRESET .

# Build inside the build-wrapper
build-wrapper-linux-x86-64 --out-dir build_wrapper_output_directory cmake --build . --preset=$CMAKE_PRESET

# Run sonar scanner (here, arguments are passed through the command line but most of them can be written in the sonar-project.properties file)
[[ -v SONAR_URL ]] && SONAR_URL_CMD_ARG="-Dsonar.host.url=${SONAR_URL}"
[[ -v SONAR_TOKEN ]] && SONAR_TOKEN_CMD_ARG="-Dsonar.login=${SONAR_TOKEN}"
SONAR_OTHER_ARGS="-X -Dsonar.sources=src,build/${CMAKE_PRESET}/vcpkg_installed/x64-linux/include/ -Dsonar.cfamily.build-wrapper-output=build_wrapper_output_directory -Dsonar.sourceEncoding=UTF-8"
sonar-scanner ${SONAR_URL_CMD_ARG} ${SONAR_TOKEN_CMD_ARG} ${SONAR_OTHER_ARGS}
