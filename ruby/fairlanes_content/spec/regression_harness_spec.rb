# frozen_string_literal: true

require "spec_helper"
require "rbconfig"
require_relative "../../../scripts/regression_harness"

RSpec.describe FairlanesRegression do
  def telemetry(duration: 10_000, changes: 2, hp_lost: 5, longest_idle: 5_000,
                attempted: 2, completed: 2, productive: 2, actors: nil, resources: nil)
    actors ||= [
      {"id" => "hero", "side" => "heroes", "attempted_actions" => attempted,
       "completed_actions" => completed, "productive_actions" => productive,
       "first_meaningful_action_uwu" => 4_800, "control_denied_uwu" => 0,
       "longest_control_denial_uwu" => 0},
      {"id" => "enemy", "side" => "enemies", "attempted_actions" => attempted,
       "completed_actions" => completed, "productive_actions" => productive,
       "first_meaningful_action_uwu" => 4_800, "control_denied_uwu" => 0,
       "longest_control_denial_uwu" => 0}
    ]
    {"schema" => "fairlanes-combat-telemetry", "version" => 1,
     "duration_uwu" => duration,
     "progress" => {"hp_lost" => hp_lost, "hp_recovered" => 0,
                    "other_state_changes" => 0, "meaningful_changes" => changes,
                    "longest_no_progress_uwu" => longest_idle},
     "actions" => {"attempted" => attempted, "completed" => completed,
                   "productive" => productive, "nonproductive" => completed - productive},
     "actors" => actors,
     "resources" => resources || {"available" => false, "reason" => "not instrumented"}}
  end

  def classifications(outcome: "hero_victory", **telemetry_options)
    described_class::Classifications.analyze_encounter(
      {"encounter_index" => 0, "outcome" => outcome,
       "telemetry" => telemetry(**telemetry_options)})
  end

  def run_summary(seed:, outcome: "hero_victory", learned: true, cap: false)
    initial = [{"known_skills" => ["observe/1"], "loadout" => ["observe/1"],
                "stats" => {"attack" => 3, "armor" => 2, "max_hp" => 20,
                            "resistances" => {"physical" => 0, "magical" => 0, "fire" => 0,
                                              "ice" => 0, "lightning" => 0}}}]
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
                         "brace_mitigated_damage" => 2,
                         "outgoing_damage" => 12, "incoming_damage" => 5}]
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
    expect(report.dig("combat", "mean_damage_dealt")).to eq(12.0)
    expect(report.dig("combat", "mean_effective_stats", "attack")).to eq(3.0)
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
        expect(error.message).to include("error: useful diagnostic", "Exit: 7", "Log: #{log}")
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
                         "regression_telemetry" => [
                           {"encounter_index" => 0, "outcome" => "hero_victory",
                            "telemetry" => telemetry}
                         ],
                         "events" => [
                           {"seq" => 1, "uwu" => 80, "type" => "action", "skill" => "brace/1",
                            "encounter_index" => 0},
                           {"seq" => 2, "uwu" => 160, "type" => "dire_bleed_tick",
                            "encounter_index" => 1}
                         ]))
      harness = described_class::Harness.new(root: dir)
      report = harness.inspect_recording(path, encounter: 0, mechanic: "brace")
      expect(report.fetch("events").map { |event| event.fetch("seq") }).to eq([1])
      focused = harness.inspect_recording(path, classification: "resource_exhaustion")
      expect(focused.fetch("evidence").first.fetch("status")).to eq("indeterminate")
      expect { harness.inspect_recording(path, mechanic: "bogus") }.to raise_error(
        described_class::Error, /unknown mechanic/)
      expect { harness.inspect_recording(path, classification: "bogus") }.to raise_error(
        described_class::Error, /unknown classification/)
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

  it "reports recruitment declarations through the concise harness entry point" do
    Dir.mktmpdir do |dir|
      manifest_dir = File.join(dir, "build-linux-debug/generated/fairlanes_content")
      FileUtils.mkdir_p(manifest_dir)
      entries = 20.times.map do |index|
        {"id" => "milestone_#{index}", "floor" => 5, "destination_party" => 1,
         "unlocked_by_party_completion" => nil}
      end
      entries.concat(5.times.map do |index|
        {"id" => "party_five_#{index}", "floor" => nil, "destination_party" => 5,
         "unlocked_by_party_completion" => 4}
      end)
      File.write(File.join(manifest_dir, "content_manifest.json"),
                 JSON.generate("regressions" => [], "recruitment_opportunities" => entries))
      runner = Object.new
      runner.define_singleton_method(:run) { |_command, **_options| "" }
      report = described_class::Harness.new(root: dir, runner: runner)
                                       .recruitment_report(build_first: false)
      expect(report.dig("milestones", "declared")).to eq(20)
      expect(report.dig("party_five_opportunities", "declared")).to eq(5)
      expect(report.fetch("checks").values).to all(be(true))
    end
  end

  it "leaves a normal short victory free of pathological classifications" do
    ids = classifications.map { |entry| [entry["id"], entry["status"]] }
    expect(ids).to eq([["resource_exhaustion", "indeterminate"]])
  end

  it "recognizes prolonged genuine progress as attrition rather than no progress" do
    entries = classifications(duration: 100_000, changes: 12, hp_lost: 40,
                              longest_idle: 20_000)
    expect(entries.map { |entry| entry["id"] }).to include("attrition")
    expect(entries.map { |entry| entry["id"] }).not_to include("no_progress")
  end

  it "distinguishes sustained control denial from temporary control and recovery" do
    locked = telemetry(duration: 60_000, actors: [
      {"id" => "hero", "side" => "heroes", "productive_actions" => 0,
       "longest_control_denial_uwu" => 40_000, "control_denied_uwu" => 40_000}
    ])
    temporary = telemetry(duration: 60_000, actors: [
      {"id" => "hero", "side" => "heroes", "productive_actions" => 2,
       "longest_control_denial_uwu" => 8_000, "control_denied_uwu" => 8_000}
    ])
    locked_ids = described_class::Classifications.analyze_encounter(
      {"outcome" => "enemy_victory", "telemetry" => locked}).map { |entry| entry["id"] }
    recovered_ids = described_class::Classifications.analyze_encounter(
      {"outcome" => "hero_victory", "telemetry" => temporary}).map { |entry| entry["id"] }
    expect(locked_ids).to include("control_lock")
    expect(recovered_ids).not_to include("control_lock")
  end

  it "reports beat-cap no progress without claiming a proven stalemate" do
    entries = classifications(outcome: "beat_cap", duration: 200_000, changes: 0,
                              hp_lost: 0, longest_idle: 200_000,
                              attempted: 0, completed: 0, productive: 0)
    expect(entries.map { |entry| [entry["id"], entry["status"]] }).to include(
      ["no_progress", "observed"])
    expect(entries.map { |entry| entry["id"] }).not_to include("stalemate_candidate")
  end

  it "marks a deterministic non-damaging action loop only as a stalemate candidate" do
    entries = classifications(outcome: "beat_cap", duration: 200_000, changes: 0,
                              hp_lost: 0, longest_idle: 200_000,
                              attempted: 5, completed: 5, productive: 0)
    candidate = entries.find { |entry| entry["id"] == "stalemate_candidate" }
    expect(candidate.fetch("status")).to eq("suspected")
    expect(candidate.dig("evidence", "limitation")).to include("not analyzed")
  end

  it "detects burst victory before meaningful retaliation" do
    actors = [
      {"id" => "hero", "side" => "heroes", "productive_actions" => 1,
       "longest_control_denial_uwu" => 0, "control_denied_uwu" => 0},
      {"id" => "enemy", "side" => "enemies", "productive_actions" => 0,
       "longest_control_denial_uwu" => 0, "control_denied_uwu" => 0}
    ]
    expect(classifications(duration: 20_000, actors: actors).map { |entry| entry["id"] }).to include(
      "burst_dominance")
  end

  it "keeps classifications independent across encounter boundaries" do
    run = {"encounters" => [
      {"encounter_index" => 0, "outcome" => "hero_victory", "telemetry" => telemetry},
      {"encounter_index" => 1, "outcome" => "beat_cap",
       "telemetry" => telemetry(duration: 200_000, changes: 0, hp_lost: 0,
                                longest_idle: 200_000, attempted: 4, completed: 4, productive: 0)}
    ]}
    entries = described_class::Classifications.analyze_run(run)
    candidate = entries.find { |entry| entry["id"] == "stalemate_candidate" }
    expect(candidate.dig("scope", "encounter_index")).to eq(1)
  end

  it "enforces required and forbidden declarations without failing observed-only differences" do
    observed = classifications(duration: 100_000, changes: 12, hp_lost: 40,
                               longest_idle: 20_000)
    expectations = described_class::Classifications.expectations(
      {"classification_expectations" => {"attrition" => "required",
                                           "control_lock" => "forbidden",
                                           "burst_dominance" => "allowed"}}, observed)
    expect(expectations).to all(include("passed" => true))
    failed = described_class::Classifications.expectations(
      {"classification_expectations" => {"attrition" => "forbidden"}}, observed)
    expect(failed.first.fetch("passed")).to be(false)
  end

  it "uses indeterminate classifications when telemetry is unavailable" do
    entries = described_class::Classifications.analyze_encounter({"outcome" => "hero_victory"})
    expect(entries.length).to eq(described_class::Classifications::IDS.length)
    expect(entries).to all(include("status" => "indeterminate"))
    expect(entries).to all(satisfy { |entry| entry.dig("evidence", "unavailable") == ["telemetry"] })
  end

  it "observes resource exhaustion only from sufficient resource telemetry" do
    entries = classifications(duration: 200_000, changes: 0, hp_lost: 0,
                              longest_idle: 100_000,
                              resources: {"available" => true, "state_changes" => 2,
                                          "exhausted_actors" => ["hero"],
                                          "resource_ids" => ["charges"]})
    resource = entries.find { |entry| entry["id"] == "resource_exhaustion" }
    expect(resource.fetch("status")).to eq("observed")
    expect(resource.dig("evidence", "actors")).to eq(["hero"])
    expect(entries.map { |entry| entry["id"] }).not_to include("no_progress")
  end

  it "keeps classification ordering and structured evidence stable" do
    encounter = {"outcome" => "beat_cap",
                 "telemetry" => telemetry(duration: 200_000, changes: 0, hp_lost: 0,
                                          longest_idle: 200_000, attempted: 3,
                                          completed: 3, productive: 0)}
    first = described_class::Classifications.analyze_encounter(encounter)
    second = described_class::Classifications.analyze_encounter(Marshal.load(Marshal.dump(encounter)))
    expect(first).to eq(second)
    expect(first.map { |entry| entry["id"] }).to eq(
      %w[no_progress stalemate_candidate resource_exhaustion])
  end

  it "reads legacy summaries and recordings as unavailable rather than zero telemetry" do
    legacy = run_summary(seed: 1)
    entries = described_class::Classifications.analyze_run(legacy)
    expect(entries).to all(include("status" => "indeterminate"))
  end
end
