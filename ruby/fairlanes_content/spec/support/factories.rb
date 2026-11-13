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
      set.skill_scaling :thump,
                        affinity: :plate,
                        stat: :attack,
                        dimension: :damage,
                        numerator: 1,
                        cap: 40
      set.random_combat_skills :thump
      set.monster :field_mouse,
                  cycle: :origin,
                  cpp_id: "FieldMouse",
                  display: "Field Mouse",
                  description: "A small mouse with a determined thump.",
                  hp: 5,
                  known_skills: %i[thump],
                  pool: :common_woodland
      set.monster_affinities :field_mouse, :plate
      set.consumable :healing_potion, display: "Healing Potion", magnitude: 12
      set.consumable :adrenaline_potion, display: "Adrenaline Potion", magnitude: 6
      set.consumable :reflex_potion, display: "Reflex Potion", magnitude: 6
      set.consumable :attack_flask, display: "Attack Flask", magnitude: 3, duration_seconds: 120
      set.consumable :armor_flask, display: "Armor Flask", magnitude: 3, duration_seconds: 120
      set.consumable :tempo_flask, display: "Tempo Flask", magnitude: 20, duration_seconds: 120
    end
  end
end
