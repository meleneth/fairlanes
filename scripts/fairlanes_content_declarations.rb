# frozen_string_literal: true

# Declarative content source for generated C++ metadata, parity tests, and
# review docs. Handwritten C++ remains the authority for behavior.

visual :flame_wave
visual :shock
visual :rocks_fall
visual :poison_cloud
visual :holy_nova
visual :blood_bloom
visual :frost_crack
visual :void_ripple

skill :observe,
      learn_chance_percent: 0,
      random_combat: false,
      execution: :observe,
      tags: %i[observe utility],
      declarative_shape: :handwritten_behavior

skill :flee,
      learn_chance_percent: 0,
      flee_success_percent: 65,
      execution: :flee,
      tags: %i[utility escape],
      declarative_shape: :handwritten_behavior

skill :thump,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt melee],
      declarative_shape: :handwritten_behavior

skill :eviscerate,
      learn_chance_percent: 2,
      execution: :eviscerate,
      tags: %i[physical slashing bleed melee],
      declarative_shape: :handwritten_behavior

skill :poison,
      learn_chance_percent: 5,
      execution: :poison,
      tags: %i[poison disease],
      declarative_shape: :handwritten_behavior

skill :cold_snap,
      learn_chance_percent: 5,
      execution: :cold_snap,
      tags: %i[cold control],
      declarative_shape: :handwritten_behavior

skill :flame_strike,
      learn_chance_percent: 5,
      execution: :flame_strike,
      visual: :flame_wave,
      tags: %i[fire projectile],
      declarative_shape: :handwritten_behavior

skill :flame_wave,
      learn_chance_percent: 2,
      execution: :flame_wave,
      visual: :flame_wave,
      tags: %i[fire area],
      declarative_shape: :handwritten_behavior

decal_skill :joltspasm,
            visual: :shock,
            tags: %i[lightning control]

decal_skill :rocks_fall,
            visual: :rocks_fall,
            tags: %i[physical earth blunt area]

decal_skill :sour_breath,
            visual: :poison_cloud,
            tags: %i[acid disease area]

decal_skill :mercyburst,
            visual: :holy_nova,
            tags: %i[healing holy area]

decal_skill :blood_bloom,
            visual: :blood_bloom,
            tags: %i[bleed healing area]

decal_skill :ice_splitter,
            visual: :frost_crack,
            tags: %i[cold piercing projectile]

decal_skill :gravity_sigh,
            visual: :void_ripple,
            tags: %i[gravity control area]

skill :bump,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt melee],
      declarative_shape: :handwritten_behavior

skill :squish,
      learn_chance_percent: 20,
      execution: :thump_like,
      tags: %i[physical blunt control melee],
      declarative_shape: :handwritten_behavior

skill :smack,
      learn_chance_percent: 20,
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
        hp: 5,
        known_skills: %i[thump],
        pool: :common_woodland

monster :honey_badger,
        hp: 500,
        known_skills: %i[eviscerate],
        pool: :rare_woodland

monster :bumpkin_hare,
        hp: 7,
        level: 2,
        known_skills: %i[bump],
        pool: :common_woodland

monster :scaredy_cat,
        hp: 6,
        level: 2,
        known_skills: %i[flee thump],
        pool: :common_woodland

monster :mire_squish,
        hp: 9,
        level: 3,
        known_skills: %i[squish],
        pool: :common_woodland

monster :bark_smack,
        hp: 12,
        level: 4,
        known_skills: %i[smack],
        pool: :common_woodland

monster :poison_toad,
        hp: 16,
        level: 5,
        known_skills: %i[poison],
        pool: :common_woodland

monster :yeti,
        hp: 40,
        known_skills: %i[cold_snap],
        pool: :common_woodland

monster :salamander,
        hp: 24,
        level: 6,
        known_skills: %i[flame_strike],
        pool: :common_woodland

monster :fire_drake,
        hp: 500,
        level: 8,
        known_skills: %i[flame_wave],
        pool: :rare_woodland

monster :stormtick_imp,
        hp: 10,
        level: 3,
        known_skills: %i[joltspasm],
        pool: :common_woodland

monster :ceiling_grudge,
        hp: 18,
        level: 5,
        known_skills: %i[rocks_fall],
        pool: :common_woodland

monster :miasma_toad,
        hp: 15,
        level: 4,
        known_skills: %i[sour_breath],
        pool: :common_woodland

monster :choir_wisp,
        hp: 12,
        level: 4,
        known_skills: %i[mercyburst],
        pool: :common_woodland

monster :gorecap_sprout,
        hp: 16,
        level: 5,
        known_skills: %i[blood_bloom],
        pool: :common_woodland

monster :rimefang_hare,
        hp: 13,
        level: 4,
        known_skills: %i[ice_splitter],
        pool: :common_woodland

monster :null_mote,
        hp: 14,
        level: 5,
        known_skills: %i[gravity_sigh],
        pool: :common_woodland
