# Development Environment Setup

## Overview

Fairlanes is developed primarily on **Windows using MSYS2 (UCRT64)** with **VS Code**.

You will need:
- Git
- CMake
- A working C++ toolchain (via MSYS2 UCRT)

---

## Install MSYS2

- Website: https://www.msys2.org/
- Installer: https://github.com/msys2/msys2-installer/releases/download/2026-03-22/msys2-x86_64-20260322.exe

After installing, launch the **UCRT64 shell** (not MINGW64, not MSYS).

---

## Update MSYS2

First run:

```sh
pacman -Syu
```

Close the terminal when prompted, reopen **UCRT64**, then run again:

```sh
pacman -Syu
```

---

## Install Toolchain

Install the core development tools:

```sh
pacman -S \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-gdb \
  mingw-w64-ucrt-x86_64-make \
  git
```

### What this gives you

- `gcc` — compiler
- `cmake` — build system generator
- `ninja` — fast builds (preferred)
- `gdb` — debugger
- `git` — source control

---

## Verify Installation

```sh
gcc --version
cmake --version
git --version
```

If those work, you're good.

---

## VS Code Setup

Recommended extensions:
- C/C++
- CMake Tools

### Important

Make sure VS Code is using the **UCRT64 environment**.

The easiest way:

```sh
code .
```

Run this from the MSYS2 UCRT64 shell.

---
## Build (Quick Check)

From the repo root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/fairlanes.exe
```

On my machine, VS Code build and test integration worked out of the box.
(I'm not sure why we currently have 3 failing tests, this is news to me)

If tests are unexpectedly failing, verify:
- you are in the MSYS2 **UCRT64** shell
- the build completed successfully before running `ctest`

---

## CMake

Fairlanes uses CMake extensively.

All third-party dependencies are fetched and built as part of the main build.  
You should not need to install libraries manually.

---

## Tracy

Fairlanes includes [Tracy](https://github.com/wolfpld/tracy) for runtime profiling.

This is why Windows may show a **network access warning** when you run the game.  
Tracy opens a connection for live profiling.

For development, this is expected.  
For production builds, profiling will likely be disabled.

To inspect profiling data:

- Download the [Tracy client](https://github.com/wolfpld/tracy/releases)
- Run the game
- Connect using the Tracy UI
