# frozen_string_literal: true

require "spec_helper"
require "rbconfig"
require "stringio"
require "open3"
require_relative "../../../scripts/quiet_command"

RSpec.describe FairlanesQuietCommand do
  it "captures successful output and reports reliable build metrics" do
    Dir.mktmpdir do |dir|
      log = File.join(dir, "build.log")
      command = [RbConfig.ruby, "-e", <<~RUBY]
        puts "[1/2] Building CXX object one.cpp.o"
        warn "one.cpp:4: warning: example warning"
        puts "[2/2] Linking CXX executable example"
      RUBY
      result = described_class.run(command, log_path: log, chdir: dir)

      expect(result).to be_success
      expect(result.compiled_units).to eq(1)
      expect(result.warnings).to eq(1)
      expect(File.read(log)).to include("Building CXX object", "example warning", "Linking")
    end
  end

  it "streams normal output only in verbose mode" do
    Dir.mktmpdir do |dir|
      output = StringIO.new
      original = $stdout
      $stdout = output
      described_class.run([RbConfig.ruby, "-e", 'puts "visible"'],
                          log_path: File.join(dir, "verbose.log"), chdir: dir, verbose: true)
      expect(output.string).to include("visible")
    ensure
      $stdout = original
    end
  end

  it "preserves exit status, complete logs, and focused diagnostics" do
    diagnostics = {
      compiler: "src/example.cpp:9:3: error: missing name",
      linker: "example.cpp.o: undefined reference to `missing()'",
      generator: "CMake Error at generator.cmake:12 (message): broken declaration",
      test: "1 - example assertion (Failed)\n***Failed: REQUIRE(value == 4)"
    }

    diagnostics.each do |kind, diagnostic|
      Dir.mktmpdir do |dir|
        log = File.join(dir, "#{kind}.log")
        command = [RbConfig.ruby, "-e", "puts #{diagnostic.dump}; puts 'complete trailing detail'; exit 17"]
        expect do
          described_class.run(command, log_path: log, chdir: dir)
        end.to raise_error(described_class::Failure) { |error|
          expect(error.result.exitstatus).to eq(17)
          expect(error.message).to include(diagnostic.lines.last.strip, "Log: #{log}")
          expect(File.read(log)).to include(diagnostic, "complete trailing detail")
        }
      end
    end
  end

  it "falls back to a bounded log tail when output has no recognized diagnostic" do
    Dir.mktmpdir do |dir|
      log = File.join(dir, "failed.log")
      command = [RbConfig.ruby, "-e", '70.times { |i| puts "line #{i}" }; exit 3']
      expect do
        described_class.run(command, log_path: log, chdir: dir)
      end.to raise_error(described_class::Failure) { |error|
        expect(error.message).to include("line 69")
        expect(error.message).not_to include("line 0\n")
      }
    end
  end

  it "keeps the build entry point quiet, supports verbose output, and runs tests" do
    Dir.mktmpdir do |dir|
      bin = File.join(dir, "bin")
      build = File.join(dir, "build")
      FileUtils.mkdir_p([bin, build])
      File.write(File.join(build, "CMakeCache.txt"), "configured\n")
      File.write(File.join(bin, "cmake"), <<~RUBY)
        #!#{RbConfig.ruby}
        puts "[1/2] Building CXX object example.cpp.o"
        puts "[2/2] Linking CXX executable example"
      RUBY
      File.write(File.join(bin, "ctest"), <<~RUBY)
        #!#{RbConfig.ruby}
        puts "100% tests passed, 0 tests failed out of 7"
      RUBY
      FileUtils.chmod(0o755, [File.join(bin, "cmake"), File.join(bin, "ctest")])
      environment = {"PATH" => "#{bin}:#{ENV.fetch('PATH')}"}
      script = File.expand_path("../../../scripts/build", __dir__)

      stdout, stderr, status = Open3.capture3(environment, RbConfig.ruby, script,
                                               "--build-dir", build, "--test")
      expect(status).to be_success
      expect(stderr).to be_empty
      expect(stdout).to include("BUILD PASS", "TEST PASS", "Passed: 7")
      expect(stdout).not_to include("Building CXX object")
      expect(File.read(File.join(build, "logs/build.log"))).to include("Building CXX object")

      stdout, = Open3.capture3(environment, RbConfig.ruby, script,
                               "--build-dir", build, "--verbose")
      expect(stdout).to include("Building CXX object", "BUILD PASS")
    end
  end

  it "propagates configure and build failures from the build entry point" do
    Dir.mktmpdir do |dir|
      bin = File.join(dir, "bin")
      FileUtils.mkdir_p(bin)
      File.write(File.join(bin, "cmake"), <<~RUBY)
        #!#{RbConfig.ruby}
        if ARGV.include?("-S")
          warn "CMake Error at CMakeLists.txt:4: configure broke"
          exit 19
        end
        warn "src/example.cpp:8: error: compilation broke"
        exit 23
      RUBY
      FileUtils.chmod(0o755, File.join(bin, "cmake"))
      environment = {"PATH" => "#{bin}:#{ENV.fetch('PATH')}"}
      script = File.expand_path("../../../scripts/build", __dir__)
      build = File.join(dir, "unconfigured")

      _stdout, stderr, status = Open3.capture3(environment, RbConfig.ruby, script,
                                                "--build-dir", build)
      expect(status.exitstatus).to eq(19)
      expect(stderr).to include("CMake Error at CMakeLists.txt:4", "configure.log")

      FileUtils.mkdir_p(build)
      File.write(File.join(build, "CMakeCache.txt"), "configured\n")
      _stdout, stderr, status = Open3.capture3(environment, RbConfig.ruby, script,
                                                "--build-dir", build)
      expect(status.exitstatus).to eq(23)
      expect(stderr).to include("src/example.cpp:8: error", "build.log")
    end
  end
end
