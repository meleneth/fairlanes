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
| Woodland 10 | Antler Stag, Bark Smack, and Bumpkin Hare | Consolidate timing, Brace, composition, and early equipment decisions without requiring Brace |
| Woodland 50 | Honey Badger | Later status-management milestone; cleansing is intentionally not designed yet |
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
Woodland floor 5 also commits the first generated recruitment opportunity. It
remains available on the Village recruitment board until claimed; details and
the other Origin milestones are in [recruitment progression](recruitment-progression.md).

| Item ID | Bonus | Job and source |
| --- | --- | --- |
| `woodland_stick` | Attack +2 | One fewer Thump hit against a 5-HP mouse; global special drop |
| `stitched_hide` | Armor +2 | Save up to 2 HP on repeated direct physical hits; global special drop |
| `hearty_locket` | Max HP +5 | Survive a 12-point hit from 10 HP; global special drop |
| `mouse_nibbled_cape` | Armor +1 | Arrival cape; global special drop |
| `mossbound_cape` | Armor +3, Max HP +5 | First Plains floor clear; no global drop weight |

Effective bonuses read current equipment. Equipping never heals; removing
Max HP clamps current HP to the new cap. Resistance bonuses remain intact.

## Woodland regression baselines

The [Ruby declaration](../scripts/fairlanes_regression_declarations.rb)
uses the current two-member opening party for floor 10. Both members have 20 HP,
2 Attack, and rank-one Thump; the leader wears the Village Guard Hide. Across
seeds 1–8, the offensive pair wins 8/8 in 568–659 beats. Replacing the leader's
Thump with Brace wins 2/8 in 798–1330 beats. Brace is useful when it catches a
physical hit, but repeatedly defending with half of this small party sacrifices
too much damage. The encounter therefore rewards sensible timing and composition
without making Brace mandatory or introducing a new mechanic.

The longitudinal `origin_woodland_brace_progression` fixture retains one party
across production floor-7 and floor-8 encounters. Its learning variant equips
Observe normally, witnesses the stag's Brace, retains the learned skill on
victory, and selects it through `PartyMember::equip_known_skill` before floor 8.
The paired variant selects Thump instead. Both variants win 8/8. At seed 1,
Brace applies and is consumed three times on floor 8. The Brace route takes 4
damage and finishes both fights in 930 beats, while the offensive route takes 1
damage on floor 8 and finishes in 641 beats. Individual Brace hits are mitigated,
but the longer fight creates more opportunities for incoming damage. This is an
actual result of giving one of two heroes a purely defensive action, not a claim
that Brace is generally harmful.

Honey Badger is excluded from random Woodland pools and explicitly authored at
floor 50, so no generated floor before 50 can introduce it. The exploratory
no-cleansing baseline uses twelve seeds. The two-Thump party wins 6/12; its runs
record 60 Eviscerate casts, 56 bleed applications, 202 ticks for 404 damage, and
18 bleed-attributed deaths. A one-Brace/one-Thump party wins 0/12 and records
81 casts, 78 applications, 240 ticks for 480 damage, and 24 bleed deaths.
Offensive runs last 54.1–73.8 simulated seconds; defensive runs last 50.8–79.3.
This is the intended counter relationship: Brace cannot mitigate periodic Dire
Bleed, so it loses decisively without the later cleansing answer. Dire Bleed
persists until its owner dies or combat cleanup occurs; a dead source is noticed
and cleared at the next scheduled tick. Eviscerate has no immediate hit, and
recasting replaces the status and restarts its scheduled tick cadence. Those are
current implementation facts, not a final balance prescription. No cleansing
skill is implemented or assigned in this slice.

```sh
cmake --build build-linux-debug -j 4
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant offensive --all-seeds --record-dir /tmp/fairlanes-floor10-offensive --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant defensive --all-seeds --record-dir /tmp/fairlanes-floor10-defensive --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_brace_progression --variant learn_brace --all-seeds --record-dir /tmp/fairlanes-brace-progression --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_honey_badger --variant offensive --all-seeds --record-dir /tmp/fairlanes-floor50 --assert-expected
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_stag_lesson.observe_equipped.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_brace_practice.braced.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_plains_cover_lesson.cover.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.offensive.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_brace_progression.learn_brace.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_honey_badger.offensive.seed1.json --overdrive 8
```

The replay commands open the actual client combat view. Quit with `q` and
confirm. Add `--no-ui` for automated replay comparison.
