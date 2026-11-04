$LOAD_PATH.unshift File.expand_path("../../FTXer/lib", __dir__)
$LOAD_PATH.unshift File.expand_path("../../CMaker/lib", __dir__)
require "ftxer"
require "tmpdir"

FAIRLANES_UI = FTXer.integration :fairlanes, namespace: "fl::generated::ftxer" do
  external_widget :existing_fairlanes_ui

  screen :raid
  screen :party
  screen :account_battle
  screen :party_battle
  screen :effect_gallery
  screen :libram
  screen :bestiary
  screen :story
  initial :party_battle

  console toggle: "`"

  key "l", navigate: :libram
  key "b", navigate: :bestiary
  key "o", action: :show_objectives
  key "v", action: :show_village
  key "a", action: :toggle_autoplay
  key "h", action: :toggle_help
  key "r", action: :return_to_town
  key " ", action: :toggle_party_battle
  key "/", action: :toggle_active_screen
  key ">", action: :collapse_help
  key ".", action: :collapse_help
  key "<", action: :expand_help
  key ",", action: :expand_help
  key "+", action: :overdrive_up
  key "=", action: :overdrive_up
  key "-", action: :overdrive_down
  key "[", action: :previous_party
  key "]", action: :next_party
  key "{", action: :previous_account
  key ";", action: :previous_account
  key "}", action: :next_account
  key "'", action: :next_account
  key "escape", back: true
  key "q", back: true
end

if $PROGRAM_NAME == __FILE__
  check = ARGV.delete("--check")
  output = ARGV.fetch(0, File.expand_path("../src/fl/generated", __dir__))
  generate = lambda do |destination|
    FTXer.generate_integration(
      FAIRLANES_UI,
      destination,
      header_include: "fl/generated/ftxer_composition_root.hpp"
    )
  end
  if check
    Dir.mktmpdir("fairlanes-ftxer-check") do |temporary|
      generate.call(temporary)
      %w[ftxer_composition_root.hpp ftxer_composition_root.cpp].each do |file|
        expected = File.binread(File.join(temporary, file))
        actual = File.binread(File.join(output, file))
        abort "generated FTXer UI is stale: #{file}" unless actual == expected
      end
    end
  else
    generate.call(output)
  end
end
