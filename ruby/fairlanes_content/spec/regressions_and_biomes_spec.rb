# frozen_string_literal: true

require "spec_helper"

RSpec.describe "Ruby regression and biome plans" do
  let(:declarations) do
    FairlanesContent::DeclarationSet.load_file(
      File.expand_path("../../../scripts/fairlanes_content_declarations.rb", __dir__))
  end

  it "loads the shared floor-ten setup and both generated variants" do
    expect(FairlanesContent::Validator.new(declarations).validate).to be_empty
    scenario = declarations.regressions.find { |entry| entry.id == :origin_woodland_floor10 }
    expect(scenario.encounter).to eq(biome: :origin_woodland_tower, floor: 10)
    expect(scenario.party.size).to eq(5)
    expect(scenario.variants.map { |variant| variant.fetch(:equipped_item) }).to eq(
      %i[mouse_nibbled_cape mossbound_cape])
    table = FairlanesContent::Renderer.new(declarations).regression_content_hpp
    expect(table).to include("origin_woodland_floor10", "MossboundCape", "kSeeds0")
  end

  it "generates the twenty-tower map and reports both assignment gaps" do
    renderer = FairlanesContent::Renderer.new(declarations)
    expect(declarations.biome_plans.size).to eq(20)
    text = renderer.biome_plan_doc
    expect(text).to include("origin_plains_tower", "`grass_wolf` | `bite`, `pack_howl`, `claw`", "Current runtime pool")
    expect(text).to include("`visitor_herald` | `visitor_fall` | `raid`")
    expect(text).to include("## Declared skills without a monster", "- `observe`")
  end

  it "rejects invalid regression references and duplicate variants" do
    scenario = declarations.regressions.first
    scenario.variants.first[:equipped_item] = :unknown_cape
    scenario.variants.last[:id] = scenario.variants.first[:id]
    errors = FairlanesContent::Validator.new(declarations).validate
    expect(errors).to include("regression origin_woodland_floor10 references unknown gear unknown_cape",
                              "regression origin_woodland_floor10 has duplicate variants")
  end
end
