# Combat regression harness

Use `scripts/regress` as the normal entry point. It builds quietly, drives the
existing production regression executable, stores detailed output under
`build-linux-debug/regression-artifacts`, and prints an aggregate report instead
of compiler commands or combat events.

The regression harness shares its command capture and diagnostic extraction
with `scripts/build`. Use `scripts/build --test` for the complete CTest suite;
see [Quiet builds and tests](quiet-build.md).

```sh
scripts/regress summary
scripts/regress compare origin_woodland_brace_progression
scripts/regress compare origin_woodland_honey_badger --json
scripts/regress recruitment
scripts/regress inspect build-linux-debug/regression-artifacts/origin_woodland_brace_progression/learn_brace/recordings/origin_woodland_brace_progression.learn_brace.seed1.json --mechanic brace
scripts/regress verify
```

`run` and `compare` accept `--variant ID`, `--no-build`, and `--seeds`. Both run
all declared seeds and variants by default; `compare` is the conventional name
when interpreting paired variants. Recording and summary directories are made
automatically. Normal output gives outcomes, beat caps, simulated duration,
remaining HP, survivors, declared correctness checks, focused mechanic counts,
effective combat stats, damage dealt/received, and paired deltas. Balance
differences are labelled `OBSERVATION` and never
become failures unless the Ruby declaration explicitly asserts them.

For investigation, start with the concise comparison, identify the unexpected
variant or seed, and then use `inspect` with `--encounter`, `--skill`, or
`--mechanic brace|bleed|cover`. Add `--replay` to locate the first deterministic
replay divergence. Open the full recording only if that focused timeline is not
enough. Do not pipe whole recordings or build logs into an automated agent's
context.

`--json` emits `fairlanes-regression-experiment-summary` version 1. It contains
aggregates, compact per-seed outcomes, semantic assertion results, mechanic
metrics, and comparison deltas—not the event stream. Measurements unavailable
from authoritative data are `null`. Full recordings remain beside the compact
per-run `fairlanes-regression-run-summary` version 1 files. These summary schemas
are independent of combat recording versions.

Successful build and test output is captured rather than printed. A failure
reports the command, original exit status, focused compiler/assertion/replay
diagnostics, and the complete log path. `scripts/regress verify` checks the
regression tests, Ruby specs, generated content consistency, and every pinned
recording.

`scripts/regress recruitment` is the compact non-balance report for the Origin
roster schedule. It validates all generated milestone counts and runs the tagged
event, capacity, Village, and production floor-clear integration coverage while
keeping detailed Catch output in the regression artifact log.

## Low-level runner and recording format

Build the client and regression runner through the quiet entry point:

```sh
scripts/build fairlanes_regression
```

The underlying executable remains available for runner development and pinned
recording maintenance:

```sh
build-linux-debug/fairlanes_regression --fixture woodland_mouse_duel --seed 42 \
  --record /tmp/woodland_mouse_duel.json --assert-expected
```

Omit `--assert-expected` for exploration. `--beat-cap N` stops after N
simulated beats and records `beat_cap`, a distinct result from either side's
defeat. The default cap is 1200 beats. The runner never sleeps or renders.

Open a recording in the normal client party combat view:

```sh
build-linux-debug/fairlanes --replay /tmp/woodland_mouse_duel.json
```

Playback starts at 1×. Use `=` or `+` and `-` to change overdrive, or the
existing console command `overdrive N`. Playback re-simulates the same fixture
through world, account, party, encounter, and ATB beats. Overdrive changes the
wall-clock delay between beats, not simulated uWu or gameplay events. When the
recorded fight ends, gameplay beats freeze so the final state stays visible.
Quit with `q`, then confirm. For automated verification without a screen, use
`build-linux-debug/fairlanes --replay /tmp/woodland_mouse_duel.json --no-ui`.
The first event or terminal-result mismatch is printed as a replay divergence.

The `woodland_mouse_duel` fixture creates one player and one generated Field
Mouse through `EncounterBuilder`. The player equips only rank-one Thump; the
mouse knows only rank-one Thump. Each chooses that skill and the sole opposing
target. The player starts
at 20 HP, 2 Attack, 0 Armor; the mouse starts at 8 HP, 0 Attack, 0 Armor.
Neither has gear or resistance. Seed 42 yields `hero_victory`.

Existing simple recordings continue to use `fairlanes-combat-recording` version
1 JSON. Ruby scenarios opt into version 2 when they need longitudinal or richer
metrics. Version 2 adds an ordered encounter list, encounter indexes and global
timestamps on events, per-encounter results, retained party snapshots, and
explicit between-encounter skill selection inputs. The reader and replay path
accept both versions. Recordings contain the fixture ID, build version and source
revision, SHA-256 of the Ruby content
declarations, generator, and library sources, gameplay seed and sequence,
beat cap, expected and actual results, explicit initial actor stats, ATB
charge, statuses, loadouts, policies, and external gameplay inputs (empty for
this automatic fixture). Each event has a stable actor ID, sequence number,
and uWu timestamp.
Action, cast, and hit events show the chosen skill, target, and damage. State
events show HP and status changes; the end event holds the terminal outcome.
Same-uWu events retain their production callback order followed by actor-order
state snapshots. Visual effects and presentation random choices do not use the
gameplay RNG stream. The included sample is
`samples/regression/woodland_mouse_duel.seed42.json`.

Ruby-authored scenarios use the same runner and replay path. The declarations
in `scripts/fairlanes_regression_declarations.rb` can equip Observe for selected
members, replace a member's one configured combat skill, select a retained skill
between declared encounters, and omit gear. This
keeps paired variants identical except for the mechanic under test. Add
`--assert-expected` to require the declared win-count range and reject any beat
cap. Ruby variants may also declare focused semantic assertions. The harness
evaluates only those declarations; an unasserted balance disadvantage does not
fail correctness.

The physical progression fixtures are:

| Fixture | Variants | Semantic check |
| --- | --- | --- |
| `origin_woodland_stag_lesson` | `observe_equipped`, `observe_unequipped` | The stag casts Brace in both; only the eligible equipped observer learns and retains it |
| `origin_woodland_brace_practice` | `braced`, `unbraced` | Brace applies, reduces and consumes on the first qualifying hit, then the next hit is ordinary |
| `origin_woodland_brace_progression` | `learn_brace`, `no_brace` | One retained party crosses floors 7–8; only the observing variant learns and then equips Brace through the normal member API |
| `origin_plains_cover_lesson` | `cover` | A projectile aimed at the meerkat resolves on the boar before Bumper Rush, then reaches the meerkat after the screen opens |
| `origin_woodland_floor10` | `offensive`, `defensive` | The two-member opening party compares two Thump users with one Brace user against the same early physical milestone |
| `origin_woodland_honey_badger` | `offensive`, `defensive` | Twelve seeds pin Eviscerate casts, bleed applications/ticks/damage/deaths, outcomes, survivors, HP, and duration at floor 50 |
| `origin_equipment_floor50` | `minimal`, `moderate`, `offensive`, `defensive`, `swamp_distribution` | Eight seeds compare deterministic generated Origin equipment through effective stats, damage, duration, and survival at floor 50; the numeric results are unasserted balance observations |

An action event retains the originally aimed target. Its hit event contains the
resolved target, so Cover assertions do not infer interception from log text.
Status state events similarly make Brace application and consumption explicit.
The Cover fixture now ends in hero victory with Attack scaling active; its
asserted purpose remains the causal interception and release sequence.

Use `scripts/regress compare FIXTURE` for these experiments. The lower-level
equivalent still supports `--all-seeds`, `--record-dir`, and `--summary-dir`.

Readable pinned recordings are replayed through the client path with:

```sh
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_stag_lesson.observe_equipped.seed1.json --no-ui
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_brace_practice.braced.seed1.json --no-ui
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_brace_progression.learn_brace.seed1.json --no-ui
build-linux-debug/fairlanes --replay samples/regression/origin_plains_cover_lesson.cover.seed1.json --no-ui
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.offensive.seed1.json --no-ui
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_honey_badger.offensive.seed1.json --no-ui
```

See `docs/origin-woodland-slice.md` for the authored fights and paired results.
