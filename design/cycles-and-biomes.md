# Cycles and biomes

Primary authority for progression-cycle and biome-tower structure. Character
level bands and XP are owned by [progression](progression.md).

## Structure

**Decided:** Fairlanes has four cycles: Origin, Resonance, Conflict, and
Singularity. Each contains five 100-floor biome towers. Preserve the existing
cycle structure and biome identities; this document does not invent additional
biomes.

**Decided:** A cycle's five biomes form an overlapping progression network, not
five independent sequential leveling ladders. Character level does not map
directly to the same floor number in every biome. Players should move between
biomes for useful skills, equipment, recruitment opportunities, and other
capabilities.

**Decided:** Tower history, biome unlocks, the selected farming destination,
and pending destination changes are party-scoped. Strategic destination
requests made during combat commit at the encounter boundary; the latest valid
request wins. Manual play chooses the plan but still advances encounters and
frontier floors automatically. Autoplay uses the same machinery and additionally
makes strategic choices.

Only a newly committed frontier clear advances highest-cleared progress or
emits a floor-clear fact. A cleared-floor victory repeats the selected farming
floor. A wipe recovers and retries the same selected floor. Floor 100 completes
the tower exactly once, never constructs floor 101, and does not choose another
biome without a separate strategic decision.

Generated floor manifests and the generated
[`brainstorms/biome-tower-map.md`](../brainstorms/biome-tower-map.md) report
current declarations. They do not override this intended structure.

## Origin order

**Decided:** The Origin biome order is:

1. Woodland.
2. Plains.
3. Murky Swamp.
4. Caves.
5. Desert.

The order describes the progression shape, not a requirement to finish each
tower before touching the next.

## Cross-biome gates

**Decided:** Special encounters may demand capabilities that are easier to
obtain elsewhere, making detours part of progression rather than failure.

**Decided:** Honey Badger belongs around Woodland floor 50. Eviscerate's bleed
punishes parties that rely exclusively on ordinary physical mitigation.
Cleansing or another effective response should become available through Origin
progression.

**Unresolved:** The source and acquisition route for that response, along with
the broader Origin difficulty curve. Candidate questions belong in
[`brainstorms/origin-cycle-progression.md`](../brainstorms/origin-cycle-progression.md).

**Implemented:** Current declarations place Honey Badger at Woodland floor 50.
The first playable slice and its current numbers are documented in
[the Origin Woodland progression slice](../docs/origin-woodland-slice.md).

Per-party Woodland/Plains histories, automatic frontier continuation,
repeat-floor farming, wipe retry, boundary destination changes, and floor-100
completion are **Implemented**. The remaining declared biomes still require
resolved encounter content before they can use the same runtime path.
