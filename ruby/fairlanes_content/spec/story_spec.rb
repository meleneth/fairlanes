# frozen_string_literal: true

require "spec_helper"

RSpec.describe "Story DSL" do
  let(:source) { File.expand_path("../../../scripts/fairlanes_content_declarations.rb", __dir__) }
  let(:declarations) { FairlanesContent::DeclarationSet.load_file(source) }

  it "generates typed objectives and dialogue choices" do
    expect(FairlanesContent::Validator.new(declarations).validate).to be_empty
    cpp = FairlanesContent::Renderer.new(declarations).story_content_hpp
    expect(cpp).to include("struct Objective", "struct Dialogue", "recruit_first_hero", "recruit_angry_fellow")
    expect(cpp).to include("struct OriginAutoRegression", "origin_woodland_opening")
  end

  it "reports dialogue events without handlers" do
    report = FairlanesContent::Renderer.new(declarations).story_event_doc
    expect(report).to include("## Dialogue events without handlers", "- `ask_about_visitor`")
    expect(report).not_to include("- `recruit_angry_fellow`")
    expect(report).to include("## Autoplay enabled routes", "`origin_opening`", "`buy_gear`")
    expect(report).to include("## Origin auto-progression regressions", "`origin_woodland_opening`")
  end

  it "rejects autoplay steps with unknown facts or dialogue choices" do
    set = FairlanesContent::DeclarationSet.new
    set.story_event :known
    set.autoplay_route :bad, steps: [
      { wait_for: :missing, action: :choose_dialogue, target: :nobody, choice: :nothing }
    ]
    expect(FairlanesContent::Validator.new(set).validate.join(" "))
      .to include("waits for unknown event", "unknown dialogue")
  end

  it "rejects unknown events and broken dialogue links" do
    set = FairlanesContent::DeclarationSet.new
    set.story_event :known
    set.dialogue :broken, location: :pub, npc: "Someone", start: :hello,
                          nodes: [{ id: :hello, speaker: "Someone", text: "Hi",
                                    choices: [{ id: :go, label: "Go", event: :missing, next: :nowhere }] }]
    errors = FairlanesContent::Validator.new(set).validate
    expect(errors.join(" ")).to include("emits unknown event", "unknown next node")
  end
end
