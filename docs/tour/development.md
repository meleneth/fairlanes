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

## Generated Content

The Ruby content declarations are the source of truth for generated skill
metadata C++: stable IDs/enums, display names, learn chances, flee chances,
execution kind tags, visual mappings, and random-combat membership. They also
generate status display/debug metadata, monster stable IDs/enums, known-skill
metadata, and encounter pool membership. Monster stat metadata such as name, HP,
MP, and level is generated from the same declarations, along with the thin
monster registration glue that applies those generated stats. Handwritten C++
remains the authority for skill behavior, status lifecycles, combat effects,
rendering implementation, and monster construction primitives.

Generator code lives in the local `ruby/fairlanes_content` gem.
Fairlanes-specific DSL usage lives outside the gem in
`scripts/fairlanes_content_declarations.rb`.

The thin launcher, `scripts/fairlanes_content_codegen.rb`, loads the local gem,
validates the decal skill/monster declarations, and writes build-tree artifacts
under:

```sh
<build-dir>/generated/fairlanes_content
```

Run the content check target after changing decal skill or monster metadata:

```sh
cmake --build build --target fairlanes_content_check
```

To verify an existing generated output directory without rewriting files:

```sh
ruby scripts/fairlanes_content_codegen.rb --out-dir build/generated/fairlanes_content --check
```

Run the Ruby generator specs with:

```sh
cd ruby/fairlanes_content
rspec
```

The generated skill metadata source is compiled into `fl`. The generated Catch2
source is compiled into `fairlanes_tests`. The generated Markdown balance report,
effect gallery metadata, JSON manifest, and manifest schema are for review and
tooling.

Declaration IDs default to the matching C++ enum spelling and display spelling:
`:flame_wave` becomes `FlameWave` and `Flame Wave`. Keep explicit `cpp_id:` or
`display:` only when the runtime spelling intentionally differs from that
convention. Skills default to random-combat availability; use
`random_combat: false` for explicit opt-outs such as `Observe`.

Use `decal_skill` for the simple decal-strike family. It fixes the execution kind
and declarative shape to the generated decal primitive while keeping visual,
tags, and metadata declarative.

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
