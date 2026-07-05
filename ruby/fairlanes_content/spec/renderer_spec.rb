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
end
