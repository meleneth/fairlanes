# frozen_string_literal: true

RSpec.describe FairlanesContent::Validator do
  it "accepts a valid declaration set" do
    declarations = build(:declaration_set)

    expect(described_class.new(declarations).validate).to be_empty
  end

  it "rejects blank monster descriptions" do
    declarations = build(:declaration_set)
    declarations.monsters.first.description = "   "

    expect(described_class.new(declarations).validate)
      .to include("monster field_mouse is missing a description")
  end

  it "rejects monster references to unknown skills" do
    declarations = build(:declaration_set)
    declarations.monster :honey_badger,
                         cycle: :origin,
                         cpp_id: "HoneyBadger",
                         display: "Honey Badger",
                         hp: 500,
                         known_skills: %i[eviscerate],
                         pool: :rare_woodland

    expect(described_class.new(declarations).validate)
      .to include("monster honey_badger references unknown skill eviscerate")
  end

  it "rejects flee success on non-flee skills" do
    declarations = FairlanesContent::DeclarationSet.new
    declarations.skill :thump,
                       cpp_id: "Thump",
                       display: "Thump",
                       learn_chance_percent: 20,
                       random_combat: true,
                       flee_success_percent: 10,
                       execution: :thump_like,
                       tags: %i[physical blunt melee],
                       declarative_shape: :handwritten_behavior

    expect(described_class.new(declarations).validate)
      .to include("skill thump has flee success but is not a flee execution")
  end

  it "rejects duplicate random combat entries" do
    declarations = build(:declaration_set)
    declarations.random_combat_skills :thump, :thump

    expect(described_class.new(declarations).validate)
      .to include("random combat skill order has duplicate entries")
  end

  it "rejects duplicate skill C++ ids" do
    declarations = FairlanesContent::DeclarationSet.new
    2.times do |index|
      declarations.skill :"thump_#{index}",
                         cpp_id: "Thump",
                         display: "Thump",
                         learn_chance_percent: 20,
                         execution: :thump_like,
                         tags: %i[physical blunt melee],
                         declarative_shape: :handwritten_behavior
    end

    expect(described_class.new(declarations).validate)
      .to include("duplicate skill C++ ids")
  end

  it "rejects duplicate monster C++ ids" do
    declarations = build(:declaration_set)
    declarations.monster :other_mouse,
                         cycle: :origin,
                         cpp_id: "FieldMouse",
                         hp: 5,
                         known_skills: %i[thump],
                         pool: :common_woodland

    expect(described_class.new(declarations).validate)
      .to include("duplicate monster C++ ids")
  end

  it "rejects duplicate status C++ ids" do
    declarations = build(:declaration_set)
    declarations.status :bleeding,
                        cpp_id: "Poison",
                        palette_index: 6

    expect(described_class.new(declarations).validate)
      .to include("duplicate status C++ ids")
  end

  it "rejects invalid status metadata" do
    declarations = build(:declaration_set)
    declarations.status :bad_status,
                        display: "",
                        debug_name: "",
                        component: "",
                        palette_index: 32

    expect(described_class.new(declarations).validate)
      .to include(
        "status bad_status is missing a display name",
        "status bad_status is missing a debug name",
        "status bad_status is missing a component",
        "status bad_status has invalid palette index 32"
      )
  end

  it "rejects invalid monster stats" do
    declarations = build(:declaration_set)
    declarations.monster :bad_mouse,
                         cycle: :origin,
                         hp: 0,
                         mp: -1,
                         level: 0,
                         known_skills: %i[thump],
                         pool: :common_woodland

    expect(described_class.new(declarations).validate)
      .to include(
        "monster bad_mouse has invalid hp 0",
        "monster bad_mouse has invalid mp -1",
        "monster bad_mouse has invalid level 0"
      )
  end

  it "rejects unknown tags" do
    declarations = FairlanesContent::DeclarationSet.new
    declarations.skill :thump,
                       cpp_id: "Thump",
                       display: "Thump",
                       learn_chance_percent: 20,
                       random_combat: true,
                       execution: :thump_like,
                       tags: %i[physical bogus],
                       declarative_shape: :handwritten_behavior

    expect(described_class.new(declarations).validate)
      .to include("skill thump has invalid tag bogus")
  end
  it "validates ordered rule skill, probability, and status conditions" do
    declarations = build(:declaration_set)
    declarations.monster_rules :field_mouse,
      {skill: :thump, target: :enemy, chance_percent: 30,
       conditions: [{predicate: :has_status, status: :burn},
                    {subject: :actor, predicate: :hp_below, percent: 50}]}
    expect(described_class.new(declarations).validate).to be_empty
    declarations.monsters.first.decision_rules.first[:chance_percent] = 101
    expect(described_class.new(declarations).validate).to include(/invalid chance/)
    declarations.monsters.first.decision_rules.first[:skill] = :missing
    expect(described_class.new(declarations).validate).to include(/unknown or unequipped skill/)
    declarations.monsters.first.decision_rules.first[:conditions][0][:status] = :imaginary
    expect(described_class.new(declarations).validate).to include(/invalid status/)
  end

  it "rejects malformed rule collections and misspelled fields" do
    declarations = build(:declaration_set)
    declarations.monsters.first.decision_rules = nil
    expect(described_class.new(declarations).validate).to include(/must be an array/)
    declarations.monster_rules :field_mouse, {skill: :thump, chance: 30}
    expect(described_class.new(declarations).validate).to include(/unknown fields/)
  end

  it "requires a recognized cycle on every monster" do
    declarations = build(:declaration_set)
    [nil, :industrial, "origin"].each do |cycle|
      declarations.monsters.first.cycle = cycle
      expect(described_class.new(declarations).validate)
        .to include("monster field_mouse has invalid cycle #{cycle.inspect}")
    end
    FairlanesContent::CYCLES.each do |cycle|
      declarations.monsters.first.cycle = cycle
      expect(described_class.new(declarations).validate).to be_empty
    end
  end

  it "accepts raid-only monsters and validates configured group damage" do
    declarations = build(:declaration_set)
    declarations.monsters.first.pool = :raid
    skill = declarations.skills.first
    skill.execution = :group_damage
    skill.effect_damage = 80
    expect(described_class.new(declarations).validate).to be_empty
    skill.effect_damage = -1
    expect(described_class.new(declarations).validate).to include("skill thump has invalid group damage")
  end

end
