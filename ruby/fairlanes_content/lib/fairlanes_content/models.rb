# frozen_string_literal: true

module FairlanesContent
  Skill = Struct.new(
    :id, :cpp_id, :display, :learn_chance_percent, :random_combat,
    :flee_success_percent, :execution, :visual, :tags, :declarative_shape, :description, :consumes_status, :effect_damage,
    keyword_init: true
  )

  Monster = Struct.new(
    :id, :cpp_id, :display, :hp, :mp, :level, :known_skills, :pool, :decision_rules, :description,
    keyword_init: true
  )

  Status = Struct.new(
    :id, :cpp_id, :display, :debug_name, :component, :palette_index,
    keyword_init: true
  )
end
