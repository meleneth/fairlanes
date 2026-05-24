# Enemies

Current enemy roster, mapped known combat skill, and base archetype stats.

| Enemy | MonsterKind | HP | Level | Known Skill |
| --- | --- | ---: | ---: | --- |
| Field Mouse | FieldMouse | 5 | not set | Thump |
| Honey Badger | HoneyBadger | 500 | not set | Eviscerate |
| Bumpkin Hare | BumpkinHare | 7 | 2 | Bump |
| Mire Squish | MireSquish | 9 | 3 | Squish |
| Bark Smack | BarkSmack | 12 | 4 | Smack |
| Poison Toad | PoisonToad | 16 | 5 | Poison |
| Yeti | Yeti | 40 | not set | Cold Snap |
| Salamander | Salamander | 24 | 6 | Flame Strike |
| Fire Drake | FireDrake | 500 | 8 | Flame Wave |
| Stormtick Imp | StormtickImp | 10 | 3 | Joltspasm |
| Ceiling Grudge | CeilingGrudge | 18 | 5 | Rocks Fall |
| Miasma Toad | MiasmaToad | 15 | 4 | Sour Breath |
| Choir Wisp | ChoirWisp | 12 | 4 | Mercyburst |
| Gorecap Sprout | GorecapSprout | 16 | 5 | Blood Bloom |
| Rimefang Hare | RimefangHare | 13 | 4 | Ice Splitter |
| Null Mote | NullMote | 14 | 5 | Gravity Sigh |

## Sources

- Monster list: src/fl/monsters/monster_kind.hpp
- Enemy->skill mapping: src/fl/monsters/monster_skills.cpp
- Stats and display names: monster archetypes in src/fl/monsters/*.cpp
