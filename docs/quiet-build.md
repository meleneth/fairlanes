# Quiet builds and tests

Use `scripts/build` for normal local and automated-agent builds. It runs the
existing CMake build without changing its flags or semantics, captures combined
stdout and stderr, and prints only a concise result.

```sh
scripts/build                         # build the default target
scripts/build fairlanes_tests         # build one target
scripts/build --test                  # build fairlanes_tests, then run CTest
scripts/build --configure             # configure, then build
scripts/build --verbose               # stream normal CMake/Ninja output too
scripts/build --build-dir build-other # select another build tree
```

If the selected build tree has no `CMakeCache.txt`, configuration happens
automatically. The default build tree remains `build-linux-debug`, with two
parallel jobs unless `--jobs` is supplied.

Complete output is retained in the selected tree:

- `logs/configure.log` for configuration
- `logs/build.log` for compilation, generation, and linking
- `logs/tests.log` for CTest

Successful reports include elapsed time, the warning count, and compiled-unit
count when recognizable build output provides one. A failure preserves the
underlying exit code and prints the failed command plus a bounded excerpt around
compiler, linker, CMake/generator, or test diagnostics. If no diagnostic is
recognized, it prints a bounded tail and points to the full log.

Use verbose mode when interactive progress is useful. For automated work, begin
with the concise report and inspect the referenced log only if the excerpt is
insufficient. Regression experiments remain under `scripts/regress`; its build
and command capture use the same quiet-command implementation.
