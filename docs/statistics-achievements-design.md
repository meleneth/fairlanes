# Statistics and achievements: initial design

Status: design in progress, 2026-09-06. Establishing these systems alongside raids
is requested. This pass is explicitly design and milestones only.
No gameplay statistics tracker or achievement system was found in
the current implementation. The contracts and milestones below are proposals
for review, not implemented features or finalized achievement content.

## Confirmed scope

Support both **per-account records** and **save-wide milestones**. Account records
retain their own statistics and attribution; save-wide milestones can recognize
progress across accounts. Decide individual achievement definitions and any
additional account-local unlocks explicitly. A shared raid contributes one account
outcome and one corresponding outcome to save-wide aggregation, not five wins
because five parties received credit.

## Purpose and separation

Statistics answer what happened and how much. Achievements recognize named
milestones or feats based on those facts. Keep stored measurements, achievement
rules, and presentation separate. Do not infer either from rendered widgets,
human-facing logs, effect animations, or profiling counters.

The [raid design](raid-design.md) requires one shared account outcome while
preserving participation and victory credit for all five parties, even wiped
parties. Statistics and achievement semantics must respect that distinction.

## Required event-system architecture

**Statistics and achievements must be implemented through the project's event
system.** This is a confirmed architectural requirement, not an optional approach.
Gameplay producers publish specific typed facts through project bus types.
Statistics listeners update the appropriate account/save records. Achievement
listeners evaluate committed facts or explicit statistic-progress events and
publish an achievement-unlocked event exactly once per defined scope.

Do not wire gameplay code directly to a tracker or achievement evaluator, poll
ECS state from a view to infer progress, or parse logs. Internal model helpers
may be called by listeners; the integration boundary is the event system.
Use scoped subscriptions with owner-controlled lifetimes and explicit event
ordering wherever evaluation depends on updated statistics. Do not rely on
listener registration order. Select one counting path for each fact to avoid
counting both a source event and its derived progress event as two occurrences.

Raid summoning is a distinct combat-exit fact. It retains successfully observed
skills from the interrupted fight but does not count as that fight's victory,
an enemy kill, a death, or an ordinary flee. Track the interruption separately
if useful. Achievement predicates must distinguish it from a real win. The
intentional timing loophole is valid gameplay; whether it deserves a named
achievement is future content design, not a reason to prevent the behavior.

## Proposed first measurements

| Measurement | Proposed contract |
| --- | --- |
| Encounters started/completed | Distinguish normal party encounters from shared raids; assign an encounter identity |
| Raid attempts/wins/losses | One attempt and at most one final outcome per account raid, regardless of party count |
| Party raid participation/victory credit | One record for each participating party; wiped parties still receive shared win credit |
| Character deaths / party wipes | Count the actual transition; a party wipe does not imply raid defeat |
| Damage dealt/taken | Actual HP removed after mitigation, capped by prior HP; attempted damage and overkill are separate possible metrics |
| Healing done/received | Actual HP restored; separate combat healing from town recovery |
| Enemies defeated | One alive-to-dead transition per enemy; settle kill attribution for damage over time and environmental effects |
| Items awarded | Count actual granted items, not loot requests; identify raid-exclusive awards |
| Raid duration | Combat elapsed time, distinct from the paused account calendar; decide whether to also retain wall time |

Begin implementation with a small subset that has reliable producers. Raid
measurements cannot be wired to real outcomes until shared raid lifecycle exists.
Do not invent empty raid events just to make a tracker appear integrated.

## Proposed ownership and records

Use data-oriented account records with encounter summaries and optional party
and character breakdowns, plus save-wide milestone state. Keep ownership and
aggregation explicit across these two confirmed scopes. Do not count a single
shared raid five times when aggregating party participation into account totals.

Snapshot attribution while participants exist: account, home party, character,
monster/skill identifiers where relevant, encounter kind, progression Cycle,
and encounter identity. Do not retain pointers to temporary contexts or destroyed
combatants. Persisted identities must not rely on reusable ECS entity values.
Keep time units explicit, especially combat time versus account calendar time.

Recording a final encounter outcome or item award twice must not grant duplicate
credit. Use explicit identities and resolution guards rather than relying on UI
visits or event subscription order. Do not impose an unbounded event log as the
storage model; decide what totals and recent summaries actually need retention.

## Existing seams and gaps

- `PartyBus` and `CombatantBus` provide scoped subscriptions, player-death,
  party-wipe, skill-hit, healing, and loot-request events.
- `DiscoveryJournalListener` demonstrates owner-managed subscriptions with
  attract-demo isolation. Discovery is presence/absence knowledge, not statistics.
- `PartyData::leave_combat` currently emits `PartyVictory` when members survive.
  Audit/fix the outcome contract before using it as a genuine win counter.
  Raid summoning needs explicit exit/retention events rather than borrowing this
  victory signal to keep skills.
- `TakeDamage::commit` calculates post-mitigation damage but returns a total that
  can exceed remaining HP. Damage statistics need the actual before/after HP
  difference. Capture the fact before death callbacks can clean up the encounter.
- Poison and Dire Bleed call `TakeDamage`; `SkillHitLanded` is emitted by selected
  skill execution paths. Counting only skill-hit events misses other damage.
- `LootDropRequested` requests loot; it does not prove an item was awarded.
- A global clock exists, but account-calendar pause and shared raid outcome
  producers are still to be designed and implemented.

Prefer authoritative facts emitted after gameplay commits, with enough captured
identity to survive cleanup. Resolve lifecycle and measurement ambiguities at the
producer before subscribing a tracker. Listeners must disconnect before owners
or buses die. Gameplay should not depend on whether a stats view is open.

## Proposed achievement foundation

Define stable achievement IDs, display text, scope, and a named predicate or
threshold over reliable facts. Keep definition data separate from unlocked state.
An unlock should occur once in its chosen scope and publish a notification once;
rendering or reopening a screen must not re-grant it.

Candidate first achievements for discussion, not approved content:

- Win a first raid.
- Win a raid after one or more participating parties have wiped.
- Clear a progression-gate raid for a particular Cycle transition.

Each needs the resolved encounter facts, not merely a counter change. The
second example requires a party-wiped-during-raid fact, including if revival is
later allowed. Progression-gate credit must not be inferred from any moon-raid win.
Achievements should not implicitly award items, power, or currency; whether they
have rewards is an open product decision. Raid-exclusive loot remains governed
by the raid reward contract.

## Open decisions

- Which specific milestones aggregate across accounts, and which achievements
  also have account-local unlocks? Both account records and save-wide milestones
  are required.
- Must the first implementation persist across restarts? What is the save owner,
  format/versioning policy, and behavior for old saves with no recorded history?
- Which measurements matter first, and which breakdowns/retention are useful?
- Are achievements cosmetic, or do they have rewards? Are any hidden or repeatable?
- Are unlocks retroactive when rules change, and can progress reset by Cycle?
- How are summons, damage over time, friendly fire, overhealing, revival, and
  simultaneous deaths attributed?
- Should tests, attract mode, and debug cheats count? Proposed default: exclude
  attract/demo activity and isolate test data; settle debug behavior explicitly.

## Proposed implementation sequence

1. Define owners for account records and save-wide milestones, choose persistence
   scope and the first small metric set. Document
   the units and exact counting contracts before implementing counters.
2. Add a focused statistics model and repair/add the necessary authoritative
   producers. Wire scoped listeners in normal gameplay, excluding attract mode.
3. Test actual damage, single death transitions, disconnect/lifetime behavior,
   genuine wins versus exits, and duplicate outcome protection as applicable.
4. Add a small achievement definition/evaluation model on those validated facts;
   drive evaluation and unlock notifications through typed events; verify
   one-time unlocks, scope isolation, explicit update ordering, and no duplicate
   counts from source/derived events. Add persistence if required for
   the initial release rather than labeling session-only counts "lifetime".
5. Once raids exist, integrate account raid summaries: one win plus five party
   victory credits, including wiped parties, and ten actually awarded items.
6. Add read-only statistics/achievement views and notifications after the data
   contracts are stable. Keep raid layout priorities intact.
