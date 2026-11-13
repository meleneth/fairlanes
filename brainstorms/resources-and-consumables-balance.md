# Resources and consumables balance experiments

This file records provisional numbers and observations. It is not canonical
design authority.

## Implemented experimental values

- Adrenaline starts at 0/10; Reflexes starts at 10/10.
- Thump generates 2 Adrenaline on hit. Thump Around costs 4 and gains up to 6
  damage from its pre-cost Adrenaline. Venom Needle gains up to 5 damage from
  remaining Reflex percentage. Brace restores 3 Reflexes; Web Snare costs 4.
- Healing potions restore 12 HP; resource potions restore 6. Autonomous healing
  begins at 35% HP and requires at least half the potion to be useful.
- Attack and Armor flasks add 3; Tempo adds 20 percentage points to charge rate.
  All last 120 simulated seconds. Identical effects replace and refresh.
- The first Origin recruit and the sandbox festival receive one experimental
  starter pack: four healing potions, two of each resource potion, and one of
  each flask. Acquisition and economy beyond this demonstration remain open.

## Deterministic observations (seeded, 2026-11-13)

- `origin_resource_chain`, seed 17: starting at 0 Adrenaline generated 12 and
  later spent 8 through the ordinary fallback/resource-skill decision path,
  dealt 63 damage, and won in 91.25 seconds. Starting at 10 generated 4, spent
  12, dealt 69, and won in 47.33 seconds.
- `origin_reflex_scaling`, seed 23: 0 Reflexes dealt 4 damage; 10 Reflexes dealt
  9 under otherwise identical conditions. Both lost, so the measurement isolates
  effect magnitude rather than claiming a balanced outcome.
- `origin_consumable_survival`, seed 31: no consumables lost after dealing 52
  and receiving 43 damage. Four healing potions produced a 114.83-second win
  with 21 party HP. One prepared Attack flask produced a 69.83-second win with
  9 party HP. These are causal samples, not target win rates.

## Unresolved

Capacity progression, passive Reflex recovery, potion acquisition and price,
flask preparation UI, multi-resource skills, refunds, final AI thresholds, and
all coefficients remain open to balance review.
