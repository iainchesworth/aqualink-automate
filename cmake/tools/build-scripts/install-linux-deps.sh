#!/bin/bash

#
# Configure additional external repositories for CMake, gcc, LLVM
#

wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null
wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc > /dev/null
sudo add-apt-repository -y 'deb http://apt.llvm.org/jammy/  llvm-toolchain-jammy-16 main'
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

#
# Update the apt cache given the above new repositories
#

sudo apt update

#
# Install any/all packages required to build under Linux
#

sudo apt install -y build-essential ca-certificates cmake curl git gpg linux-libc-dev ninja-build pkg-config wget unzip zip
sudo apt install -y cmake 
sudo apt install -y clang-16 clang-tools-16 clang-16-doc libclang-common-16-dev libclang-16-dev libclang1-16 clang-format-16 python3-clang-16 clangd-16 clang-tidy-16 lld-16 libc++-16-dev libc++abi-16-dev clang-tidy-16
sudo apt install -y gcc-13 g++-13

#
# Bootstrap vcpkg (as required)
#

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
${SCRIPT_DIR}/../../deps/vcpkg/bootstrap-vcpkg.sh
