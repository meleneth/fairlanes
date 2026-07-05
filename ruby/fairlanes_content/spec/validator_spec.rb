# frozen_string_literal: true

RSpec.describe FairlanesContent::Validator do
  it "accepts a valid declaration set" do
    declarations = build(:declaration_set)

    expect(described_class.new(declarations).validate).to be_empty
  end

  it "rejects monster references to unknown skills" do
    declarations = build(:declaration_set)
    declarations.monster :honey_badger,
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
end
