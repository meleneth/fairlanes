# frozen_string_literal: true

require "json"

RSpec.describe "Gear and tower declarations" do
  def tower_set
    build(:declaration_set).tap do |set|
      set.monster :honey_badger, cycle: :origin, hp: 500,
                  description: "A stubborn test foe.", known_skills: %i[thump],
                  pool: :rare_woodland, resistances: {fire: 25}
      set.gear :damp_boots, display: "Damp Boots", slot: :boots,
               armor_kind: :leather, tier: :excellent, weight: 20,
               modifiers: {resistances: {lightning: 20}}
      set.encounter_pool :opening, lineups: [Array.new(5, :field_mouse)]
      set.encounter_pool :deep, lineups: [Array.new(5, :field_mouse)]
      set.biome :mouse_tower, display: "Mouse Tower", floors: 100,
                bands: [{from: 1, to: 49, pool: :opening},
                        {from: 50, to: 100, pool: :deep}],
                overrides: [{floor: 10, lineup: [:honey_badger] + Array.new(4, :field_mouse)}]
    end
  end

  it "validates supported gear and a fully covered tower" do
    set = tower_set
    expect(FairlanesContent::Validator.new(set).validate).to be_empty
    manifest = JSON.parse(FairlanesContent::Renderer.new(set).manifest)
    expect(manifest.fetch("gear").first.fetch("id")).to eq("damp_boots")
    expect(manifest.fetch("gear").first.fetch("modifiers").fetch("resistances")).to eq("lightning" => 20)
    expect(manifest.fetch("monsters").last.fetch("resistances")).to eq("fire" => 25)
    expect(FairlanesContent::Renderer.new(set).gear_content_hpp).to include("Resistances{0, 0, 0, 0, 20}")
    floors = manifest.fetch("biomes").first.fetch("resolved_floors")
    expect(floors.size).to eq(100)
    expect(floors[9]).to include("id" => "mouse_tower/010", "source" => "override")
    expect(floors[9].fetch("lineup").first).to eq("honey_badger")
    expect(floors[49].fetch("source")).to eq("deep")
  end

  it "supports a single-monster authored floor" do
    set = tower_set
    set.biomes.first.overrides << FairlanesContent::FloorOverride.new(
      floor: 1, lineup: [:field_mouse])
    expect(FairlanesContent::Validator.new(set).validate).to be_empty
    header = FairlanesContent::Renderer.new(set).floor_content_hpp
    expect(header).to include('FloorLineup{1, "mouse_tower/001", {fl::monster::MonsterKind::FieldMouse}, {5}, {5}, {5}, 1}')
  end

  it "generates a five-floor composition pattern and raises scaling every five floors" do
    set = tower_set
    set.tower_pattern :mouse_climb,
                      monsters: %i[field_mouse honey_badger]
    set.biomes.clear
    set.biome :mouse_tower, display: "Mouse Tower", floors: 100,
              pattern: :mouse_climb
    expect(FairlanesContent::Validator.new(set).validate).to be_empty
    floors = FairlanesContent::FloorResolver.new(set).resolve(set.biomes.first)
    expect(floors[0]).to include(lineup: Array.new(5, :field_mouse),
                                 level_percent: 5, hp_percent: 5,
                                 attack_percent: 5)
    expect(floors[4].fetch(:lineup)).to eq(
      %i[field_mouse honey_badger honey_badger honey_badger honey_badger])
    expect(floors[5]).to include(level_percent: 10, hp_percent: 10,
                                 attack_percent: 10)
    expect(floors[99]).to include(level_percent: 100, hp_percent: 100,
                                  attack_percent: 100)
  end

  it "rejects unknown resistance channels and percentages outside 0..100" do
    set = tower_set
    set.monsters.last.resistances = {acid: 10, fire: 101}
    set.gears.first.modifiers = {resistances: {lightning: -1}}
    expect(FairlanesContent::Validator.new(set).validate).to include(
      /unknown channel acid/, /fire must be an integer from 0 to 100/,
      /lightning must be an integer from 0 to 100/)
  end

  it "keeps floors outside an edited band unchanged" do
    set = tower_set
    first = FairlanesContent::FloorResolver.new(set).resolve(set.biomes.first)
    set.encounter_pools.last.lineups << Array.new(5, :field_mouse)
    second = FairlanesContent::FloorResolver.new(set).resolve(set.biomes.first)
    expect(second.first(49)).to eq(first.first(49))
    expect(second[9][:source]).to eq("override")
  end

  it "rejects duplicate ids, overrides, and unsupported modifiers" do
    set = tower_set
    set.gear :damp_boots, display: "Other", slot: :boots,
             armor_kind: :leather, tier: :fine, weight: 1,
             modifiers: {strength: 2}
    set.biomes.first.overrides << FairlanesContent::FloorOverride.new(
      floor: 10, lineup: Array.new(5, :field_mouse))
    set.encounter_pool :opening, lineups: [Array.new(5, :field_mouse)]
    set.biome :mouse_tower, display: "Other Mouse Tower", floors: 100,
              bands: [{from: 1, to: 100, pool: :deep}]
    expect(FairlanesContent::Validator.new(set).validate).to include(
      "duplicate gear ids", "duplicate encounter pool ids", "duplicate biome ids",
      "gear damp_boots has unsupported stat modifiers",
      "biome mouse_tower has duplicate override 10")
  end

  it "rejects unknown references, overlapping bands, range errors, and gaps" do
    set = tower_set
    set.encounter_pools.first.lineups << Array.new(5, :missing_mouse)
    set.biomes.first.bands << FairlanesContent::FloorBand.new(from: 49, to: 50, pool: :unknown)
    set.biomes.first.overrides << FairlanesContent::FloorOverride.new(
      floor: 101, lineup: Array.new(5, :field_mouse))
    set.biomes.first.bands << FairlanesContent::FloorBand.new(from: 0, to: 1, pool: :opening)
    errors = FairlanesContent::Validator.new(set).validate
    expect(errors).to include(/unknown monster missing_mouse/, /unknown pool unknown/,
                              /overlapping bands at floor 49/, /out-of-range override 101/,
                              /out-of-range band 0..1/)

    set.biomes.first.bands.pop
    set.biomes.first.bands.pop
    set.biomes.first.bands.pop
    expect(FairlanesContent::Validator.new(set).validate).to include(/missing floor coverage/)
  end
end
