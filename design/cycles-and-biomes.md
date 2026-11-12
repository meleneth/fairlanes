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

**Decided:** Biome origin influences the probability distribution of equipment
bonuses without absolutely restricting them. This creates cross-biome farming
incentives: equipment from an earlier biome may remain useful when its stat
distribution better supports a newly observed skill. Exact Origin weights and
stat associations are experimental. Equipment owns the complete rule in
[equipment](equipment.md).

**Decided:** The five Origin biomes have distinct equipment-acquisition
emphases. Woodland, Plains, and Murky Swamp each offer one of the three pairwise
combinations of Cloth, Leather, and Plate; Caves emphasizes Jewelry; and Desert
emphasizes Weapons. The assignment of particular armor pairs to the first three
biomes remains **Unresolved**. Acquisition category is distinct from a biome's
stat-weight distribution.

These emphases are not late-game equipment unlocks. The existing early access
to all five Origin biomes is intentional, allowing equipment detours across the
overlapping progression network.

**Decided:** Origin monster affinities and player equipment specialization add
cross-biome reasons to detour. Enemies may combine Cloth, Leather, and Plate
disciplines, while players choose one active affinity and seek compatible gear
or skills in other towers. This does not change Cycle identities, biome
structure, or create biome-derived hero abilities.

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

**Decided:** For the current playable Origin foundation, clearing floor 9 of a
biome unlocks the next biome in the order above. This provides access to all
five towers without requiring completion of the preceding 100-floor tower.
Exact unlock milestones may be revisited as the overlapping progression economy
is balanced.

**Decided:** Origin autoplay is a deliberately limited early-game policy, not a
general game-playing AI. When enabled for a party, it observes committed
progression facts and submits ordinary destination and recruitment requests at
simulation boundaries. It advances the least-progressed unlocked Origin biome
with deterministic Origin-order tie breaking, uses a bounded retry/farming
recovery after defeats, and stops when no legal recovery exists. This simple
policy keeps all five towers moving through their shared recruitment bands.
Manual mode makes none of these strategic choices; both modes continue to use
the same party loop and combat simulation.

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

Per-party histories for all five Origin biomes, sequential floor-9 unlocks,
automatic frontier continuation, repeat-floor farming, wipe retry, boundary
destination changes, and floor-100 completion are **Implemented**. Every Origin
tower has a temporary generated 100-floor encounter table using its declared
monsters; those fallback patterns are implementation scaffolding, not settled
encounter design or balance.

The party-scoped `OriginAutoProgression` decision controller described above is
also **Implemented** for every party in the story account. The autoplay toggle
is global: existing controllers change mode together, and newly unlocked
parties inherit its current state. Each controller still reads and changes only
its assigned party's progression. The policy is intentionally Origin-only and
does not optimize equipment, skills, transfers, or later cycles.
