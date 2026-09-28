# Event-driven cleanup

This is the cleanup list from the combat, skill, and UI event-path audit.
Fairlanes intentionally uses a hybrid architecture: keep direct calls when one
clear owner performs an operation, and add events for requests crossing an
authority boundary or for committed gameplay facts that need observers.

## Priority 1: combat correctness

- [ ] Centralize successful Flee resolution instead of setting `Stats::hp_` to
      zero in `SkillSequencer`.
  - Preserve the distinction between fleeing and dying.
  - Emit the committed flee fact from the authority that removes the combatant.
  - Re-evaluate party wipe and encounter completion after removal.
  - Ensure death-only cleanup and attribution do not run for a flee.
  - Add coverage for one player fleeing, every player fleeing, an enemy fleeing,
    and Flee failing or being rejected in a raid.

- [ ] Add authoritative post-commit damage and defeat facts.
  - Report actual HP removed after mitigation, capped by the defender's HP
    before the hit; keep attempted damage and overkill separately if useful.
  - Include attacker, defender, damage channels, encounter kind/identity, and
    skill or status attribution where available.
  - Emit one alive-to-dead transition fact per defeated combatant.
  - Cover direct skill damage, Poison, Dire Bleed, status detonation, field or
    environmental damage, and simultaneous raid outcomes.
  - Emit facts before encounter cleanup can invalidate attribution data.

- [ ] Move enemy-defeat reactions behind the committed defeat fact.
  - Replace the direct `GrantXPToParty::commit` call from `TakeDamage` with a
    scoped listener or another explicitly ordered event consumer.
  - Keep raid kill loot and XP suppression explicit.
  - Avoid listener-registration-order dependencies between XP, loot, cleanup,
    statistics, and achievements.

## Priority 2: missing committed gameplay facts

- [ ] Make XP changes observable.
  - Either use the existing `PartyGainedXP` event or replace it with a more
    precise committed-XP contract.
  - Emit it after XP is actually applied, including the party, amount, source,
    and affected members as required by the final statistics contract.
  - Retain `PartyGainedLevel` as a separate derived fact.

- [ ] Publish an actual normal-loot award fact.
  - Do not treat `LootDropRequested` as proof that an item was created or added.
  - Emit only after the item is successfully inserted into party inventory.
  - Include the source, recipient party, item identity/type, and encounter
    attribution needed by statistics.
  - Preserve `RaidLootAwarded` as the authoritative raid-specific award path.

- [ ] Add a post-commit combat-healing fact.
  - Include healer, target, skill, requested amount, and actual HP restored.
  - Emit for Mercyburst, single-target heals, group heals, drains, and future
    combat recovery paths.
  - Keep combat healing distinct from town revitalization and resurrection.
  - Decide whether `PartyHealed` remains town recovery or becomes a broader
    event with an explicit healing cause.

## Priority 3: skill progression and interaction

- [ ] Separate witnessed-skill publication from learning resolution.
  - Keep `SkillWitnessed` as the discovery/observation fact.
  - Route learning attempts through one progression listener or controller
    instead of calling `teach_party_from_observed_skill` from every sequence.
  - Preserve per-observer Observe rank, chance, party scope, raid forwarding,
    and the raid-summoning retention loophole.
  - Ensure a new skill execution cannot publish a witness while accidentally
    omitting learning, or learn a skill without publishing the witness.

- [ ] Publish committed skill-progression facts.
  - Define facts for a skill learned, retained after combat, revoked after a
    failed outcome, equipped automatically, and dropped by the player.
  - Emit only after the grimoire and skill slots have been updated.
  - Carry stable attribution needed by future statistics and achievements.

- [ ] Route skill dropping through a request/commit boundary.
  - Replace the direct `PlayerDetailsPane -> PartyData::drop_skill` mutation
    with a small callback/controller or `SkillDropRequested` event.
  - Validate membership, slot, and skill ownership outside the widget.
  - Publish `SkillDropped` only after both the slot and grimoire commit.
  - Keep the widget responsible only for selection, input routing, and display.

## Priority 4: UI lifecycle cleanup

- [ ] Move raid screen transitions out of `RootComponent::Render`.
  - Subscribe the root/shell navigation layer to `RaidStarted` and
    `RaidResolved`, or feed those facts into an explicit UI update method.
  - Handle result-expiry navigation outside the render pass.
  - Keep `Render` free of screen replacement and other composition mutations.
  - Verify that raid navigation works even when rendering is delayed or paused.

- [ ] Move farming-plan submission out of `FarmingChoiceView`.
  - Submit the selected discipline/focus through a callback, controller, or
    typed request rather than calling `PartyData::select_farming_plan` directly.
  - Keep recommendation preview and selection cursor state inside the view.
  - Publish a committed plan-selected fact only if another subsystem needs to
    observe the choice.

## Priority 5: status observability

- [ ] Standardize post-commit status facts without forcing every status command
      through an event.
  - Continue calling `CombatStatusSystem` directly where it is the clear owner.
  - Add facts such as status applied, refreshed, removed, cleansed, expired, and
    field effect applied/removed when observers require them.
  - Align Poison and Freeze with the generic status contract while preserving
    their specialized freeze/thaw and damage behavior.
  - Ensure cleanup emits at most one removal fact per committed status removal.

## Existing event-driven paths to preserve

- [ ] Keep equipment interaction as
      `EquipmentEquipRequested -> ManualEquipmentSystem -> EquipmentEquipped`.
- [ ] Keep return-to-town input as
      `PartyReturnToTownRequested -> PartyLoopMachine`.
- [ ] Keep raid records driven by `RaidStarted`, `RaidResolved`,
      `RaidRecorded`, `RaidLootAwarded`, and `AchievementUnlocked`.
- [ ] Keep the combat timing spine driven by
      `Beat -> PartyTick -> ATB -> BecameActive -> FinishedTurn`.
- [ ] Keep account/party selection and overdrive controls as shell/configuration
      state unless a real observer requirement emerges.

## Completion checks

- [ ] Update `docs/tour/event-flow.md` and
      `docs/statistics-achievements-design.md` when contracts change.
- [ ] Update the event reference for every added, removed, or renamed event.
- [ ] Add focused tests for producer ordering, committed values, duplicate
      suppression, listener lifetime, cleanup, raids, and attract-mode exclusion.
- [ ] Run `cmake --build build-linux-debug`.
- [ ] Run `ctest --test-dir build-linux-debug --output-on-failure`.
