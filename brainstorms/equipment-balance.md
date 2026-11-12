# Equipment balance experiments

## Origin affinity and tower first pass

The implemented experiment uses a 20% matching-affinity contribution after the
declared stat contribution. Damage generally reads Attack; mitigation and the
initial Web Snare control path read Armor. Neutral declarations receive no
affinity multiplier. These coefficients, caps, and Armor as the available
control proxy are provisional.

Affinity switching keeps equipped pieces and lets automatic gearing replace a
slot only when a compatible, level-eligible improvement exists. Unique items
and inventory ownership are preserved.

The tower experiment derives effective level from the configured 1–100 floor
curve, then applies bounded identity, biome, and authored-difficulty adjustments.
HP uses half identity HP plus effective level; Attack uses one configured level
curve to avoid double floor scaling. Representative findings, especially around
Woodland 45–60 and Honey Badger, remain balance evidence rather than decisions.

The pinned floor-10 two-hero sample now wins 3/8 offensive seeds and 0/8 with
one hero spending turns on Brace. This is an exposed early breakpoint, not a
target win rate. At Woodland floor 50 the two-hero no-cleanse samples win 0/12
with either offense or Brace; Dire Bleed remains the decisive gate and direct
physical mitigation still does not answer it.

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
| Worn Woodland cape | 8/8 | 63.85 | 106.12 | 110.38 | 53.62 | 4.08 | 1.30 | 30.98 |
| Fine Woodland cape | 8/8 | 57.72 | 142.38 | 110.25 | 46.00 | 4.50 | 2.65 | 35.18 |
| Excellent Desert weapon | 8/8 | 29.90 | 160.50 | 112.25 | 8.62 | 12.55 | 1.38 | 33.68 |
| Excellent Caves chest | 8/8 | 59.50 | 143.62 | 109.88 | 52.75 | 4.28 | 5.40 | 36.75 |
| Excellent Swamp chest | 8/8 | 60.64 | 148.62 | 109.50 | 57.38 | 4.20 | 3.55 | 39.60 |

Every variant won, but the non-offensive variants sometimes lost party members.
Offensive gear substantially shortened exposure to Eviscerate. HP-heavy gear
increases Dire Bleed tick size because that status scales from effective maximum
HP; the sampled swamp distribution also averaged 10% magical resistance, while
the Caves distribution produced more Armor and physical/cold resistance.

## Longitudinal Origin sample (seed 1)

The five-hero baseline has 30 HP, 4 Attack, 1 Armor, Thump, and no equipment.
The requested floors cover all five Origin biomes across the sample. All sampled
monsters currently have zero authored resistance, which is itself a visible
content gap.

| Floor | Biome | Enemy level / HP / Attack | Result | Seconds | Dealt / received |
| ---: | --- | --- | --- | ---: | ---: |
| 1 | Woodland | 1 / 3 / 1 | Win, 5 survive | 7.25 | 6 / 0 |
| 10 | Plains | 8–11 / 10–25 / 2–3 | Win, 5 survive | 60.17 | 105 / 30 |
| 25 | Murky Swamp | 28–29 / 40–48 / 6 | Win, 5 survive | 144.83 | 199 / 17 |
| 40 | Caves | 40–42 / 50–57 / 8–9 | Win, 5 survive | 145.08 | 140 / 22 |
| 50 | Desert | 53–54 / 69–71 / 11 | Loss | 185.50 | 251 / 163 |
| 60 | Woodland | 57–61 / 59–69 / 12–13 | Loss | 159.50 | 161 / 143 |
| 75 | Murky Swamp | 78–79 / 90–98 / 16 | Win, 5 survive | 282.33 | 402 / 61 |
| 100 | Desert | 103–104 / 119–121 / 21 | Loss | 99.00 | 118 / 160 |

The non-monotonic floor-75 win after floor-50 and floor-60 losses comes from
lineup behavior and counterplay rather than a stat-curve reversal. It is a useful
warning that encounter composition currently dominates some large floor gaps.
