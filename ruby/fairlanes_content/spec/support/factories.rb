# frozen_string_literal: true

FactoryBot.define do
  factory :declaration_set, class: "FairlanesContent::DeclarationSet" do
    initialize_with { new }

    after(:build) do |set|
      set.visual :shock, cpp: "Shock"
      set.status :poison,
                 cpp_id: "Poison",
                 display: "Poison",
                 component: "Poison",
                 palette_index: 20
      set.skill :thump,
                cpp_id: "Thump",
                display: "Thump",
                learn_chance_percent: 20,
                execution: :thump_like,
                tags: %i[physical blunt melee],
                declarative_shape: :handwritten_behavior
      set.random_combat_skills :thump
      set.monster :field_mouse,
                  cpp_id: "FieldMouse",
                  display: "Field Mouse",
                  hp: 5,
                  known_skills: %i[thump],
                  pool: :common_woodland
    end
  end
end
