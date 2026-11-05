# frozen_string_literal: true

# Floor ten compares an all-offense loadout with one hero selecting Brace.
# Both variants otherwise use the current two-member opening party setup.
regression :origin_woodland_floor10,
  encounter: {biome: :origin_woodland_tower, floor: 10},
  party: Array.new(2) { {hp: 20, attack: 2, armor: 0, resistances: {},
                         skill: :thump, policy: :single_equipped_skill} },
  seeds: [1, 2, 3, 4, 5, 6, 7, 8],
  beat_cap: 2400,
  recording_version: 2,
  variants: [
    {id: :offensive, equipped_item: :village_guard_hide, expected_wins: 8..8},
    {id: :defensive, equipped_item: :village_guard_hide,
     skill_overrides: {0 => :brace}, expected_wins: 2..2}
  ]

# This sequence keeps one production party alive across both authored floors.
# Brace is equipped after floor seven only if normal Observe learning retained it.
regression :origin_woodland_brace_progression,
  encounters: [
    {biome: :origin_woodland_tower, floor: 7},
    {biome: :origin_woodland_tower, floor: 8}
  ],
  party: [
    {hp: 20, attack: 2, armor: 1, resistances: {},
     skill: :thump, policy: :single_equipped_skill},
    {hp: 20, attack: 2, armor: 0, resistances: {},
     skill: :thump, policy: :single_equipped_skill}
  ],
  seeds: [1, 2, 3, 4, 5, 6, 7, 8],
  beat_cap: 3200,
  recording_version: 2,
  variants: [
    {id: :learn_brace, observe_members: [0], next_skill: :brace, expected_wins: 8..8},
    {id: :no_brace, next_skill: :thump, expected_wins: 8..8}
  ]

# Floor fifty is exploratory. Neither representative party has cleansing.
regression :origin_woodland_honey_badger,
  encounter: {biome: :origin_woodland_tower, floor: 50},
  party: [
    {hp: 20, attack: 2, armor: 1, resistances: {},
     skill: :thump, policy: :single_equipped_skill},
    {hp: 20, attack: 2, armor: 0, resistances: {},
     skill: :thump, policy: :single_equipped_skill}
  ],
  seeds: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12],
  beat_cap: 4000,
  recording_version: 2,
  variants: [
    {id: :offensive, expected_wins: 6..6},
    {id: :defensive, skill_overrides: {0 => :brace}, expected_wins: 0..0}
  ]

regression :origin_woodland_stag_lesson,
  encounter: {biome: :origin_woodland_tower, floor: 7},
  party: [
    {hp: 20, attack: 2, armor: 1, resistances: {},
     skill: :thump, policy: :single_equipped_skill},
    {hp: 20, attack: 2, armor: 0, resistances: {},
     skill: :thump, policy: :single_equipped_skill}
  ],
  seeds: [1],
  beat_cap: 1200,
  variants: [
    {id: :observe_equipped, observe_members: [0], expected_wins: 1..1},
    {id: :observe_unequipped, expected_wins: 1..1}
  ]

regression :origin_woodland_brace_practice,
  encounter: {biome: :origin_woodland_tower, floor: 8},
  party: [
    {hp: 20, attack: 2, armor: 1, resistances: {},
     skill: :thump, policy: :single_equipped_skill},
    {hp: 20, attack: 2, armor: 0, resistances: {},
     skill: :thump, policy: :single_equipped_skill}
  ],
  seeds: [1],
  beat_cap: 1600,
  variants: [
    {id: :braced, skill_overrides: {0 => :brace}, expected_wins: 1..1},
    {id: :unbraced, expected_wins: 1..1}
  ]

regression :origin_plains_cover_lesson,
  encounter: {biome: :origin_plains_tower, floor: 3},
  party: [
    {hp: 20, attack: 2, armor: 1, resistances: {},
     skill: :pebble_spit, policy: :single_equipped_skill},
    {hp: 20, attack: 2, armor: 0, resistances: {},
     skill: :pebble_spit, policy: :single_equipped_skill}
  ],
  seeds: [1],
  beat_cap: 1600,
  variants: [
    {id: :cover, expected_wins: 0..0}
  ]
