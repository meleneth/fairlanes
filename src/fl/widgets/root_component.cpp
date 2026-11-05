#include "root_component.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "account_battle_view.hpp"
#include "bestiary_view.hpp"
#include "console_overlay.hpp"
#include "effect_gallery_view.hpp"
#include "fl/lospec500.hpp"
#include "fl/tracy_shim.hpp"
#include "libram_view.hpp"
#include "party_battle_screen.hpp"
#include "party_view.hpp"
#include "raid_view.hpp"
#include "root_chrome.hpp"
#include "story_screen.hpp"

namespace fl::widgets {

namespace generated = fl::generated::ftxer;

RootComponent::RootComponent(fl::context::AccountCtx ctx,
                             std::deque<fl::primitives::AccountData>& accounts,
                             FancyLog& console_log, fl::primitives::WorldClock& world_clock,
                             const fl::primitives::DiscoveryJournal* discoveries,
                             fl::events::AchievementBus* achievements,
                             fl::primitives::ObjectiveSystem* objectives)
    : ctx_(std::move(ctx)), accounts_(&accounts), console_log_(&console_log),
      world_clock_(&world_clock), commands_(accounts, console_log, world_clock),
      discoveries_(discoveries), objectives_(objectives) {
  important_notices_.set_auto_acknowledge(objectives_ && objectives_->autoplay_enabled());
  console_overlay_ = ftxui::Make<ConsoleOverlay>(console_log_);
  console_overlay()->set_on_command(
      [this](std::string_view command) { commands_.handle(command); });

  quit_cancel_ = ftxui::Button(
      "Cancel", [this] { shell_.close_overlay(generated::OverlayId::QuitConfirmation); });
  auto quit = ftxui::Button("Quit", [this] { quit_requested_ = true; });
  auto quit_buttons = ftxui::Container::Horizontal({quit_cancel_, quit});
  quit_confirmation_ = ftxui::Renderer(quit_buttons, [quit_buttons] {
    using namespace ftxui;
    return window(text("Quit Fairlanes?") | bold,
                  vbox({text("Progress is not saved and will be lost."), separator(),
                        quit_buttons->Render()})) |
           center | bgcolor(fl::lospec500::color_at(0)) | clear_under;
  });

  auto autoplay_body = ftxui::Renderer([] {
    using namespace ftxui;
    return window(text("Enable story autoplay?"),
                  vbox({paragraph("Autoplay will make dialogue, town, and purchase decisions for "
                                  "the opening story."),
                        text("Enter/y: enable   Esc/n: stay manual")})) |
           center | bgcolor(fl::lospec500::color_at(0)) | clear_under;
  });
  autoplay_confirmation_ = ftxui::CatchEvent(autoplay_body, [this](const ftxui::Event& event) {
    if (event == ftxui::Event::Return || event == ftxui::Event::Character("y")) {
      if (objectives_) objectives_->request_autoplay(true);
      shell_.close_overlay(generated::OverlayId::AutoplayConfirmation);
      return true;
    }
    if (event == ftxui::Event::Character("n")) {
      shell_.close_overlay(generated::OverlayId::AutoplayConfirmation);
      return true;
    }
    return true;
  });

  auto sandbox_body = ftxui::Renderer([] {
    using namespace ftxui;
    return window(text("Switch to full roster?"),
                  vbox({paragraph("The current story session cannot be recovered after switching."),
                        text("Enter/y: switch   Esc/n: stay")})) |
           center | bgcolor(fl::lospec500::color_at(0)) | clear_under;
  });
  sandbox_confirmation_ = ftxui::CatchEvent(sandbox_body, [this](const ftxui::Event& event) {
    if (event == ftxui::Event::Return || event == ftxui::Event::Character("y")) {
      sandbox_requested_ = true;
      quit_requested_ = true;
      return true;
    }
    if (event == ftxui::Event::Character("n")) {
      shell_.close_overlay(generated::OverlayId::SandboxConfirmation);
      return true;
    }
    return true;
  });

  auto help_body = ftxui::Renderer([this] { return render_keybind_help(); });
  keybind_help_ = ftxui::CatchEvent(help_body, [this](const ftxui::Event& event) {
    if (event == ftxui::Event::Character("h"))
      shell_.close_overlay(generated::OverlayId::KeybindHelp);
    return true;
  });

  commands_.set_show_account_view([this](std::size_t account_index) {
    ctx_ = make_context(account_index);
    shell_.navigate(generated::ScreenId::AccountBattle, true);
  });
  commands_.set_show_party_view([this](std::size_t account_index, std::size_t) {
    ctx_ = make_context(account_index);
    shell_.navigate(generated::ScreenId::Party, true);
  });
  commands_.set_show_effect_gallery(
      [this] { shell_.navigate(generated::ScreenId::EffectGallery, true); });
  commands_.set_show_libram([this] {
    return_monster_.reset();
    selected_skill_.reset();
    shell_.navigate(generated::ScreenId::Libram, true);
  });
  commands_.set_show_bestiary([this] { shell_.navigate(generated::ScreenId::Bestiary, true); });
  commands_.set_account_selected([this](std::size_t account_index, std::size_t) {
    ctx_ = make_context(account_index);
    if (shell_.refresh) shell_.refresh();
  });
  commands_.set_spawn_visitor([this](std::size_t account_index) {
    auto context = make_context(account_index);
    return context.account_data().spawn_visitor(context);
  });
  if (objectives_)
    commands_.set_start_sandbox(
        [this] { shell_.open_overlay(generated::OverlayId::SandboxConfirmation); });

  for (auto& account : accounts) {
    raid_started_subs_.emplace_back(
        account.raid_bus(), std::in_place_type<fl::events::RaidStarted>,
        [this, account_id = account.account_id()](const fl::events::RaidStarted&) {
          if (ctx_.account_data().account_id() == account_id && shell_.navigate)
            shell_.navigate(generated::ScreenId::Raid, false);
        });
    for (auto& party : account.parties()) {
      combat_started_subs_.emplace_back(party.party_bus(),
                                        std::in_place_type<fl::events::PartyCombatStarted>,
                                        [this, account_id = account.account_id()](const auto&) {
                                          on_party_combat_started(account_id);
                                        });
      milestone_subs_.emplace_back(party.party_bus(), std::in_place_type<fl::events::BiomeUnlocked>,
                                   [this](const fl::events::BiomeUnlocked& event) {
                                     const std::string name =
                                         event.biome_id == "origin_plains_tower"
                                             ? "Origin Plains Tower"
                                             : event.biome_id;
                                     important_notices_.enqueue({"BIOME UNLOCKED", name, 20});
                                   });
      milestone_subs_.emplace_back(party.party_bus(), std::in_place_type<fl::events::CycleUnlocked>,
                                   [this](const fl::events::CycleUnlocked& event) {
                                     std::string name;
                                     switch (event.cycle) {
                                     case fl::primitives::Cycle::Origin: name = "Origin"; break;
                                     case fl::primitives::Cycle::Resonance:
                                       name = "Resonance";
                                       break;
                                     case fl::primitives::Cycle::Conflict: name = "Conflict"; break;
                                     case fl::primitives::Cycle::Singularity:
                                       name = "Singularity";
                                       break;
                                     }
                                     important_notices_.enqueue({"CYCLE UNLOCKED", name, 27});
                                   });
    }
  }
  if (achievements) {
    achievement_sub_ = fl::events::ScopedAchievementListener{
        *achievements, std::in_place_type<fl::events::AchievementUnlocked>,
        [this](const fl::events::AchievementUnlocked& event) {
          if (event.achievement == fl::events::AchievementId::RaidMutualDestruction)
            important_notices_.enqueue({"ACHIEVEMENT UNLOCKED", "Raid Mutual Destruction", 14});
        }};
  }
  if (objectives_) {
    objective_sub_ = fl::events::ScopedStoryListener{
        objectives_->bus(), std::in_place_type<fl::events::ObjectiveRevealed>,
        [this](const fl::events::ObjectiveRevealed& event) {
          for (const auto& objective : fl::story::generated_content::kObjectives)
            if (objective.id == event.id)
              important_notices_.enqueue({"OBJECTIVE REVEALED", std::string(objective.title), 27});
        }};
    autoplay_sub_ = fl::events::ScopedStoryListener{
        objectives_->bus(), std::in_place_type<fl::events::AutoplayChanged>,
        [this](const fl::events::AutoplayChanged& event) {
          important_notices_.set_auto_acknowledge(event.enabled);
          console_log_->append_plain(event.enabled ? "Story autoplay enabled."
                                                   : "Story autoplay paused.");
        }};
  }
}

void RootComponent::bind_shell(generated::ShellCallbacks callbacks) {
  shell_ = std::move(callbacks);
}

generated::ScreenId RootComponent::initial_screen(generated::ScreenId declared) const {
  return ctx_.account_data().in_raid() ? generated::ScreenId::Raid : declared;
}

ftxui::Component RootComponent::make_raid() {
  auto* raid = ctx_.account_data().raid();
  if (!raid) return {};
  return ftxui::Make<RaidView>(ctx_);
}

ftxui::Component RootComponent::make_party() {
  if (raid_locked()) return {};
  return ftxui::Make<PartyView>(ctx_, commands_.party_index());
}

ftxui::Component RootComponent::make_account_battle() {
  if (raid_locked()) return {};
  return ftxui::Make<AccountBattleView>(ctx_);
}

ftxui::Component RootComponent::make_party_battle() {
  if (raid_locked()) return {};
  return ftxui::Make<PartyBattleScreen>(ctx_, commands_.party_index());
}

ftxui::Component RootComponent::make_effect_gallery() {
  if (raid_locked()) return {};
  return ftxui::Make<EffectGalleryView>();
}

ftxui::Component RootComponent::make_libram() {
  if (raid_locked() || !discoveries_) return {};
  auto screen = ftxui::Make<LibramView>(*discoveries_, selected_skill_);
  selected_skill_.reset();
  return screen;
}

ftxui::Component RootComponent::make_bestiary() {
  if (raid_locked() || !discoveries_) return {};
  auto selected = return_monster_;
  return_monster_.reset();
  return ftxui::Make<BestiaryView>(
      *discoveries_,
      [this](fl::monster::MonsterKind kind, fl::skills::SkillId skill) {
        return_monster_ = kind;
        selected_skill_ = skill;
        shell_.navigate(generated::ScreenId::Libram, true);
      },
      selected);
}

ftxui::Component RootComponent::make_story_screen(bool village) {
  if (raid_locked() || !objectives_) return {};
  auto& parties = ctx_.account_data().parties();
  if (parties.empty()) return {};
  auto& party = parties[std::min(commands_.party_index(), parties.size() - 1)];
  return ftxui::Make<StoryScreen>(
      *objectives_, party, [this](std::string_view id) { objectives_->emit(id); },
      [this] { objectives_->request_purchase("village_guard_hide"); },
      [&party](std::string_view biome, int floor) { return party.select_tower(biome, floor); },
      [this](std::string_view opportunity) { objectives_->request_recruitment(opportunity); },
      village ? StoryScreen::Start::Village : StoryScreen::Start::Objectives);
}

ftxui::Component RootComponent::make_objectives() {
  return make_story_screen(false);
}
ftxui::Component RootComponent::make_village() {
  return make_story_screen(true);
}

ftxui::Component RootComponent::make_console_overlay() {
  return console_overlay_;
}
ftxui::Component RootComponent::make_quit_confirmation_overlay() {
  return quit_confirmation_;
}
ftxui::Component RootComponent::make_autoplay_confirmation_overlay() {
  return autoplay_confirmation_;
}
ftxui::Component RootComponent::make_sandbox_confirmation_overlay() {
  return sandbox_confirmation_;
}
ftxui::Component RootComponent::make_keybind_help_overlay() {
  return keybind_help_;
}

ftxui::Element RootComponent::decorate(generated::ScreenId screen, ftxui::Element content) {
  using namespace ftxui;
  ZoneScoped;
  console_overlay()->tick();
  important_notices_.tick();
  update_fps_counter();
  auto& account = ctx_.account_data();
  content = render_root_chrome(account.calendar(), std::move(content),
                               VisitorCountdown{account.beats_until_visitor(), account.in_raid()});
  if (screen != generated::ScreenId::Raid) content = dbox({content, render_help_hint()});
  if (important_notices_.active()) {
    const int height = viewport_box_.y_max > viewport_box_.y_min
                           ? viewport_box_.y_max - viewport_box_.y_min + 1
                           : 24;
    content = dbox({content, important_notices_.render(height)});
  }
  return content | reflect(viewport_box_);
}

bool RootComponent::intercept(const ftxui::Event& event) {
  if (important_notices_.active() &&
      (event == ftxui::Event::Character(" ") || event == ftxui::Event::Return)) {
    important_notices_.dismiss();
    return true;
  }
  return false;
}

void RootComponent::overlay_opened(generated::OverlayId overlay) {
  if (overlay == generated::OverlayId::Console) {
    console_overlay()->set_open(true);
    console_overlay()->FocusInput();
  } else if (overlay == generated::OverlayId::QuitConfirmation) {
    quit_cancel_->TakeFocus();
  }
}

void RootComponent::overlay_closed(generated::OverlayId overlay) {
  if (overlay == generated::OverlayId::Console) console_overlay()->set_open(false);
}

bool RootComponent::toggle_autoplay(generated::ScreenId) {
  if (!objectives_) return false;
  if (objectives_->autoplay_enabled())
    objectives_->request_autoplay(false);
  else
    shell_.open_overlay(generated::OverlayId::AutoplayConfirmation);
  return true;
}

bool RootComponent::toggle_help(generated::ScreenId) {
  shell_.open_overlay(generated::OverlayId::KeybindHelp);
  return true;
}

bool RootComponent::return_to_town(generated::ScreenId screen) {
  if (screen != generated::ScreenId::Party && screen != generated::ScreenId::PartyBattle)
    return false;
  auto& parties = ctx_.account_data().parties();
  if (parties.empty()) return false;
  parties[std::min(commands_.party_index(), parties.size() - 1)].party_bus().emit(
      fl::events::PartyEvent{fl::events::PartyReturnToTownRequested{}});
  return true;
}

bool RootComponent::collapse_help(generated::ScreenId) {
  help_hint_collapsed_ = true;
  return true;
}
bool RootComponent::expand_help(generated::ScreenId) {
  help_hint_collapsed_ = false;
  return true;
}
bool RootComponent::overdrive_up(generated::ScreenId) {
  commands_.adjust_overdrive(1);
  return true;
}
bool RootComponent::overdrive_down(generated::ScreenId) {
  commands_.adjust_overdrive(-1);
  return true;
}
bool RootComponent::select_party(int delta) {
  commands_.select_party_relative(delta);
  return true;
}
bool RootComponent::previous_party(generated::ScreenId) {
  return select_party(-1);
}
bool RootComponent::next_party(generated::ScreenId) {
  return select_party(1);
}
bool RootComponent::select_account(int delta) {
  commands_.select_account_relative(delta);
  ctx_ = make_context(commands_.account_index());
  return true;
}
bool RootComponent::previous_account(generated::ScreenId) {
  return select_account(-1);
}
bool RootComponent::next_account(generated::ScreenId) {
  return select_account(1);
}

std::size_t RootComponent::console_history_size() const {
  return console_overlay() ? console_overlay()->history_size() : 0U;
}
std::string_view RootComponent::console_input_line() const {
  return console_overlay() ? std::string_view{console_overlay()->input_line()} : std::string_view{};
}
void RootComponent::set_full_open() {
  console_overlay()->set_full_open();
}

ConsoleOverlay* RootComponent::console_overlay() {
  return dynamic_cast<ConsoleOverlay*>(console_overlay_.get());
}
const ConsoleOverlay* RootComponent::console_overlay() const {
  return dynamic_cast<const ConsoleOverlay*>(console_overlay_.get());
}

fl::context::AccountCtx RootComponent::make_context(std::size_t account_index) {
  return fl::context::AccountCtx{ctx_.reg(), ctx_.rng(), accounts_->at(account_index)};
}

bool RootComponent::raid_locked() const noexcept {
  return ctx_.account_data().in_raid();
}

void RootComponent::on_party_combat_started(entt::entity account) {
  if (ctx_.account_data().account_id() == account && !ctx_.account_data().in_raid() &&
      shell_.navigate)
    shell_.navigate(generated::ScreenId::PartyBattle, false);
}

void RootComponent::update_fps_counter() {
  ++render_frames_;
  const auto now = std::chrono::steady_clock::now();
  if (!fps_initialized_) {
    fps_initialized_ = true;
    fps_window_start_ = now;
    fps_frame_count_ = 0;
    displayed_fps_ = 0;
    return;
  }
  ++fps_frame_count_;
  const auto elapsed = now - fps_window_start_;
  if (elapsed < std::chrono::milliseconds{500}) return;
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  if (elapsed_ms > 0)
    displayed_fps_ = static_cast<int>(
        (static_cast<double>(fps_frame_count_) * 1000.0 / static_cast<double>(elapsed_ms)) + 0.5);
  fps_frame_count_ = 0;
  fps_window_start_ = now;
}

ftxui::Element RootComponent::render_help_hint() const {
  using namespace ftxui;
  const auto label = color(fl::lospec500::color_at(14));
  const auto value = color(fl::lospec500::color_at(28)) | bold;
  const auto key = color(fl::lospec500::color_at(15)) | bold;
  const auto edge = color(fl::lospec500::color_at(15));
  const auto muted = color(fl::lospec500::color_at(24));
  if (help_hint_collapsed_) {
    auto handle = vbox({text("┌"), text(",") | key, text("└")}) | edge | clear_under;
    return vbox({filler(), hbox({filler(), std::move(handle)}), text("")});
  }
  auto chunk = [](std::string name, std::string value_text, Decorator label_style,
                  Decorator value_style) {
    return hbox({text(std::move(name)) | label_style, text(std::move(value_text)) | value_style});
  };
  const auto account_count = accounts_ ? accounts_->size() : 0U;
  const auto party_count = accounts_ && commands_.account_index() < accounts_->size()
                               ? accounts_->at(commands_.account_index()).parties().size()
                               : 0U;
  auto metrics = hbox({
      chunk("FPS ", std::to_string(displayed_fps_), label, value),
      text("  ") | muted,
      chunk("OD ", "x" + std::to_string(world_clock_->beat_rate_multiplier()), label, value),
      text("  ") | muted,
      chunk("AC ",
            std::to_string(commands_.account_index() + 1) + "/" + std::to_string(account_count),
            label, value),
      text("  ") | muted,
      chunk("PT ", std::to_string(commands_.party_index() + 1) + "/" + std::to_string(party_count),
            label, value),
      text("  ") | muted,
      chunk("WT ", std::to_string(world_clock_->elapsed_beats()), label, value),
  });
  auto controls = hbox({
      render_help_keys("=/-"),
      text(" overdrive  ") | muted,
      render_help_keys("[/]"),
      text(" party  ") | muted,
      render_help_keys(";/'"),
      text(" account  ") | muted,
      text("/") | key,
      text(" screen  ") | muted,
      text("Space") | key,
      text(" party battle  ") | muted,
      text("h") | key,
      text(" help  ") | muted,
      text(objectives_ ? "o" : "") | key,
      text(objectives_ ? " objectives  " : "") | muted,
      text(objectives_ ? "v" : "") | key,
      text(objectives_ ? " village  " : "") | muted,
      text(objectives_ ? "a" : "") | key,
      text(objectives_ ? " autoplay  " : "") | muted,
      text(".") | key,
      text(" hide") | muted,
  });
  auto panel = vbox({metrics, controls}) | border | edge | clear_under;
  return vbox({filler(), hbox({filler(), panel}), text("")});
}

ftxui::Element RootComponent::render_keybind_help() const {
  using namespace ftxui;
  const auto chrome = fl::lospec500::on_not_black(fl::lospec500::color_at(32));
  const auto accent = fl::lospec500::on_not_black(fl::lospec500::color_at(15));
  const auto bg = bgcolor(fl::lospec500::color_at(0));
  std::vector<Element> lines{
      text("Keybinds") | bold | accent,
      separator() | chrome,
      render_help_line("h / Esc", "  close this help"),
      render_help_line("`", "        open command console"),
      render_help_line(". / ,", "    collapse / expand metrics"),
      render_help_line("l / b", "    open libram / bestiary"),
      render_help_line("/", "        toggle account / party screen"),
      render_help_line("Space", "    toggle account / party battle"),
      render_help_line("= / -", "    change overdrive level"),
      render_help_line("[ / ]", "    switch selected party"),
      render_help_line("; / '", "    switch selected account"),
      render_help_line("q / Esc", "  back, or confirm quit from gameplay"),
      render_help_line("Ctrl-C", "   quit immediately"),
  };
  if (objectives_) {
    lines.insert(lines.end() - 2, render_help_line("o", "        open objectives"));
    lines.insert(lines.end() - 2, render_help_line("v", "        visit village from a party"));
    lines.insert(lines.end() - 2,
                 render_help_line("a", "        toggle story autoplay at any time"));
  }
  return window(text("Help") | accent, vbox(std::move(lines)) | bg) | chrome | bg | clear_under |
         size(WIDTH, GREATER_THAN, 44) | center;
}

} // namespace fl::widgets
