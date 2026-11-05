# Character and experience progression

Primary authority for character levels, XP intent, and progression freedom.
Tower structure is owned by [cycles and biomes](cycles-and-biomes.md).

## Four progression cycles

**Decided:** Fairlanes has four major progression cycles with approximate
character-level bands:

| Cycle | Approximate character levels |
| --- | ---: |
| Origin | 1–100 |
| Resonance | 101–200 |
| Conflict | 201–300 |
| Singularity | 301–400 |

These are intended progression bands, not mandatory hard caps. Their exact
boundaries remain **Unresolved** for balancing. A character need not reach an
arbitrary maximum level before entering another cycle.

## Leveling philosophy

**Decided:** Levels establish baseline combat power. Gear shapes and modifies
that baseline. Skills determine how available power is applied. Leveling alone
must not become a universal answer to progression barriers.

Players may solve difficult encounters through combinations of levels,
equipment, learned skills, skill loadouts, party composition, tactical choices,
and biome detours. Skilled players may advance while underleveled; farming may
produce modest overleveling.

## Experience philosophy

**Decided:** XP rewards meaningful challenges appropriate to a character's
current progression. Substantially outleveling an encounter must sharply reduce
its XP. Suppression must be severe enough that obsolete encounters cannot
practically provide indefinite advancement. Small amounts of overlevel farming
are acceptable; long-term weak-enemy farming should be practically worthless.

**Decided:** Repetition also suppresses XP, but it is distinct from overlevel
suppression. Repeating appropriately challenging content must not immediately
destroy its value. The purpose is to discourage exploiting one efficient farm,
not to punish ordinary progression.

The intended model keeps four concerns distinct:

1. XP required for the next character level.
2. Base XP awarded by an encounter.
3. Suppression based on character level relative to encounter difficulty.
4. Suppression based on repeated farming of the same content.

**Unresolved:** Formulas, thresholds, history scope, distribution, catch-up
rules, and the mathematical relationship among these mechanisms. They are
explored in [`brainstorms/character-progression.md`](../brainstorms/character-progression.md)
and [`brainstorms/experience-suppression.md`](../brainstorms/experience-suppression.md).

## Progression gates

**Decided:** Difficult encounters should prompt investigation of defensive
equipment, skills, loadouts, recruits, other biomes, and alternative strategies.
Grinding weak enemies must not invalidate those systems. Prefer strong economic
incentives and diminishing returns over artificial restrictions; hard
character-level caps and mandatory level requirements require an explicit new
decision.

## Current implementation gaps

- **Planned:** Level progression beyond 100. `TrackXP` currently stops producing
  a next-level threshold after level 100, while the accepted progression spans
  approximately 400 levels.
- **Planned:** Challenge-relative and repetition XP suppression. Current kills
  grant a flat 256 XP to every member of the killer's party, regardless of
  level, encounter difficulty, or farming history.
- **Unresolved:** The current XP curve, equal whole-party award, and permanent
  max-HP gain on level-up are implementation choices, not canonical formulas.
