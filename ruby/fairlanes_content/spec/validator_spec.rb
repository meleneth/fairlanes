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
