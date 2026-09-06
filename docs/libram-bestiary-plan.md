# Libram, bestiary, and monster decisions

Every monster declaration includes a nonempty flavor description. The bestiary
shows that description when the monster is encountered, above its base stats and
witnessed skill links. Descriptions do not mark skills as witnessed.

Implement these goals in order. The game-wide reference is the libram; a
member's collection of learned, ranked skills remains the grimoire.

Skill descriptions are part of the implementation requirements. Every new or
changed skill must state its actual damage/healing numbers and damage channel,
target count, chance, duration, tick interval/count, and rank formula where
applicable. Describe status replacement, removal and lethal consequences
explicitly; a status with no timeout must say so. Verify against the execution
path and ECS systems, not the name, tags or intended future design. Document
missing effects plainly. Update descriptions in the same change as behavior.
The libram reports unbalanced current values, with a visible notice that no
balance passes have been done; damage values precede combat modifiers and times
are combat time. Content validation rejects descriptions without numbers, but
review must still verify that those numbers match the implementation.

1. **Reference and discovery data.** Give every skill a textual description in
   content metadata. Track encountered monster kinds and witnessed skill uses
   for the game session, shared across accounts. A discovered monster exposes
   one slot per declared skill; an unwitnessed skill projects as `--` without an
   identifier or link. Discovery does not require Observe or a victory. Party
   discovery events populate the journal through listeners owned by the normal
   game; the attract demo does not wire those listeners.
2. **Reference screens.** Witnessed skills unlock descriptions in the libram. Browse
   discovered monsters in the bestiary and follow witnessed skills into the
   libram. Keep screen navigation in the shell and projection in the views.
3. **Monster decisions.** Add validated, ordered rules to monster declarations.
   Select both a skill and a matching target. Support status presence/absence,
   health thresholds, percentage gates, and fallback. Rules must respect skill
   availability and never redirect a combo to an unrelated target.
4. **Buff and combo behavior.** Exercise rules with real monster examples and
   tests: fill missing allied buffs, skip redundant buffs, and require a DoT
   before its follow-up. Document status occupancy, consumption, and cleanup.

Each goal requires focused checks before proceeding. Session discovery does not
imply save/load persistence; the project needs an explicit save format before
discovery can survive restarting the game.

Initial audit: statuses are per combatant, and group shields already work.
Monster choices currently sample usable equipped skills uniformly. Some content
tags and `placeholder_effect` labels lag behind handwritten runtime effects;
reference prose must describe the runtime behavior.

Milestones 1 and 2 are implemented and covered by discovery, content, and UI
navigation tests. Use `l` / `screen libram` and `b` / `screen bestiary`.

## Decision contract

`monster_rules` stores an ordered list on the monster declaration. A rule names
a known skill, a `self`, `ally`, or `enemy` target selector, optional
`chance_percent` (default 100), and an AND-list of conditions. Conditions use
`has_status`, `missing_status`, `hp_below`, or `hp_above`; `subject: :actor`
checks the actor instead of the candidate target. Duplicate rules provide OR
branches without an additional expression language.

Only living targets qualify. Conditions must all match the same candidate.
Percentage gates roll once per eligible rule after target and skill checks;
0 never fires, 100 always fires, and a 30% rule passes rolls 1 through 30.
Health comparisons are strict and use maximum HP without integer rounding.
The actor must have the skill equipped and be able to use it under current
statuses. No matching rule means the turn ends without an action. Monsters with
no rule list keep their existing random selection policy. Rule selection is a
direct decision service; witnessed actions remain events.

Current Shield and Haste effects can exist on any number of combatants. One
combatant has at most one effect of each shared status kind; reapplication
replaces the prior effect. Missing-Shield conditions treat all shields as the
same occupancy, regardless of source or strength. The existing status lifetime
handles expiration, death, fleeing, and leaving combat.

A percentage decision gate is distinct from the selected skill's own outcome
roll: a 30% Flee rule still uses Flee's separate 65% escape chance.

## Combat examples

Glass Lizard shields an unshielded ally before using its attack/flee rules.
Chrome Gecko similarly fills missing Haste. Drill Beetle shields itself before
choosing attacks. These are initial authored policies; other monsters retain
random choice until their declarations acquire rules.

Salamander now knows Kindle Wound and Cinderburst in addition to Flame Strike.
It prioritizes detonating an existing Burn, then applying a Burn, then its
fallback attack. Cinderburst is a status-detonation skill with a declared
consumed status and base fire damage (8). It checks the prerequisite before
scheduling and again at impact. Cleansing/expiration cancels the effect, not
turn completion. Successful consumption cancels the old burn's pending ticks.
The selected target remains fixed throughout the sequence.

The first consumption primitive supports shared `CombatStatusKind` effects.
Rules can already test Poison, Dire Bleed, and Freeze; consuming their bespoke
lifecycles would require an additional execution implementation. No future
singleton-buff or arbitrary scripting policy is implied by these initial rules.

## Completion and checks

All four milestones are implemented. The normal debug build passes all 258
CTest cases; the Ruby generator passes 29 examples. Generated content and
documentation freshness checks also pass. Coverage includes discovery across
accounts, attract exclusion, hidden slots, navigation, rule precedence and
probability boundaries, buff coverage, combo target identity, consumed-status
cleanup, canceled prerequisites, and turn completion.

The full run exposed a burn test that advanced unrelated random combat while
asserting exact burn damage. It now advances only status scheduler time and
passes 20 repeated runs. Discovery persistence and authoring policies for the
remaining monsters are subsequent content/save-system work.
