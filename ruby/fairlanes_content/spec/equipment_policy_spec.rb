# frozen_string_literal: true

RSpec.describe "Declarative equipment and production policy" do
  let(:source) do
    File.expand_path("../../../scripts/fairlanes_content_declarations.rb", __dir__)
  end

  def mutable_declarations(source)
    FairlanesContent::DeclarationSet.new.tap do |declarations|
      declarations.instance_eval(File.read(source), source)
    end
  end

  it "generates extensible material and recipe IDs independent of declaration order" do
    declarations = mutable_declarations(source)
    expect(FairlanesContent::Validator.new(declarations).validate).to be_empty
    header = FairlanesContent::Renderer.new(declarations).production_ids_hpp
    material_line = header.lines.find { |line| line.include?("WoodlandThread =") }
    recipe_line = header.lines.find { |line| line.include?("ThreadedGloves =") }
    expect(material_line).to include("0x")
    expect(recipe_line).to include("0x")

    declarations.materials.rotate!
    declarations.recipes.rotate!
    reordered = FairlanesContent::Renderer.new(declarations).production_ids_hpp
    expect(reordered).to include(material_line, recipe_line)
  end

  it "renders policy changes into typed C++ without changing engine code" do
    declarations = mutable_declarations(source)
    original = FairlanesContent::Renderer.new(declarations).equipment_policy_hpp
    attack = declarations.equipment_stat_policies.find { |entry| entry.stat == :attack }
    attack.combat_value_coefficient = 9
    changed = FairlanesContent::Renderer.new(declarations).equipment_policy_hpp
    expect(original).to include("combat_value_coefficient = 6")
    expect(changed).to include("combat_value_coefficient = 9")
    expect(changed).not_to eq(original)
  end

  it "rejects incomplete, unknown, and malformed equipment policy" do
    declarations = mutable_declarations(source)
    declarations.equipment_tier_policies.pop
    declarations.equipment_stat_policies.first.budget_cost = 0
    declarations.equipment_slot_policies.first.weights[:imaginary] = 1
    errors = FairlanesContent::Validator.new(declarations).validate
    expect(errors).to include(a_string_matching(/equipment tier policy is missing mythic/),
                              a_string_matching(/attack has invalid budget cost/),
                              a_string_matching(/unknown stats imaginary/))
  end

  it "rejects unknown production references before rendering C++" do
    declarations = mutable_declarations(source)
    declarations.recipes.first.inputs = {missing_reagent: 1}
    expect(FairlanesContent::Validator.new(declarations).validate)
      .to include("recipe healing_potion has invalid inputs")
  end
end
