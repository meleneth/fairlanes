#include "fl/widgets/production_view.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "fl/ecs/components/consumable.hpp"
#include "fl/ecs/components/crafted_provenance.hpp"
#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/profession_expertise.hpp"
#include "fl/ecs/components/skill_study.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/generated/production_content.hpp"
#include "fl/lospec500.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/account_economy.hpp"
#include "fl/primitives/party_activity.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/world_clock.hpp"
#include "fl/skills/skill.hpp"

namespace fl::widgets {
namespace {

constexpr std::array<std::string_view, 5> kModes{"Combat", "Idle", "Gather", "Craft", "Study"};

using StudyChoice = std::pair<entt::entity, fl::skills::SkillKey>;

std::vector<StudyChoice> study_choices(const fl::context::AccountCtx& ctx,
                                       const fl::primitives::PartyData& party) {
  std::vector<StudyChoice> result;
  for (const auto& member : party.members())
    if (const auto* study = ctx.reg().try_get<fl::ecs::components::SkillStudy>(member.member_id()))
      for (const auto& entry : study->eligible) result.emplace_back(member.member_id(), entry.skill);
  return result;
}

} // namespace

ProductionView::ProductionView(fl::context::AccountCtx ctx, std::size_t party_index)
    : ctx_(ctx), party_index_(party_index) {}

std::size_t ProductionView::option_count() const {
  if (mode_ == 2) return fl::economy::generated_content::kMaterials.size();
  if (mode_ == 3) return fl::economy::generated_content::kRecipes.size();
  if (mode_ == 4 && party_index_ < ctx_.account_data().parties().size())
    return study_choices(ctx_, ctx_.account_data().party(party_index_)).size();
  return 1;
}

bool ProductionView::OnEvent(ftxui::Event event) {
  if (event == ftxui::Event::Character("d") &&
      party_index_ < ctx_.account_data().parties().size() &&
      !ctx_.account_data().economy().items().empty()) {
    const auto item = ctx_.account_data().economy().items().front();
    return ctx_.account_data().economy().distribute_item(
        ctx_.account_data().party(party_index_), item);
  }
  if (event == ftxui::Event::ArrowRight || event == ftxui::Event::Character("l")) {
    mode_ = (mode_ + 1) % kModes.size();
    option_ = 0;
    return true;
  }
  if (event == ftxui::Event::ArrowLeft || event == ftxui::Event::Character("h")) {
    mode_ = (mode_ + kModes.size() - 1) % kModes.size();
    option_ = 0;
    return true;
  }
  const auto count = option_count();
  if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character("j")) {
    option_ = count ? (option_ + 1) % count : 0;
    return true;
  }
  if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character("k")) {
    option_ = count ? (option_ + count - 1) % count : 0;
    return true;
  }
  if (event == ftxui::Event::Return) {
    apply();
    return true;
  }
  return false;
}

void ProductionView::apply() {
  if (party_index_ >= ctx_.account_data().parties().size()) return;
  auto& party = ctx_.account_data().party(party_index_);
  if (mode_ == 0) {
    party.request_activity({.activity = fl::primitives::PartyActivity::CombatProgression});
  } else if (mode_ == 1) {
    party.request_activity({.activity = fl::primitives::PartyActivity::Idle});
  } else if (mode_ == 2 && option_ < fl::economy::generated_content::kMaterials.size()) {
    const auto& material = fl::economy::generated_content::kMaterials[option_];
    party.request_activity({.activity = fl::primitives::PartyActivity::Gathering,
                            .profession = material.profession,
                            .biome_id = std::string(material.biome_id),
                            .material = material.id});
  } else if (mode_ == 3 && option_ < fl::economy::generated_content::kRecipes.size()) {
    const auto& recipe = fl::economy::generated_content::kRecipes[option_];
    party.request_activity({.activity = fl::primitives::PartyActivity::Crafting,
                            .profession = recipe.profession,
                            .recipe = recipe.id});
  } else if (mode_ == 4) {
    const auto choices = study_choices(ctx_, party);
    if (option_ < choices.size())
      party.request_activity({.activity = fl::primitives::PartyActivity::SkillStudy,
                              .profession = fl::economy::Profession::Study,
                              .student = choices[option_].first,
                              .skill = choices[option_].second});
  }
}

ftxui::Element ProductionView::Render() {
  using namespace ftxui;
  auto& account = ctx_.account_data();
  if (account.parties().empty()) return text("No party selected.");
  auto& party = account.party(std::min(party_index_, account.parties().size() - 1));
  auto& economy = account.economy();
  Elements modes;
  for (std::size_t i = 0; i < kModes.size(); ++i) {
    auto label = text(std::string(i == mode_ ? "[" : " ") + std::string(kModes[i]) +
                      (i == mode_ ? "]" : " "));
    if (i == mode_) label = label | bold | inverted;
    modes.push_back(std::move(label));
  }

  Elements details;
  if (mode_ == 2) {
    for (std::size_t i = 0; i < fl::economy::generated_content::kMaterials.size(); ++i) {
      const auto& value = fl::economy::generated_content::kMaterials[i];
      details.push_back(text(fmt::format("{}{} | {} | {}s | stock {} | {}",
                                         i == option_ ? "> " : "  ", value.display_name,
                                         fl::economy::display_name(value.profession),
                                         value.duration_seconds, economy.material_count(value.id),
                                         party.biome_unlocked(value.biome_id) ? "available" : "LOCKED")));
    }
  } else if (mode_ == 3) {
    for (std::size_t i = 0; i < fl::economy::generated_content::kRecipes.size(); ++i) {
      const auto& value = fl::economy::generated_content::kRecipes[i];
      std::string inputs;
      for (const auto& input : value.inputs) {
        if (!inputs.empty()) inputs += ", ";
        inputs += fmt::format("{} {}/{}", fl::economy::stable_id(input.material),
                              economy.material_count(input.material), input.quantity);
      }
      details.push_back(text(fmt::format("{}{} | {} {} | {}s | {}",
                                         i == option_ ? "> " : "  ", value.display_name,
                                         fl::economy::display_name(value.profession),
                                         value.required_proficiency, value.duration_seconds, inputs)));
    }
  } else if (mode_ == 4) {
    const auto choices = study_choices(ctx_, party);
    if (choices.empty()) details.push_back(text("No successfully observed advanced skills."));
    for (std::size_t i = 0; i < choices.size(); ++i) {
      const auto* study = ctx_.reg().get<fl::ecs::components::SkillStudy>(choices[i].first).find(
          choices[i].second);
      const auto& name = ctx_.reg().get<fl::ecs::components::Stats>(choices[i].first).name_;
      details.push_back(text(fmt::format("{}{} studies {} | {} beats accumulated",
                                         i == option_ ? "> " : "  ", name,
                                         fl::skills::display_name(choices[i].second),
                                         study ? study->progress_beats : 0)));
    }
  } else {
    details.push_back(text(mode_ == 0 ? "Resume tower progression at the next safe boundary."
                                      : "Remain idle without advancing the tower."));
  }

  Elements profession_rows;
  for (const auto& member : party.members()) {
    const auto* expertise =
        ctx_.reg().try_get<fl::ecs::components::ProfessionExpertise>(member.member_id());
    std::string row = member.name() + ": ";
    if (expertise)
      for (const auto profession : fl::economy::kProfessions)
        row += fmt::format("{} {}  ", fl::economy::display_name(profession),
                           expertise->proficiency(profession));
    profession_rows.push_back(paragraph(row));
  }

  std::string operation = "none";
  if (const auto& active = party.production_operation()) {
    const auto now = account.calendar().elapsed_beats();
    operation = fmt::format("{} by entity {} | {} beats remaining",
                            fl::primitives::display_name(active->activity),
                            entt::to_integral(active->worker),
                            active->completes_world_beat > now ? active->completes_world_beat - now : 0);
  }
  std::string objective = "none";
  if (const auto* active = economy.objective())
    objective = active->completed ? "complete" : active->blocked_reason.empty()
                                                  ? "active"
                                                  : "BLOCKED: " + active->blocked_reason;

  Elements inventory;
  inventory.push_back(text(fmt::format("Account products: {}", economy.items().size())));
  for (const auto item : economy.items()) {
    std::string label = "item #" + std::to_string(entt::to_integral(item));
    if (const auto* consumable = ctx_.reg().try_get<fl::ecs::components::Consumable>(item))
      label = std::string(fl::consumables::display_name(consumable->kind)) +
              " x" + std::to_string(consumable->quantity);
    if (const auto* equipment = ctx_.reg().try_get<fl::ecs::components::Equipment>(item))
      label = std::string(equipment->name());
    if (const auto* provenance = ctx_.reg().try_get<fl::ecs::components::CraftedProvenance>(item))
      label += " | crafter " + provenance->crafter_name + " (" +
               std::string(fl::economy::display_name(provenance->profession)) + " " +
               std::to_string(provenance->proficiency) + ")";
    inventory.push_back(text(label));
  }

  return window(text("Production - " + std::string(party.name())) | bold,
                vbox({text("Current activity: " +
                           std::string(fl::primitives::display_name(party.activity_assignment().activity))),
                      text("Operation: " + operation),
                      text("Objective: " + objective),
                      party.production_blocked_reason().empty()
                          ? text("Status: ready")
                          : text("Status: BLOCKED - " +
                                 std::string(party.production_blocked_reason())),
                      separator(), hbox(std::move(modes)), separator(),
                      vbox(std::move(details)) | flex, separator(),
                      text("Character expertise") | bold, vbox(std::move(profession_rows)),
                      separator(), vbox(std::move(inventory)),
                      separator(),
                      text("←/→ activity  ↑/↓ option  Enter assign  d distribute first product  Esc back") |
                          dim})) |
         bgcolor(fl::lospec500::color_at(0)) |
         fl::lospec500::on_not_black(fl::lospec500::color_at(32));
}

} // namespace fl::widgets
