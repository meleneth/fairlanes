# frozen_string_literal: true

require "spec_helper"

RSpec.describe "Ruby regression and biome plans" do
  let(:declarations) do
    FairlanesContent::DeclarationSet.load_file(
      File.expand_path("../../../scripts/fairlanes_content_declarations.rb", __dir__))
  end

  it "loads the early milestone, longitudinal Brace sequence, and floor-fifty baseline" do
    expect(FairlanesContent::Validator.new(declarations).validate).to be_empty
    scenario = declarations.regressions.find { |entry| entry.id == :origin_woodland_floor10 }
    expect(scenario.encounter).to eq(biome: :origin_woodland_tower, floor: 10)
    expect(scenario.encounters).to eq([{biome: :origin_woodland_tower, floor: 10}])
    expect(scenario.party.size).to eq(2)
    expect(scenario.variants.map { |variant| variant.fetch(:equipped_item) }).to eq(
      %i[village_guard_hide village_guard_hide])
    progression = declarations.regressions.find do |entry|
      entry.id == :origin_woodland_brace_progression
    end
    expect(progression.encounters.map { |entry| entry.fetch(:floor) }).to eq([7, 8])
    expect(progression.recording_version).to eq(2)
    expect(progression.variants.map { |variant| variant[:next_skill] }).to eq(%i[brace thump])
    honey = declarations.monsters.find { |entry| entry.id == :honey_badger }
    expect(honey.pool).to eq(:authored_only)
    woodland = declarations.biomes.find { |entry| entry.id == :origin_woodland_tower }
    floors = FairlanesContent::FloorResolver.new(declarations).resolve(woodland)
    expect(floors[9].fetch(:lineup)).to eq(%i[antler_stag bark_smack bumpkin_hare])
    expect(floors[49].fetch(:lineup)).to eq(%i[honey_badger])
    expect(floors.first(49).flat_map { |floor| floor.fetch(:lineup) }).not_to include(:honey_badger)
    table = FairlanesContent::Renderer.new(declarations).regression_content_hpp
    expect(table).to include("origin_woodland_floor10", "VillageGuardHide", "kSeeds0",
                             "origin_woodland_brace_progression", "kEncounters1",
                             "SkillId::Brace")
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
    scenario.variants.first[:next_skill] = :unknown_skill
    scenario.variants.last[:id] = scenario.variants.first[:id]
    errors = FairlanesContent::Validator.new(declarations).validate
    expect(errors).to include("regression origin_woodland_floor10 references unknown gear unknown_cape",
                              "regression origin_woodland_floor10 references unknown next skill unknown_skill",
                              "regression origin_woodland_floor10 has duplicate variants")
  end
end
