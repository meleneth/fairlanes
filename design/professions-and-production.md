# Professions and production

Primary authority for character expertise, party work assignments, materials,
crafting, and extended skill study. Character combat identity remains owned by
[characters](characters.md), equipment power by [equipment](equipment.md), and
consumable effects by [resources and consumables](resources-and-consumables.md).

## Ownership and purpose

**Decided:** Combat creates economic opportunities; professions convert them
into lasting character and account progression. Characters own profession
experience, proficiency, discovered techniques, and skill-study progress.
Expertise follows a character between unlocked parties. It is not a class,
affinity, or new primary-stat system.

**Decided:** Parties own activity assignments. The initial activities are
combat progression, targeted farming, gathering, crafting, skill study, and
idle. Gathering, crafting, and study consume simulated account-world beats and
do not advance that party's tower frontier at the same time. A change requested
during combat commits at the encounter boundary rather than interrupting the
encounter.

**Decided:** The account owns the shared production economy: material balances,
production objectives, and newly produced goods. Goods move to party inventory
through an explicit transfer that removes them from account inventory exactly
once. Characters may craft for other characters and parties.

## Materials and recipes

**Decided:** Material availability is tied to real unlocked Origin biomes.
Gathering never unlocks or bypasses a biome. Combat rewards and gathering may
both supply materials as the economy expands. Ordinary equipment and
consumables should increasingly come from professions; exceptional finished
drops remain valid during this migration.

**Implemented first slice:** Herbalism, Mining, and Scavenging gather a compact
Origin catalog. Alchemy produces the existing consumables. Tailoring,
Leatherworking, and Smithing produce ordinary armor and a weapon through the
existing equipment builder and biome-weighted stat allocator. Recipes declare
proficiency, duration, inputs, output, and XP in the Ruby content DSL.

**Decided:** Recipe inputs commit atomically when work begins. Rejection spends
nothing. Interruption after commitment does not refund materials and cannot
produce an output. Completion deposits one output exactly once. Crafted gear
retains identity, tier, affinity shape, modifiers, biome/material provenance,
crafter, and crafting proficiency.

## Expertise and study

**Decided:** Profession XP comes from meaningful completed work. Trivial work
stops granting XP once proficiency substantially exceeds its tier or recipe
requirement. Exact thresholds and XP values are experimental.

**Decided:** Observe discovers a skill; Study develops extended understanding.
Ordinary skills retain immediate successful Observe learning. Only declared
advanced skills enter character-owned study state. Study uses simulated time,
survives party transfer, and completes through the existing Grimoire and skill
slots rather than a parallel spellbook.

**Implemented first slice:** Eviscerate is the representative advanced skill.
Successful Observe makes it study-eligible; 120 experimental simulated seconds
of Study learns it and equips it when a normal slot is open. Encounter
retention rules apply to the observation result before study begins.

## Autonomous work and persistence

**Decided:** Production objectives may request materials, consumables, crafted
equipment, or study. Autonomous production uses only automatically controlled
parties that are idle or already working for the objective. It selects declared
inputs deterministically, reports stable blocked reasons, and never overrides a
manual assignment.

**Implemented first slice:** Autonomous material and consumable stockpiles use
the same operations as manual assignments. Multiple parties progress
independently from the account's single beat loop.

Character transfer preservation and screen continuity are **Implemented**. A
disk save/load format does not yet exist; serializing expertise, assignments,
operations, account inventory, committed inputs, and study progress remains
**Planned** rather than falsely claimed as implemented.

All durations, yields, XP values, proficiency thresholds, recipes, gathering
sources, and autonomous priorities are **Unresolved balance experiments**.
