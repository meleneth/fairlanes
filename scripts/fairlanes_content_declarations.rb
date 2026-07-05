# frozen_string_literal: true

# Declarative mirror of handwritten C++ content. The C++ engine remains the
# runtime authority; these declarations generate parity tests and review docs.

visual :flame_wave, cpp: "FlameWave"
visual :shock, cpp: "Shock"
visual :rocks_fall, cpp: "RocksFall"
visual :poison_cloud, cpp: "PoisonCloud"
visual :holy_nova, cpp: "HolyNova"
visual :blood_bloom, cpp: "BloodBloom"
visual :frost_crack, cpp: "FrostCrack"
visual :void_ripple, cpp: "VoidRipple"

skill :observe,
      cpp_id: "Observe",
      display: "Observe",
      learn_chance_percent: 0,
      random_combat: false,
      execution: :observe,
      tags: %i[observe utility],
      declarative_shape: :handwritten_behavior

skill :flee,
      cpp_id: "Flee",
      display: "Flee",
      learn_chance_percent: 0,
      random_combat: true,
      flee_success_percent: 65,
      execution: :flee,
      tags: %i[utility escape],
      declarative_shape: :handwritten_behavior

skill :thump,
      cpp_id: "Thump",
      display: "Thump",
      learn_chance_percent: 20,
      random_combat: true,
      execution: :thump_like,
      tags: %i[physical blunt melee],
      declarative_shape: :handwritten_behavior

skill :eviscerate,
      cpp_id: "Eviscerate",
      display: "Eviscerate",
      learn_chance_percent: 2,
      random_combat: true,
      execution: :eviscerate,
      tags: %i[physical slashing bleed melee],
      declarative_shape: :handwritten_behavior

skill :poison,
      cpp_id: "Poison",
      display: "Poison",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :poison,
      tags: %i[poison disease],
      declarative_shape: :handwritten_behavior

skill :cold_snap,
      cpp_id: "ColdSnap",
      display: "Cold Snap",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :cold_snap,
      tags: %i[cold control],
      declarative_shape: :handwritten_behavior

skill :flame_strike,
      cpp_id: "FlameStrike",
      display: "Flame Strike",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :flame_strike,
      visual: :flame_wave,
      tags: %i[fire projectile],
      declarative_shape: :handwritten_behavior

skill :flame_wave,
      cpp_id: "FlameWave",
      display: "Flame Wave",
      learn_chance_percent: 2,
      random_combat: true,
      execution: :flame_wave,
      visual: :flame_wave,
      tags: %i[fire area],
      declarative_shape: :handwritten_behavior

skill :joltspasm,
      cpp_id: "Joltspasm",
      display: "Joltspasm",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :shock,
      tags: %i[lightning control],
      declarative_shape: :decal_strike

skill :rocks_fall,
      cpp_id: "RocksFall",
      display: "Rocks Fall",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :rocks_fall,
      tags: %i[physical earth blunt area],
      declarative_shape: :decal_strike

skill :sour_breath,
      cpp_id: "SourBreath",
      display: "Sour Breath",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :poison_cloud,
      tags: %i[acid disease area],
      declarative_shape: :decal_strike

skill :mercyburst,
      cpp_id: "Mercyburst",
      display: "Mercyburst",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :holy_nova,
      tags: %i[healing holy area],
      declarative_shape: :decal_strike

skill :blood_bloom,
      cpp_id: "BloodBloom",
      display: "Blood Bloom",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :blood_bloom,
      tags: %i[bleed healing area],
      declarative_shape: :decal_strike

skill :ice_splitter,
      cpp_id: "IceSplitter",
      display: "Ice Splitter",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :frost_crack,
      tags: %i[cold piercing projectile],
      declarative_shape: :decal_strike

skill :gravity_sigh,
      cpp_id: "GravitySigh",
      display: "Gravity Sigh",
      learn_chance_percent: 5,
      random_combat: true,
      execution: :decal_strike,
      visual: :void_ripple,
      tags: %i[gravity control area],
      declarative_shape: :decal_strike

skill :bump,
      cpp_id: "Bump",
      display: "Bump",
      learn_chance_percent: 20,
      random_combat: true,
      execution: :thump_like,
      tags: %i[physical blunt melee],
      declarative_shape: :handwritten_behavior

skill :squish,
      cpp_id: "Squish",
      display: "Squish",
      learn_chance_percent: 20,
      random_combat: true,
      execution: :thump_like,
      tags: %i[physical blunt control melee],
      declarative_shape: :handwritten_behavior

skill :smack,
      cpp_id: "Smack",
      display: "Smack",
      learn_chance_percent: 20,
      random_combat: true,
      execution: :thump_like,
      tags: %i[physical blunt melee],
      declarative_shape: :handwritten_behavior

random_combat_skills(
  :thump,
  :eviscerate,
  :poison,
  :cold_snap,
  :flame_strike,
  :flame_wave,
  :bump,
  :squish,
  :smack,
  :joltspasm,
  :rocks_fall,
  :sour_breath,
  :mercyburst,
  :blood_bloom,
  :ice_splitter,
  :gravity_sigh,
  :flee
)

monster :field_mouse,
        cpp_id: "FieldMouse",
        display: "Field Mouse",
        known_skills: %i[thump],
        pool: :common_woodland

monster :honey_badger,
        cpp_id: "HoneyBadger",
        display: "Honey Badger",
        known_skills: %i[eviscerate],
        pool: :rare_woodland

monster :bumpkin_hare,
        cpp_id: "BumpkinHare",
        display: "Bumpkin Hare",
        known_skills: %i[bump],
        pool: :common_woodland

monster :scaredy_cat,
        cpp_id: "ScaredyCat",
        display: "Scaredy Cat",
        known_skills: %i[flee thump],
        pool: :common_woodland

monster :mire_squish,
        cpp_id: "MireSquish",
        display: "Mire Squish",
        known_skills: %i[squish],
        pool: :common_woodland

monster :bark_smack,
        cpp_id: "BarkSmack",
        display: "Bark Smack",
        known_skills: %i[smack],
        pool: :common_woodland

monster :poison_toad,
        cpp_id: "PoisonToad",
        display: "Poison Toad",
        known_skills: %i[poison],
        pool: :common_woodland

monster :yeti,
        cpp_id: "Yeti",
        display: "Yeti",
        known_skills: %i[cold_snap],
        pool: :common_woodland

monster :salamander,
        cpp_id: "Salamander",
        display: "Salamander",
        known_skills: %i[flame_strike],
        pool: :common_woodland

monster :fire_drake,
        cpp_id: "FireDrake",
        display: "Fire Drake",
        known_skills: %i[flame_wave],
        pool: :rare_woodland

monster :stormtick_imp,
        cpp_id: "StormtickImp",
        display: "Stormtick Imp",
        known_skills: %i[joltspasm],
        pool: :common_woodland

monster :ceiling_grudge,
        cpp_id: "CeilingGrudge",
        display: "Ceiling Grudge",
        known_skills: %i[rocks_fall],
        pool: :common_woodland

monster :miasma_toad,
        cpp_id: "MiasmaToad",
        display: "Miasma Toad",
        known_skills: %i[sour_breath],
        pool: :common_woodland

monster :choir_wisp,
        cpp_id: "ChoirWisp",
        display: "Choir Wisp",
        known_skills: %i[mercyburst],
        pool: :common_woodland

monster :gorecap_sprout,
        cpp_id: "GorecapSprout",
        display: "Gorecap Sprout",
        known_skills: %i[blood_bloom],
        pool: :common_woodland

monster :rimefang_hare,
        cpp_id: "RimefangHare",
        display: "Rimefang Hare",
        known_skills: %i[ice_splitter],
        pool: :common_woodland

monster :null_mote,
        cpp_id: "NullMote",
        display: "Null Mote",
        known_skills: %i[gravity_sigh],
        pool: :common_woodland
