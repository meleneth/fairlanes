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
