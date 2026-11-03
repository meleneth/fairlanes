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

Recordings use `fairlanes-combat-recording` version 1 JSON. They contain the
fixture ID, build version and source revision, SHA-256 of the Ruby content
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

Ruby-authored scenarios use the same runner and replay path. The floor-10
scenario has two named variants in
`scripts/fairlanes_regression_declarations.rb`. Run `--fixture
origin_woodland_floor10 --variant arrival|detour --all-seeds --record-dir DIR`
to evaluate its declared seed set. Add `--assert-expected` to require its
win-count range and reject any beat cap. See `docs/origin-woodland-slice.md`
for paired results and replay commands.
