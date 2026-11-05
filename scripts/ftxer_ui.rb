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
  screen :objectives
  screen :village
  initial :party_battle

  console toggle: "`"
  overlay :quit_confirmation
  overlay :autoplay_confirmation
  overlay :sandbox_confirmation
  overlay :keybind_help
  empty_back opens: :quit_confirmation

  key "l", navigate: :libram, on: %i[raid party account_battle party_battle libram bestiary objectives village]
  key "b", navigate: :bestiary, on: %i[raid party account_battle party_battle libram bestiary objectives village]
  key "o", navigate: :objectives
  key "v", navigate: :village, on: %i[party party_battle]
  key "v", navigate: :village, on: :objectives, after: %i[party party_battle]
  key "a", action: :toggle_autoplay
  key "h", action: :toggle_help
  key "r", action: :return_to_town
  key " ", navigate: :party_battle, on: %i[account_battle libram bestiary effect_gallery objectives village]
  key " ", navigate: :account_battle, on: :party_battle
  key "/", navigate: :account_battle, on: :party
  key "/", navigate: :party, on: %i[raid account_battle party_battle]
  key "/", navigate: :party_battle, on: %i[effect_gallery libram bestiary objectives village]
  key ">", action: :collapse_help
  key ".", action: :collapse_help
  key "<", action: :expand_help
  key ",", action: :expand_help
  key "+", action: :overdrive_up
  key "=", action: :overdrive_up
  key "-", action: :overdrive_down
  key "[", action: :previous_party, refresh: true
  key "]", action: :next_party, refresh: true
  key "{", action: :previous_account, refresh: true
  key ";", action: :previous_account, refresh: true
  key "}", action: :next_account, refresh: true
  key "'", action: :next_account, refresh: true
  key "enter", navigate: :party_battle, on: :raid
  key "escape", back: true, on: %i[party objectives village], screen_first: true
  key "escape", back: true, on: %i[raid account_battle party_battle effect_gallery libram bestiary]
  key "q", back: true, on: %i[party objectives village], screen_first: true
  key "q", back: true, on: %i[raid account_battle party_battle effect_gallery libram bestiary]
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
