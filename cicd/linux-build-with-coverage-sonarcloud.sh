########################################################################################
#
# Environment variables....
#
#    # SONAR_TOKEN -> the API key that enables access to the project
#    # SONAR_URL   -> sets the URL for the SonarQube server 
#    # SONAR_DEBUG -> adds the "--debug" flag for debug output during processing
#
# Use the following to run this build script:
#
#    user@computer:~/repos/aqualink-automate$ SONAR_TOKEN=mytokenvaluegoeshere ./cicd/linux-build-with-coverage-sonarcloud.sh
#
########################################################################################

#!/bin/bash

# Accept preset name as the first argument or set to default
CMAKE_PRESET=${1:-"ci-ninja-gcc-linux-x86_64-coverage"}

# Configuration for SonarCloud
#SONAR_TOKEN= # access token from SonarCloud project creation page -Dsonar.login=XXXX: here it is defined in the environment through the CI
if [ -z "$SONAR_TOKEN" ]; then
    echo "Error: SONAR_TOKEN environment variable is not set."
    exit 1
fi

# Set default to SONAR_HOST_URL in not provided
SONAR_URL=${SONAR_HOST_URL:-https://sonarcloud.io}

rm -rf $HOME/.sonar
mkdir $HOME/.sonar
export SONAR_SCANNER_VERSION=5.0.1.3006
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
cmake --fresh -DUSE_SONARQUBE=ON --preset=$CMAKE_PRESET .

# Build inside the build-wrapper
build-wrapper-linux-x86-64 --out-dir build_wrapper_output_directory cmake --build . --preset=$CMAKE_PRESET

# Run sonar scanner (here, arguments are passed through the command line but most of them can be written in the sonar-project.properties file)
[[ -v SONAR_DEBUG ]] && SONAR_DEBUG_CMD_ARG="--debug"
[[ -v SONAR_URL ]] && SONAR_URL_CMD_ARG="-Dsonar.host.url=${SONAR_URL}"
[[ -v SONAR_TOKEN ]] && SONAR_TOKEN_CMD_ARG="-Dsonar.login=${SONAR_TOKEN}"
SONAR_OTHER_ARGS="-Dsonar.sources=src -Dsonar.tests=test -Dsonar.cfamily.build-wrapper-output=build_wrapper_output_directory -Dsonar.sourceEncoding=UTF-8"
SONAR_GCOVR_COVERAGE="-Dsonar.coverageReportPaths=./build/${CMAKE_PRESET}/coverage-testaqualink-automate.xml"
sonar-scanner ${SONAR_DEBUG_CMD_ARG} ${SONAR_URL_CMD_ARG} ${SONAR_TOKEN_CMD_ARG} ${SONAR_OTHER_ARGS} ${SONAR_COVERAGE_REPORT}
