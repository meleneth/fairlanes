# frozen_string_literal: true

require "spec_helper"
require "rbconfig"
require_relative "../../../scripts/regression_harness"

RSpec.describe FairlanesRegression do
  def run_summary(seed:, outcome: "hero_victory", learned: true, cap: false)
    initial = [{"known_skills" => ["observe/1"], "loadout" => ["observe/1"]}]
    final_skills = learned ? ["observe/1", "brace/1"] : ["observe/1"]
    {
      "seed" => seed,
      "outcome" => cap ? "beat_cap" : outcome,
      "simulated_seconds" => 10.0 + seed,
      "remaining_party_hp" => 20 - seed,
      "survivors" => outcome == "hero_victory" ? 1 : 0,
      "initial_party_state" => initial,
      "party_state_before_encounters" => [initial, [{"known_skills" => final_skills}]],
      "final_party_state" => [{"known_skills" => final_skills, "loadout" => ["brace/1"]}],
      "encounters" => [{"brace_applications" => 1, "brace_consumptions" => 1,
                         "brace_mitigated_hits" => 1,
                         "brace_mitigated_damage" => 2}]
    }
  end

  let(:fixture) { {"id" => "brace_test"} }
  let(:variant) do
    {"id" => "learn", "expected_wins" => [1, 2],
     "assertions" => %w[brace_learned brace_retained brace_equipped brace_mitigated]}
  end

  it "separates declared correctness from aggregate balance observations" do
    report = described_class::Metrics.aggregate(fixture, variant,
                                                 [run_summary(seed: 1), run_summary(seed: 2)], [])
    expect(report.dig("correctness", "all_declared_assertions_passed")).to be(true)
    expect(report.dig("mechanics", "brace", "damage_prevented")).to eq(4)
    expect(report.fetch("seed_results").map { |entry| entry.fetch("seed") }).to eq([1, 2])
    expect(report.dig("correctness", "semantic_assertions").map { |entry| entry.fetch("id") }).to eq(
      %w[brace_learned brace_retained brace_equipped brace_mitigated])
  end

  it "fails only explicitly declared assertions and reports beat caps separately" do
    control = variant.merge("assertions" => ["brace_learned"])
    report = described_class::Metrics.aggregate(
      fixture, control, [run_summary(seed: 1, learned: false, cap: true)], [])
    expect(report.fetch("beat_caps")).to eq(1)
    expect(report.fetch("losses")).to eq(0)
    expect(report.dig("correctness", "win_assertion_passed")).to be(false)
    expect(report.dig("correctness", "semantic_assertions", 0, "passed")).to be(false)
  end

  it "reports missing and malformed input without dumping JSON" do
    Dir.mktmpdir do |dir|
      harness = described_class::Harness.new(root: dir)
      expect { harness.read_json(File.join(dir, "missing.json")) }.to raise_error(
        described_class::Error, /is missing/)
      malformed = File.join(dir, "malformed.json")
      File.write(malformed, "{")
      expect { harness.read_json(malformed) }.to raise_error(described_class::Error, /malformed JSON/)
    end
  end

  it "preserves failed command status and writes a focused complete log" do
    Dir.mktmpdir do |dir|
      log = File.join(dir, "failed.log")
      command = [RbConfig.ruby, "-e", 'warn "error: useful diagnostic"; exit 7']
      expect do
        described_class::CommandRunner.new.run(command, log_path: log, chdir: dir)
      end.to raise_error(described_class::Error) { |error|
        expect(error.status).to eq(7)
        expect(error.message).to include("error: useful diagnostic", "exit=7", "log=#{log}")
      }
      expect(File.read(log)).to include("error: useful diagnostic")
    end
  end

  it "filters focused mechanic timelines and rejects invalid mechanics" do
    Dir.mktmpdir do |dir|
      path = File.join(dir, "recording.json")
      File.write(path, JSON.generate(
                         "fixture" => "brace_test", "gameplay_seed" => 4,
                         "result" => {"outcome" => "hero_victory"},
                         "events" => [
                           {"seq" => 1, "uwu" => 80, "type" => "action", "skill" => "brace/1",
                            "encounter_index" => 0},
                           {"seq" => 2, "uwu" => 160, "type" => "dire_bleed_tick",
                            "encounter_index" => 1}
                         ]))
      harness = described_class::Harness.new(root: dir)
      report = harness.inspect_recording(path, encounter: 0, mechanic: "brace")
      expect(report.fetch("events").map { |event| event.fetch("seq") }).to eq([1])
      expect { harness.inspect_recording(path, mechanic: "bogus") }.to raise_error(
        described_class::Error, /unknown mechanic/)
    end
  end

  it "rejects unknown fixtures and variants through the generated catalog" do
    Dir.mktmpdir do |dir|
      path = File.join(dir, "manifest.json")
      File.write(path, JSON.generate("regressions" => [
                                     {"id" => "known", "variants" => [{"id" => "a"}]}
                                   ]))
      catalog = described_class::Catalog.new(path)
      expect { catalog.fixture("missing") }.to raise_error(described_class::Error, /unknown regression/)
      expect { catalog.variants(catalog.fixture("known"), ["b"]) }.to raise_error(
        described_class::Error, /unknown variant/)
    end
  end
end
