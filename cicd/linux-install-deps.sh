#!/bin/bash

#
# Configure additional external repositories for CMake, gcc, LLVM
#

wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc > /dev/null
sudo add-apt-repository -y 'deb http://apt.llvm.org/jammy/  llvm-toolchain-jammy-17 main'
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

#
# Update the apt cache given the above new repositories
#

sudo apt update

#
# Install any/all packages required to build under Linux
#

sudo apt install -y build-essential ca-certificates cmake curl git gpg linux-libc-dev pkg-config rpm rsync wget unzip zip
sudo apt install -y cmake 
sudo apt install -y clang-17 clang-tools-17 clang-17-doc libclang-common-17-dev libclang-17-dev libclang1-17 clang-format-17 python3-clang-17 clangd-17 clang-tidy-17 lld-17 libc++-17-dev libc++abi-17-dev clang-tidy-17
sudo apt install -y gcc-13 g++-13

#
# Install the latest ninja-build version from GitHub
#
curl -L "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip" > /tmp/ninja.zip
sudo unzip /tmp/ninja.zip -d /usr/local/bin


#
# Install gcovr
#
sudo apt install -y python3-pip
sudo pip3 install gcovr

#
# Configure the installed gcc versions to default to gcc v13
#
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 11
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 11
sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-11 11
sudo update-alternatives --install /usr/bin/gcov-dump gcov-dump /usr/bin/gcov-dump-11 11
sudo update-alternatives --install /usr/bin/gcov-tool gcov-tool /usr/bin/gcov-tool-11 11

sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 13
sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-13 13
sudo update-alternatives --install /usr/bin/gcov-dump gcov-dump /usr/bin/gcov-dump-13 13
sudo update-alternatives --install /usr/bin/gcov-tool gcov-tool /usr/bin/gcov-tool-13 13

#
# Bootstrap vcpkg (as required)
#

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
/bin/bash ${SCRIPT_DIR}/../../../deps/vcpkg/bootstrap-vcpkg.sh
