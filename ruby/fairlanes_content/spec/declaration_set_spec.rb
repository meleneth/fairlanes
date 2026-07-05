# frozen_string_literal: true

RSpec.describe FairlanesContent::DeclarationSet do
  it "defaults visual C++ ids from declaration ids" do
    declarations = described_class.new

    declarations.visual :flame_wave

    expect(declarations.visuals.fetch(:flame_wave)).to eq("FlameWave")
  end

  it "defaults skill C++ ids and display names from declaration ids" do
    declarations = described_class.new

    declarations.skill :cold_snap,
                       learn_chance_percent: 5,
                       execution: :cold_snap,
                       tags: %i[cold control],
                       declarative_shape: :handwritten_behavior

    skill = declarations.skills.fetch(0)
    expect(skill.cpp_id).to eq("ColdSnap")
    expect(skill.display).to eq("Cold Snap")
    expect(skill.random_combat).to be(true)
  end

  it "defaults monster C++ ids and display names from declaration ids" do
    declarations = described_class.new

    declarations.monster :field_mouse,
                         hp: 5,
                         known_skills: %i[thump],
                         pool: :common_woodland

    monster = declarations.monsters.fetch(0)
    expect(monster.cpp_id).to eq("FieldMouse")
    expect(monster.display).to eq("Field Mouse")
    expect(monster.hp).to eq(5)
    expect(monster.mp).to eq(0)
    expect(monster.level).to be_nil
  end

  it "defaults status metadata from declaration ids" do
    declarations = described_class.new

    declarations.status :dire_bleed,
                        palette_index: 6

    status = declarations.statuses.fetch(0)
    expect(status.cpp_id).to eq("DireBleed")
    expect(status.display).to eq("Dire Bleed")
    expect(status.debug_name).to eq("dire-bleed")
    expect(status.component).to eq("DireBleed")
    expect(status.palette_index).to eq(6)
  end

  it "allows explicit names to override defaults" do
    declarations = described_class.new

    declarations.skill :aoe,
                       cpp_id: "FlameWave",
                       display: "Flame Wave",
                       learn_chance_percent: 2,
                       execution: :flame_wave,
                       tags: %i[fire area],
                       declarative_shape: :handwritten_behavior

    skill = declarations.skills.fetch(0)
    expect(skill.cpp_id).to eq("FlameWave")
    expect(skill.display).to eq("Flame Wave")
  end

  it "allows skills to opt out of random combat" do
    declarations = described_class.new

    declarations.skill :observe,
                       learn_chance_percent: 0,
                       random_combat: false,
                       execution: :observe,
                       tags: %i[observe utility],
                       declarative_shape: :handwritten_behavior

    expect(declarations.skills.fetch(0).random_combat).to be(false)
  end

  it "declares decal skills with the standard decal strike shape" do
    declarations = described_class.new

    declarations.decal_skill :rocks_fall,
                             visual: :rocks_fall,
                             tags: %i[physical earth blunt area]

    skill = declarations.skills.fetch(0)
    expect(skill.learn_chance_percent).to eq(5)
    expect(skill.random_combat).to be(true)
    expect(skill.execution).to eq(:decal_strike)
    expect(skill.visual).to eq(:rocks_fall)
    expect(skill.declarative_shape).to eq(:decal_strike)
  end

  it "allows decal skill metadata to override defaults" do
    declarations = described_class.new

    declarations.decal_skill :void,
                             cpp_id: "GravitySigh",
                             display: "Gravity Sigh",
                             visual: :void_ripple,
                             tags: %i[gravity control area],
                             learn_chance_percent: 2,
                             random_combat: false

    skill = declarations.skills.fetch(0)
    expect(skill.cpp_id).to eq("GravitySigh")
    expect(skill.display).to eq("Gravity Sigh")
    expect(skill.learn_chance_percent).to eq(2)
    expect(skill.random_combat).to be(false)
  end
end
