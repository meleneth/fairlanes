# Equipment balance experiments

> **Brainstorm — non-canonical.** Accepted equipment intent lives in
> [`design/equipment.md`](../design/equipment.md). All numbers and associations
> here are candidates to test, not requirements.

## First Origin experiment

Keep the first implementation within the existing combat vocabulary: Attack,
Armor, maximum HP, and per-channel resistances. This avoids inventing primary
stats before enough skills exist to distinguish them coherently.

Candidate policy questions:

- What inclusive tier budget ranges and character-level thresholds make
  upgrades noticeable without making tier the only sensible choice?
- Which slots may roll Attack, Armor, HP, or resistances, and how large should
  their budget multipliers be?
- Which provisional weight vectors make the five Origin biomes measurably
  different without making any stat exclusive to one biome?
- Should ordinary items allocate all budget deterministically from a random
  weighted sequence, or reserve a predictable slot-specific portion first?
- Should three-item upgrades require matching provenance, preserve one selected
  input distribution, or combine distributions? Mixed-biome behavior needs an
  explicit design decision before it becomes permanent.
- What simple score avoids obvious automatic-gearing downgrades while leaving
  deliberate specialization to manual equipment selection?

Provisional acquisition mapping to test (not approved): Woodland Cloth/Leather,
Plains Leather/Plate, and Murky Swamp Cloth/Plate. Caves then weights ordinary
drops toward Jewelry and Desert toward Weapons. Swap the first three mappings
during experiments if another ordering produces clearer early choices.

## Measurement target

Use deterministic Origin encounters around floors 45–60 to compare minimal,
moderate, offense-emphasized, defense-emphasized, and different biome-weighted
loadouts. Record damage dealt and received, encounter beats, survival, effective
Attack/Armor/HP/resistances, equipped skills, and seed. Enemy scaling should not
be changed to manufacture a favorable result.

## Implemented provisional curve

The first experiment currently uses these inclusive budget ranges and minimum
character levels:

| Tier | Level | Budget |
| --- | ---: | ---: |
| Worn | 1 | 1–2 |
| Plain | 1 | 2–4 |
| Sturdy | 10 | 4–6 |
| Fine | 25 | 6–9 |
| Excellent | 40 | 9–13 |
| Masterwork | 55 | 13–18 |
| Mythic | 75 | 18–24 |

A point currently buys +1 Attack, +1 Armor, +3 maximum HP, or +5 percentage
points of one resistance. Slot weights make weapons strongly prefer Attack,
ordinary armor prefer Armor/HP/resistances, capes prefer resistances, and
jewelry allow the broadest mixture. The Ruby declarations contain the exact
experimental biome vectors and acquisition weights.

The provisional armor-pair mapping is Woodland Cloth/Leather, Plains
Leather/Plate, and Murky Swamp Cloth/Plate. Caves assigns 75/95 ordinary-kind
weight to Jewelry and Desert assigns 75/95 to Weapons. Every profile retains a
nonzero chance for the other ordinary categories.

## Floor-50 result (8 deterministic seeds)

Fixture: `origin_equipment_floor50`, five Thump users against the authored
Woodland floor-50 Honey Badger. These observations are not balance targets.

| Loadout | Wins | Seconds | Party HP | Dealt | Received | Mean Atk | Armor | Max HP |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Worn Woodland cape | 8/8 | 30.86 | 146.25 | 53.50 | 9.00 | 4.08 | 1.30 | 30.98 |
| Fine Woodland cape | 8/8 | 29.57 | 169.12 | 52.75 | 9.00 | 4.50 | 2.65 | 35.18 |
| Excellent Desert weapon | 8/8 | 14.35 | 168.38 | 58.62 | 0.00 | 12.55 | 1.38 | 33.68 |
| Excellent Caves chest | 8/8 | 29.90 | 176.62 | 52.50 | 9.38 | 4.28 | 5.40 | 36.75 |
| Excellent Swamp chest | 8/8 | 30.54 | 189.75 | 53.38 | 9.38 | 4.20 | 3.55 | 39.60 |

All parties survived, so this setup distinguishes duration and attrition rather
than win rate. Offensive gear ended the encounter before Eviscerate landed.
HP-heavy gear increased Dire Bleed tick size because that status scales from
effective maximum HP; this explains its slightly higher recorded incoming
damage despite better survival. The swamp distribution averaged 10% magical
resistance, while the Caves distribution produced more Armor and physical/cold
resistance. The offensive seed-1 recording replayed identically through the
production replay path.
