# Origin Woodland progression slice

The story's first two victories clear Woodland floors 1 and 2, unlocking the
next floor and funding the village hide purchase. Use `tower woodland` or
`tower plains` in the client console to choose a biome at its latest unlocked
floor. Add a floor number, such as `tower woodland 1`, to replay any floor up
to that biome's latest unlocked floor. A win unlocks the next floor only when
the latest floor is cleared; a wipe retries the floor after recovery. A clear
returns the party to full effective HP. Clearing Woodland
floor 9 pauses the route until the player chooses `tower plains` or
`tower woodland`. The party may clear Plains floor 1 for a cape,
equip it in the party inventory, then use `tower woodland` to return to floor
10. `tower status` shows the current route. Encounter selection outside a
selected tower remains unchanged.

A wipe keeps the selected biome and floor. After the existing 60-second town
recovery, manual play and story autoplay both start another attempt on that
floor. `tower status` and the party status row show the attempt count. Choosing
a different biome or floor resets the count; clearing a floor advances to the
next floor and resets it before that floor's first attempt.

Ruby `tower_pattern` declarations generate each five-floor composition cycle
from an ordered list of one to five monster types. Within the cycle, later
types replace more of the five slots. Level, HP, and Attack use one generated
percentage for the whole five-floor tier, rising from 5% on floors 1–5 to 100%
on floors 96–100. Authored floor overrides retain that tier's percentage.

The percentages are applied to runtime maxima when an encounter is built. Use
`--tower-max-level`, `--tower-max-hp`, and `--tower-max-attack` with either
`fairlanes` or `fairlanes_regression` to run balance experiments without
recompiling. Defaults are level 20, 100 HP, and 20 Attack at 100%.

The floor-9 clear emits `BiomeUnlocked`. The client shows a colored notice
that rises from the lower right to the upper right. It stays there until
Space or Enter acknowledges it; that key is consumed before normal navigation.
The same notice queue handles `CycleUnlocked` events and the existing raid
achievement event. Combat and progression continue while a notice is shown.

| Woodland milestone | Encounter job |
| --- | --- |
| Floors 1–2 | One Field Mouse each for the opening story victories |
| Floors 3–5 | The Woodland pattern introduces Hare, Squish, Bark Smack, and Poison Toad |
| Floors 6–9 | The composition repeats at 10% of the runtime maxima |
| Floor 9 clear | Unlock Origin Plains Tower before floor 10 |
| Floor 10 | Authored Honey Badger and four Field Mice |

The historical README biome list names Plains with wolves and meerkats. The
generated [biome tower map](../brainstorms/biome-tower-map.md) gives five towers
per Cycle, monster skill lists, current runtime pools, and assignment gaps.

| Item ID | Bonus | Job and source |
| --- | --- | --- |
| `woodland_stick` | Attack +2 | One fewer Thump hit against a 5-HP mouse; global special drop |
| `stitched_hide` | Armor +2 | Save up to 2 HP on repeated direct physical hits; global special drop |
| `hearty_locket` | Max HP +5 | Survive a 12-point hit from 10 HP; global special drop |
| `mouse_nibbled_cape` | Armor +1 | Arrival cape; global special drop |
| `mossbound_cape` | Armor +3, Max HP +5 | First Plains floor clear; no global drop weight |

Effective bonuses read current equipment. Equipping never heals; removing
Max HP clamps current HP to the new cap. Resistance bonuses remain intact.

## Floor-10 Ruby regression

The [Ruby declaration](../scripts/fairlanes_regression_declarations.rb)
shares a five-hero setup across two variants. Each hero has 20 base HP,
2 Attack, 0 Armor, and rank-one Thump. Only the leader's cape changes;
the town heal fills effective Max HP before combat. Both use the generated
Woodland floor-10 lineup, seeds 1–8, and a 2400-beat cap. Honey Badger now
uses the floor's 10% scaling. With the default maxima, every floor-10 enemy
has 10 HP, level 2, and 2 Attack. Both cape variants currently win all eight
seeds. These assertions protect deterministic execution while the runtime
maximum flags are used to search for useful balance ranges.

```sh
cmake --build build-linux-debug -j 4
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant arrival --all-seeds --record-dir /tmp/fairlanes-floor10-arrival --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant detour --all-seeds --record-dir /tmp/fairlanes-floor10-detour --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant arrival --all-seeds --record-dir /tmp/fairlanes-floor10-sweep --tower-max-level 40 --tower-max-hp 400 --tower-max-attack 60
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.arrival.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.detour.seed1.json --overdrive 8
```

The replay commands open the actual client combat view. Quit with `q` and
confirm. Add `--no-ui` for automated replay comparison.
