# Origin Woodland progression slice

Use `tower woodland` in the client console to start the selected party's tower
route. Wins advance its resolved floor lineup; a wipe retries the floor after
recovery. A clear returns the party to full effective HP. Clearing Woodland
floor 9 pauses the route until the player chooses `tower plains` or
`tower woodland`. The party may clear Plains floor 1 for a cape,
equip it in the party inventory, then use `tower woodland` to return to floor
10. `tower status` shows the current route. Encounter selection outside a
selected tower remains unchanged.

The floor-9 clear emits `BiomeUnlocked`. The client shows a colored notice
that rises from the lower right to the upper right. It stays there until
Space or Enter acknowledges it; that key is consumed before normal navigation.
The same notice queue handles `CycleUnlocked` events and the existing raid
achievement event. Combat and progression continue while a notice is shown.

| Woodland milestone | Encounter job |
| --- | --- |
| Floors 1–3 | Starter Field Mice and Bumpkin Hare |
| Floors 4–6 | Repeated light physical hits from mice, hares, and Mire Squish |
| Floors 7–9 | Heavier Bark Smack hits mixed with light attackers |
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
has 120 HP. The asserted win-count ranges are 2–3 of 8 on arrival and 3–4
of 8 after the detour; a beat cap fails separately from a loss.

| Seed | Arrival | Beats | Party HP / boss HP | Detour | Beats | Party HP / boss HP |
| ---: | --- | ---: | ---: | --- | ---: | ---: |
| 1 | Loss | 1459 | 0 / 23 | Win | 1673 | 16 / 0 |
| 2 | Loss | 1570 | 0 / 13 | Loss | 1570 | 0 / 13 |
| 3 | Win | 1728 | 18 / 0 | Win | 1728 | 16 / 0 |
| 4 | Loss | 1456 | 0 / 24 | Loss | 1447 | 0 / 24 |
| 5 | Loss | 1458 | 0 / 41 | Loss | 1458 | 0 / 41 |
| 6 | Win | 1743 | 25 / 0 | Win | 1743 | 25 / 0 |
| 7 | Loss | 1438 | 0 / 21 | Loss | 1447 | 0 / 17 |
| 8 | Loss | 1549 | 0 / 20 | Loss | 1549 | 0 / 20 |

No run hit the cap. Seed 1 shows the item benefit. Dire Bleed ticks the
leader for 2 HP. Arrival gear leaves the leader at 2 HP before the uWu 52880
tick, which kills them after two Thumps. The detour cape starts them at 25
rather than 20 HP; that tick leaves 5 HP. They take a third action at uWu
56800 and hit Honey Badger for 4 at uWu 58880 (71→67 HP), before dying on
a later tick. Another hero eventually finishes the boss. The upgrade moves
the fixed set from two to three wins while leaving alternate arrival clears.

```sh
cmake --build build-linux-debug -j 4
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant arrival --all-seeds --record-dir /tmp/fairlanes-floor10-arrival --assert-expected
build-linux-debug/fairlanes_regression --fixture origin_woodland_floor10 --variant detour --all-seeds --record-dir /tmp/fairlanes-floor10-detour --assert-expected
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.arrival.seed1.json --overdrive 8
build-linux-debug/fairlanes --replay samples/regression/origin_woodland_floor10.detour.seed1.json --overdrive 8
```

The replay commands open the actual client combat view. Quit with `q` and
confirm. Add `--no-ui` for automated replay comparison.
