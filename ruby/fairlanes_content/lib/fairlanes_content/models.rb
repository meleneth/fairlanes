# frozen_string_literal: true

module FairlanesContent
  Skill = Struct.new(
    :id, :cpp_id, :display, :learn_chance_percent, :random_combat,
    :flee_success_percent, :execution, :visual, :tags, :declarative_shape,
    keyword_init: true
  )

  Monster = Struct.new(
    :id, :cpp_id, :display, :hp, :mp, :level, :known_skills, :pool,
    keyword_init: true
  )

  Status = Struct.new(
    :id, :cpp_id, :display, :debug_name, :component, :palette_index,
    keyword_init: true
  )
end
