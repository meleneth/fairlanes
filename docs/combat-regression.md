# Combat regression smoke fixture

Build the client and regression runner:

```sh
cmake --build build-linux-debug -j 4
```

Run the one authored fixture and require its expected `hero_victory`:

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
cap.

The physical progression fixtures are:

| Fixture | Variants | Semantic check |
| --- | --- | --- |
| `origin_woodland_stag_lesson` | `observe_equipped`, `observe_unequipped` | The stag casts Brace in both; only the eligible equipped observer learns and retains it |
| `origin_woodland_brace_practice` | `braced`, `unbraced` | Brace applies, reduces and consumes on the first qualifying hit, then the next hit is ordinary |
| `origin_woodland_brace_progression` | `learn_brace`, `no_brace` | One retained party crosses floors 7–8; only the observing variant learns and then equips Brace through the normal member API |
| `origin_plains_cover_lesson` | `cover` | A projectile aimed at the meerkat resolves on the boar before Bumper Rush, then reaches the meerkat after the screen opens |
| `origin_woodland_floor10` | `offensive`, `defensive` | The two-member opening party compares two Thump users with one Brace user against the same early physical milestone |
| `origin_woodland_honey_badger` | `offensive`, `defensive` | Twelve seeds pin Eviscerate casts, bleed applications/ticks/damage/deaths, outcomes, survivors, HP, and duration at floor 50 |

An action event retains the originally aimed target. Its hit event contains the
resolved target, so Cover assertions do not infer interception from log text.
Status state events similarly make Brace application and consumption explicit.
The Cover fixture's declared result is an enemy victory; its purpose is the
causal interception sequence, while the production story-route test proves the
normal party can clear Plains 1–3.

```sh
build-linux-debug/fairlanes_regression --fixture origin_woodland_stag_lesson --variant observe_equipped --all-seeds --record-dir /tmp/fairlanes-stag --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_brace_practice --variant braced --all-seeds --record-dir /tmp/fairlanes-brace --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_brace_progression --variant learn_brace --all-seeds --record-dir /tmp/fairlanes-brace-progression --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_plains_cover_lesson --variant cover --all-seeds --record-dir /tmp/fairlanes-cover --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant offensive --all-seeds --record-dir /tmp/fairlanes-floor10 --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_honey_badger --variant offensive --all-seeds --record-dir /tmp/fairlanes-badger --assert-expected
```

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
