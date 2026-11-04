# frozen_string_literal: true

# The arrival setup is shared. A variant changes only the equipped cape.
# Outcomes remain exploratory until the generated combat runs establish a
# useful win-count range for both routes.
regression :origin_woodland_floor10,
  encounter: {biome: :origin_woodland_tower, floor: 10},
  party: Array.new(5) { {hp: 20, attack: 2, armor: 0, resistances: {},
                         skill: :thump, policy: :single_equipped_skill} },
  seeds: [1, 2, 3, 4, 5, 6, 7, 8],
  beat_cap: 2400,
  variants: [
    {id: :arrival, equipped_item: :mouse_nibbled_cape, expected_wins: 8..8},
    {id: :detour, equipped_item: :mossbound_cape, expected_wins: 8..8}
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
