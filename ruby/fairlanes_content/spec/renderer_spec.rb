# frozen_string_literal: true

require "json"

RSpec.describe FairlanesContent::Renderer do
  it "renders a parseable manifest" do
    declarations = build(:declaration_set)

    manifest = JSON.parse(described_class.new(declarations).manifest)

    expect(manifest.fetch("schema_version")).to eq(1)
    expect(manifest.fetch("statuses").first.fetch("id")).to eq("poison")
    expect(manifest.fetch("skills").first.fetch("id")).to eq("thump")
    expect(manifest.fetch("monsters").first.fetch("known_skills")).to eq(["thump"])
  end
  it "reports every cycle including empty cycles and serializes membership" do
    declarations = build(:declaration_set)
    declarations.monsters.first.cycle = :conflict
    renderer = described_class.new(declarations)

    expect(JSON.parse(renderer.manifest).fetch("monsters").first.fetch("cycle")).to eq("conflict")
    expect(renderer.balance_report).to include("| Origin | 0 |", "| Resonance | 0 |",
                                               "| Conflict | 1 |", "| Singularity | 0 |")
    expect(renderer.monster_entry_cpp(declarations.monsters.first)).to include("fl::primitives::Cycle::Conflict")
  end

end
