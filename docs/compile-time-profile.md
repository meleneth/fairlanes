# Compile-time profile: first measured baseline

Captured after `180c5cd`, 2026-09-06. No header optimizations have been applied.
Eight selected translation units were compiled sequentially, separately with
Clang 22.1.6 time traces and GCC 14.2 `-ftime-report`. These are a targeted sample,
not a full clean-build benchmark. The normal Debug flags and include paths came
from `build-linux-debug/compile_commands.json`; outputs went under `/tmp`.
Compiler-specific diagnostic flags were removed for Clang, and warnings did not
fail the profiling compilations. All sixteen compilations succeeded.

## Measured compilation wall time

| Translation unit | Clang trace run | GCC report run |
| --- | ---: | ---: |
| `context.cpp` | 6.89s | 9.10s |
| `visual_resolver.cpp` | 7.58s | 11.83s |
| `encounter_data.cpp` | 9.38s | 13.64s |
| `party_data.cpp` | 12.25s | 18.12s |
| `skill_sequence.cpp` | 11.59s | 20.25s |
| generated `visitor_fall.cpp` | 2.18s | 4.20s |
| `root_component.cpp` | 6.66s | 10.68s |
| `grand_central.cpp` | 10.40s | 16.91s |

Clang recorded 66.52s inside ExecuteCompiler, 58.20s in Frontend, and 8.02s
in Backend: roughly 88% frontend in this sample. GCC independently shows large
parsing and instantiation costs: `context.cpp` spent 6.38s parsing, 2.12s in
language-deferred work, and 0.38s in optimization/code generation. Its template
instantiation counter was 3.80s, overlapping those phases. `skill_sequence.cpp`
spent 9.17s parsing, 4.51s deferred, and 5.44s generating code, with 6.32s of
instantiation work across phases.

Different instrumentation and one sequential sample do not establish that one
compiler is faster. Filesystem cache, host load, compiler version and profiling
overhead affect measurements. These numbers also do not predict parallel build
wall time or the savings from removing an include.

## Concrete header costs

Clang Source spans across the eight compilations report these **inclusive** costs:

| Header | Aggregate time | Recorded occurrences |
| --- | ---: | ---: |
| `entt/entt.hpp` | 10.082s | 9 |
| `encounter_data.hpp` | 8.623s | 6 |
| standard `<chrono>` | 6.327s | 7 |
| `account_data.hpp` | 5.958s | 4 |
| `party_data.hpp` | 5.362s | 5 |
| `grand_central.hpp` | 4.044s | 1 |
| `root_component.hpp` | 3.975s | 2 |
| `context.hpp` | 3.809s | 6 |
| `party_bus.hpp` | 3.026s | 7 |

Header times overlap through nested includes and **must not be summed**. Counts
are recorded trace occurrences, not unique dependents or include directives.
Include order also changes which header is charged for a shared dependency.

A particularly useful small case is the generated Visitor skill. Its definition
header consumed 1.881s, including 1.501s attributed to `combat_status.hpp` and
1.285s within that to the EnTT umbrella. The include chain is:

```text
visitor_fall.cpp -> visitor_fall.hpp -> skill_definition.hpp
                                      -> combat_status.hpp -> entt/entt.hpp
```

Skill metadata needs the status-kind enum, yet receives the full status component
and ECS dependency. Moving such enums to small definition headers is a concrete
candidate to measure. Any changes to generated includes must be made in Ruby.

The trace also identifies repeated standard-format instantiations: char
`std::vformat_to` accumulated 2.399s and the wchar variant 1.724s across seven
occurrences each. Their internal implementations appear separately with overlapping
timings. `<chrono>` includes `bits/chrono_io.h` (3.788s inclusive here), so clock
headers can bring substantially more work than their data types suggest.

## Include analysis

Clang-tidy 19.1.7 `misc-include-cleaner` ran on `context.cpp`, `party_data.cpp`,
`encounter_data.cpp`, and `root_component.cpp`, using an isolated compilation
database stripped of profiling flags. It reported one unused direct include
(`party_bus.hpp` in `context.cpp`) and 71 missing-direct-provider diagnostics,
including EnTT types, standard types and FTXUI helpers.

These are dependency-hygiene suggestions requiring review, not 72 proven
performance fixes. Removing a redundant direct include does not eliminate its
cost if another included header still pulls it in. The measured priority is
reducing what shared headers expose, then making their consumers include their
actual providers. No automatic fixes were applied.

## Next optimization experiments

1. Separate lightweight status/decal identifiers from component/rendering headers
   used by generated metadata. Reprofile the generated Visitor definition.
2. Move AccountData construction/destruction and inline raid behavior into its
   implementation; forward-declare pointer-owned raid/statistics/log types.
3. Reduce the encounter/party/ATB header chain. PartyLoopMachine already provides
   an example of an implementation object that hides state-machine machinery.
4. Replace EnTT umbrella includes with appropriate forward/declaration headers
   where complete registry operations are unnecessary.
5. Rerun identical GCC compilations and controlled incremental builds after each
   change. Consider precompiled headers/cache separately after dependency cleanup.

## Reproduce

`scripts/profile_compile_times.py` replays compile commands without touching the
normal object files. Use a new output directory for each capture. Do not run a
normal build concurrently with a measurement.

```sh
python3 scripts/profile_compile_times.py \
  --compiler /opt/devastation/toolchains/clang-22.1.6/bin/clang++ \
  --output /tmp/fairlanes-clang-next \
  --file src/fl/context.cpp \
  --file src/fl/primitives/party_data.cpp \
  --file src/fl/primitives/encounter_data.cpp \
  --file src/fl/skills/skill_sequence.cpp \
  --file src/fl/widgets/root_component.cpp \
  --file src/fl/ecs/systems/visual_resolver.cpp \
  --file src/fl/grand_central.cpp \
  --file fl/generated/skills/visitor_fall.cpp
```

For GCC use `--compiler /usr/bin/c++ --mode gcc` and a different output directory.
For all project/generated/test compilation entries, use `--file ALL`. The script
produces `results.json`, exact replay commands, object files, compiler logs, Clang
traces where applicable, and `report.md`. `--summarize-only` rereads a capture
without compiling. Clang traces include both complete events and Clang 22's
asynchronous Source spans; four Python tests cover the normalization:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s tests/tools -p 'test_profile_compile_times.py'
```

Initial local artifacts: `/tmp/fairlanes-clang-profile`,
`/tmp/fairlanes-gcc-profile`, and `/tmp/fairlanes-include-profile/findings.log`.
These temporary artifacts are not committed and may disappear on cleanup.

Tool references: [Clang time tracing](https://clang.llvm.org/docs/UsersManual.html#cmdoption-ftime-trace),
[GCC phase reports](https://gcc.gnu.org/onlinedocs/gcc-15.1.0/gcc/Developer-Options.html),
[include-cleaner](https://clang.llvm.org/extra/clang-tidy/checks/misc/include-cleaner.html).

## Implemented improvements (2026-09-06)

The baseline above is retained for comparison. Two dependency cleanups and an
optional native GCC/Clang engine PCH are now implemented, with no gameplay,
ownership, container, or ECS API changes.

- Status and decal identifiers have small dedicated headers. Generated skill
  definitions no longer include status components and rendering implementation.
- EnTT umbrella includes are replaced by specific registry, entity, handle, or
  forward-declaration headers. Data-only Stats now includes only what it uses.
- CMake shares one stable third-party PCH across the five engine libraries.
  It contains `<chrono>`, EnTT registry, FTXUI component, and SML headers.
  Generated metadata skips this PCH entirely. The pre-existing Catch2 test PCH
  is extended with the same stable headers; it has its own build because test
  compiler flags differ from engine-library flags.

### Fresh compilation measurements

The same eight GCC translation units were recompiled into new temporary objects,
with the same sequential profiling method. A separate PCH probe measured creation
from scratch plus all seven engine compilations; generated metadata remained
unprecompiled. These are clean **compilation-work** measurements, not a claim of
a measured full clean parallel-build wall-time improvement.

| Source | Original | Narrow includes, no engine PCH | With engine PCH |
| --- | ---: | ---: | ---: |
| `context.cpp` | 9.10s | 5.63s | 3.46s |
| `visual_resolver.cpp` | 11.83s | 7.30s | 5.16s |
| `encounter_data.cpp` | 13.64s | 8.85s | 6.53s |
| `party_data.cpp` | 18.12s | 12.13s | 10.05s |
| `skill_sequence.cpp` | 20.25s | 12.73s | 10.62s |
| `visitor_fall.cpp` | 4.20s | 0.88s | 0.88s |
| `root_component.cpp` | 10.68s | 6.56s | 4.45s |
| `grand_central.cpp` | 16.91s | 10.75s | 8.53s |
| PCH creation | — | — | 4.61s |
| **Total, including PCH creation** | **104.73s** | **64.82s** | **54.28s** |

The dependency cleanup reduced sampled work by 38%; including the PCH reduced
it by 48% from the original. PCH itself saves another 16% even after creation
cost in this small sample. No link or unchanged-file cache was involved.
Larger builds amortize creation differently, and host/cache variation still applies.

### Validation and controls

- GCC normal builds and all 285 C++ tests pass both before enabling the
  engine PCH and afterward. The existing Catch2-only PCH was present in the
  earlier run; no engine or game dependencies were precompiled there.
- Clang 22.1.6 successfully creates the same third-party PCH and compiles
  all seven sampled engine files against it. This is targeted Clang
  compatibility coverage, not a full Clang game/test-suite build.
- Six Python tests cover profiler spans and IWYU argument handling.
  IWYU strips CMake PCH injection so analysis sees actual includes.
- `FAIRLANES_ENABLE_ENGINE_PCH=OFF` disables the added engine PCH.
  `CMAKE_DISABLE_PRECOMPILE_HEADERS=ON` disables all CMake-managed PCHs,
  including the previously existing Catch2 PCH. Emscripten and compilers
  outside GNU/Clang do not enable this engine PCH path.
- PCH contains no Fairlanes headers. Keep direct includes correct; never
  rely on PCH for declarations. Libraries sharing it must retain matching
  flags and defines, as required by CMake REUSE_FROM.
- Profiling `--without-pch` forces full parsing for header comparisons.
  Clang replay of a GCC compilation database automatically strips CMake
  PCH injection because the serialized formats are incompatible.
  Normal per-source profiling does not include PCH creation; count it
  separately when assessing clean builds.

Additional local captures: `/tmp/fairlanes-gcc-narrow-headers`,
`/tmp/fairlanes-pch-probe`, `/tmp/fairlanes-pch-probe-clang`.

Further account/ATB ownership refactors were deliberately deferred: these
changes already remove substantial work without altering implementation
boundaries or adding opaque wrapper layers.

PCH references: [CMake](https://cmake.org/cmake/help/latest/command/target_precompile_headers.html),
[GCC](https://gcc.gnu.org/onlinedocs/gcc/Precompiled-Headers.html).
