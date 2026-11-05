# frozen_string_literal: true

require "spec_helper"

RSpec.describe "Origin recruitment progression" do
  let(:declarations) do
    FairlanesContent::DeclarationSet.load_file(
      File.expand_path("../../../scripts/fairlanes_content_declarations.rb", __dir__))
  end

  it "declares all twenty milestones and five party-five opportunities" do
    entries = declarations.recruitment_opportunities
    milestones = entries.select(&:biome)
    completion = entries.reject(&:biome)
    expect(entries.size).to eq(25)
    expect(milestones.group_by(&:floor).transform_values(&:size)).to eq(
      5 => 5, 25 => 5, 50 => 5, 75 => 5)
    expect(milestones.group_by(&:floor).transform_values do |group|
      group.map(&:destination_party).uniq
    end).to eq(5 => [1], 25 => [2], 50 => [3], 75 => [4])
    expect(completion.map(&:destination_party).uniq).to eq([5])
    expect(completion.map(&:unlocked_by_party_completion).uniq).to eq([4])
    expect(FairlanesContent::Validator.new(declarations).validate).to be_empty
  end

  it "generates stable runtime identities and review documentation" do
    renderer = FairlanesContent::Renderer.new(declarations)
    expect(renderer.recruitment_content_hpp).to include(
      "origin_woodland_floor_5_recruit", "Trigger::TowerMilestone",
      "origin_party_five_recruit_5", "Trigger::PartyCompleted")
    expect(renderer.recruitment_progression_doc).to include(
      "`origin_desert_floor_75_recruit`", "party 4 roster completed")
  end

  it "rejects incomplete schedules and invalid destinations" do
    entries = declarations.recruitment_opportunities.map(&:dup)
    entries.pop
    entries.first.destination_party = 6
    declarations.instance_variable_set(:@recruitment_opportunities, entries)
    errors = FairlanesContent::Validator.new(declarations).validate
    expect(errors).to include(
      "recruitment opportunity origin_woodland_floor_5_recruit has invalid destination party",
      "Origin party five must have five completion opportunities")
  end
end
