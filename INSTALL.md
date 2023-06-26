# Installation

Here are the comprehensive instructions detailing how to installing pre-built binaries or build Aqualink Automate directly from source.

## Download and Installation

Aqualink Automate currently provides [pre-built binaries](https://github.com/iainchesworth/aqualink-automate/releases) for the following platforms:

- Windows
- Linux
- MacOS

To use one of these binaries, follow the steps below for your platform:

1. Download the appropriate version for your platform from the [Releases](https://github.com/iainchesworth/aqualink-automate/releases) page.
2. Run the installer (Windows) or unpack the release archive (Linux, MacOS) to extract the executable binary.

## Building from Source

The following guide will walk you through the process of building Aqualink Automate from source.

### Pre-requisites

To build from source, you need:
- A supported version of your chosen build toolchain i.e. MSVC, gcc
- Have a working cmake (>= 3.20), git, and ninja installation

### Windows

To clone the repository (and, optionally, install the MSVC build toolchain plus dependencies)

```powershell

```

To build the application

```powershell

```

### Linux

To clone the repository (and, optionally, install build toolchain and dependencies)

```bash
$ git clone --recurse-submodules https://github.com/iainchesworth/aqualink-automate.git <local-repo-location>
$ cd <local-repo-location>
$ sudo ./cmake/tools/build-scripts/install-linux-deps.sh
```

To build the application

```bash
$ cd <local-repo-location>
$ cmake -G Ninja --preset linux-x64-release-gcc
$ cmake --build build/linux-x64-release-gcc
$ ctest --test-dir build/linux-x64-release-gcc/test --output-on-failure
```


### MacOS

```
TBC
```
