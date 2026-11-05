# Experience suppression candidates

> **Brainstorm — non-canonical.** No formula or threshold in this document is
> accepted design. Canonical XP intent lives in
> [`design/progression.md`](../design/progression.md).

The design requires strong overlevel suppression and a separate repetition
mechanism. Candidate models should keep base encounter XP, level-relative
suppression, repetition suppression, and level requirements independently
observable.

## Overlevel suppression questions

- Compare ratios, differences, bands, or encounter-rated difficulty?
- Use the character's level, party strength, highest member, or another measure?
- Apply suppression per character so mixed-level parties can support catch-up,
  or per party to resist power-leveling exploits?
- Should rewards taper continuously, fall through broad bands, or eventually
  reach zero?
- Should the cumulative XP obtainable from obsolete encounters be
  mathematically bounded, or is practical near-zero value sufficient?

## Repetition history questions

- Track exact floor clears, encounter identities, biome-wide activity, monster
  composition, or a combination?
- Store history per character, party, account, or save?
- How quickly should history decay or recover after progressing elsewhere?
- Should repeating a difficult floor while learning it remain valuable for a
  reasonable number of clears?
- Can alternating nearly identical floors bypass a floor-specific penalty? Can
  rotating parties or one low-level passenger bypass a party-relative penalty?

## Degenerate strategies to test

- Indefinite farming of the safest obsolete floor.
- Alternating two efficient floors to evade repetition history.
- Carrying one low-level recruit through trivial content with an overpowered
  party.
- Rotating parties or characters to reset penalties.
- Choosing a nominally high-level but mechanically harmless encounter.
- Exploiting summons, multi-enemy fights, retries, or partial completion to
  multiply base XP.

Future experiments should report outcomes and tradeoffs, not promote their
tested constants into canonical requirements.
