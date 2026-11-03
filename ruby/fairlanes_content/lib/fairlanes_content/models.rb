# frozen_string_literal: true

module FairlanesContent
  CYCLES = %i[origin resonance conflict singularity].freeze

  Skill = Struct.new(
    :id, :cpp_id, :display, :learn_chance_percent, :random_combat,
    :flee_success_percent, :execution, :visual, :tags, :declarative_shape, :description, :consumes_status, :effect_damage,
    keyword_init: true
  )

  Monster = Struct.new(
    :id, :cpp_id, :display, :hp, :level, :known_skills, :pool, :decision_rules, :description, :cycle, :resistances,
    keyword_init: true
  )

  Status = Struct.new(
    :id, :cpp_id, :display, :debug_name, :component, :palette_index,
    keyword_init: true
  )

  Gear = Struct.new(:id, :display, :slot, :armor_kind, :tier, :weight, :modifiers, keyword_init: true)
  EncounterPool = Struct.new(:id, :lineups, keyword_init: true)
  FloorBand = Struct.new(:from, :to, :pool, keyword_init: true)
  FloorOverride = Struct.new(:floor, :lineup, keyword_init: true)
  Biome = Struct.new(:id, :display, :floors, :bands, :overrides, keyword_init: true)
end
