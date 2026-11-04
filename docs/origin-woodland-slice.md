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
floor. Advertised seconds are always simulated time converted to beats and uWu;
overdrive only increases how many of those beats run per wall second. Thus an
8-second effect takes about 1 wall second at 8x without changing its gameplay
duration. `tower status` and the party status row show the attempt count. Choosing
a different biome or floor resets the count; clearing a floor advances to the
next floor and resets it before that floor's first attempt.

Ruby `tower_pattern` declarations generate each five-floor composition cycle
from an ordered list of one to five monster types. Within the cycle, later
types replace more of the five slots. Level, HP, and Attack use one generated
percentage for the whole five-floor tier, rising from 5% on floors 1–5 to 100%
on floors 96–100. Authored floor overrides normally retain that tier's
percentage, and may override Level, HP, or Attack percentages independently for
balance experiments.

The percentages are applied to runtime maxima when an encounter is built. Use
`--tower-max-level`, `--tower-max-hp`, and `--tower-max-attack` with either
`fairlanes` or `fairlanes_regression` to run balance experiments without
recompiling. Defaults are level 20, 100 HP, and 20 Attack at 100%.

The floor-9 clear emits `BiomeUnlocked`. The client shows a colored notice
that rises from the lower right to the upper right. It stays there until
Space or Enter acknowledges it; that key is consumed before normal navigation.
The same notice queue handles `CycleUnlocked` events and the existing raid
achievement event. Combat and progression continue while a notice is shown.

| Floor | Authored encounter | Teaching job |
| --- | --- | --- |
| Woodland 1 | Field Mouse | First story victory |
| Woodland 2 | Field Mouse | Second story victory |
| Woodland 3 | Two Field Mice | Short multi-target introduction |
| Woodland 4 | Hare | Faster solo enemy |
| Woodland 5 | Field Mouse and Hare | Mixed speed and composition |
| Woodland 6 | Bark Smack | First solo heavy hitter |
| Woodland 7 | Antler Stag | Observe and learn rank-one Brace |
| Woodland 8 | Two Bark Smacks | Spend Brace against direct physical hits |
| Woodland 9 | Field Mouse, Hare, and Bark Smack | Short mixed gate fight; unlock Plains on victory |
| Woodland 10 | Honey Badger and four Field Mice | Optional milestone before or after Plains |
| Plains 1 | Prairie Meerkat | Physical Pebble Spit projectile; awards Mossbound Cape |
| Plains 2 | Prairie Meerkat and Grass Wolf | Ranged and melee composition |
| Plains 3 | Prairie Meerkat and Bristle Boar | Projectile Cover and its Bumper Rush opening |

Brace is a combat-local self status. It reduces the next landed direct physical
hit by 35% after resistance and percentage modifiers but before flat Armor and
the positive-hit minimum, then is spent. A miss, periodic tick, or nonphysical
hit leaves it armed. Observe must be equipped when the other combatant casts
Brace; it does not need to be the action selected on that turn. Learning uses
the normal grimoire, closet, auto-equip, seeded roll, and wipe rollback paths.

On Plains 3, the living boar screens the meerkat from direct single-target
physical projectiles. HP loss, visuals, kill handling, and `SkillHitLanded` all
use the boar as the resolved target while the action trace retains the aimed
meerkat. Melee and periodic damage bypass Cover. The boar's first Bumper Rush,
or its death, ends the screen for that encounter.

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
seeds. At seed 1 their hit sequence and 688-beat timing are identical, while
the return cape leaves the leader at 25 HP instead of the arrival cape's 20 HP.
Plains does not add a player skill or interrupt in this slice, so the detour is
primarily a route, encounter, and isolated cape comparison. These assertions
protect deterministic execution while the runtime maximum flags are used to
search for useful balance ranges.

```sh
cmake --build build-linux-debug -j 4
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant arrival --all-seeds --record-dir /tmp/fairlanes-floor10-arrival --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant detour --all-seeds --record-dir /tmp/fairlanes-floor10-detour --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant arrival --all-seeds --record-dir /tmp/fairlanes-floor10-sweep --tower-max-level 40 --tower-max-hp 400 --tower-max-attack 60
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_stag_lesson.observe_equipped.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_brace_practice.braced.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_plains_cover_lesson.cover.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.arrival.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.detour.seed1.json --overdrive 8
```

The replay commands open the actual client combat view. Quit with `q` and
confirm. Add `--no-ui` for automated replay comparison.
