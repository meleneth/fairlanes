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
