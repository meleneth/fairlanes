# frozen_string_literal: true

RSpec.describe FairlanesContent::Artifacts do
  it "writes all generated artifacts" do
    declarations = build(:declaration_set)

    Dir.mktmpdir do |dir|
      described_class.new(declarations).write!(dir)

      expect(Dir.children(dir)).to contain_exactly(
        "generated_decal_content_tests.cpp",
        "decal_content_balance.md",
        "effect_gallery.md",
        "content_manifest.json",
        "content_manifest.schema.json"
      )
    end
  end

  it "does not rewrite artifacts whose contents are unchanged" do
    declarations = build(:declaration_set)

    Dir.mktmpdir do |dir|
      artifacts = described_class.new(declarations)
      artifacts.write!(dir)

      path = File.join(dir, "content_manifest.json")
      old_time = Time.at(1)
      File.utime(old_time, old_time, path)

      artifacts.write!(dir)

      expect(File.mtime(path)).to eq(old_time)
    end
  end

  it "rewrites artifacts whose contents changed" do
    declarations = build(:declaration_set)

    Dir.mktmpdir do |dir|
      artifacts = described_class.new(declarations)
      artifacts.write!(dir)

      path = File.join(dir, "content_manifest.json")
      File.write(path, "{}\n")

      artifacts.write!(dir)

      expect(File.read(path)).to eq(artifacts.generated.fetch("content_manifest.json"))
    end
  end

  it "checks current generated artifacts without rewriting them" do
    declarations = build(:declaration_set)

    Dir.mktmpdir do |dir|
      artifacts = described_class.new(declarations)
      artifacts.write!(dir)

      expect(artifacts.check!(dir)).to be(true)
    end
  end

  it "reports stale generated artifacts" do
    declarations = build(:declaration_set)

    Dir.mktmpdir do |dir|
      artifacts = described_class.new(declarations)
      artifacts.write!(dir)
      File.write(File.join(dir, "content_manifest.json"), "{}\n")

      expect { artifacts.check!(dir) }
        .to raise_error(FairlanesContent::StaleArtifactsError) { |error|
          expect(error.errors).to include("content_manifest.json is stale")
        }
    end
  end
end
