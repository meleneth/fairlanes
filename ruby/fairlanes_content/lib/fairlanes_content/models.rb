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
  FloorOverride = Struct.new(
    :floor, :lineup, :cover, :level_percent, :hp_percent, :attack_percent,
    keyword_init: true
  )
  Biome = Struct.new(:id, :display, :floors, :bands, :overrides, :pattern, keyword_init: true)
  RegressionScenario = Struct.new(:id, :encounter, :encounters, :party, :seeds, :beat_cap,
                                  :recording_version, :variants, keyword_init: true)
  BiomePlan = Struct.new(:id, :cycle, :order, :monsters, keyword_init: true)
  RecruitmentOpportunity = Struct.new(
    :id, :cycle, :biome, :floor, :destination_party, :unlocked_by_party_completion,
    keyword_init: true
  )
  StoryEvent = Struct.new(:id, :handled, keyword_init: true)
  Objective = Struct.new(:id, :title, :description, :revealed_by, :completed_by, keyword_init: true)
  Dialogue = Struct.new(:id, :location, :npc, :start, :nodes, keyword_init: true)
  AutoplayRoute = Struct.new(:id, :steps, keyword_init: true)
  OriginAutoRegression = Struct.new(:id, :seed, :clears, :expected_decisions, keyword_init: true)
end
