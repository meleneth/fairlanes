#include "objective_system.hpp"

#include <algorithm>

#include "fl/ecs/components/equipment.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/loot_drop.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/loot/equipment_slot.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::primitives {

bool ObjectiveSystem::supports(std::string_view id) {
  return id == "visitor_destroyed_village" || id == "recruit_angry_fellow" ||
         id == "first_combat_won" || id == "two_starter_wins" ||
         id == "village_gear_purchased" ||
         id == "recruit_second_hero";
}

ObjectiveSystem::ObjectiveSystem(fl::context::AccountCtx account)
    : account_(account) {
  fact_sub_ = fl::events::ScopedStoryListener{
      bus_, std::in_place_type<fl::events::StoryFact>,
      [this](const fl::events::StoryFact &event) { receive(event.id); }};
  autoplay_sub_ = fl::events::ScopedStoryListener{
      bus_, std::in_place_type<fl::events::AutoplayRequested>,
      [this](const fl::events::AutoplayRequested &event) {
        if (autoplay_enabled_ == event.enabled) return;
        autoplay_enabled_ = event.enabled;
        bus_.emit(fl::events::StoryEvent{
            fl::events::AutoplayChanged{event.enabled}});
      }};
  purchase_sub_ = fl::events::ScopedStoryListener{
      bus_, std::in_place_type<fl::events::VillagePurchaseRequested>,
      [this](const fl::events::VillagePurchaseRequested &event) {
        if (event.item_id == "village_guard_hide") buy_village_guard_hide();
      }};
  auto &account_data = account_.account_data();
  raid_sub_ = fl::events::ScopedRaidListener{
      account_data.raid_bus(), std::in_place_type<fl::events::RaidResolved>,
      [this](const fl::events::RaidResolved &event) {
        if (event.id == 1 && event.result != fl::events::RaidResult::Victory)
          emit("visitor_destroyed_village");
      }};
  if (account_data.parties().empty()) return;
  auto &bus = account_data.parties().front().party_bus();
  victory_sub_ = fl::events::ScopedPartyListener{
      bus, std::in_place_type<fl::events::PartyVictory>, [this](const auto &) {
        ++combat_wins_;
        if (combat_wins_ == 1) emit("first_combat_won");
        if (combat_wins_ == 2) emit("two_starter_wins");
      }};
  return_sub_ = fl::events::ScopedPartyListener{
      bus, std::in_place_type<fl::events::PartyReturnedToTown>, [this](const auto &) {
        if (combat_wins_ >= 2) returned_after_two_ = true;
      }};
}

void ObjectiveSystem::emit(std::string_view event_id) {
  bus_.emit(fl::events::StoryEvent{fl::events::StoryFact{std::string(event_id)}});
}

void ObjectiveSystem::request_autoplay(bool enabled) {
  bus_.emit(fl::events::StoryEvent{fl::events::AutoplayRequested{enabled}});
}

void ObjectiveSystem::request_purchase(std::string_view item_id) {
  bus_.emit(fl::events::StoryEvent{
      fl::events::VillagePurchaseRequested{std::string(item_id)}});
}

void ObjectiveSystem::receive(std::string_view event_id) {
  if (event_id == "recruit_angry_fellow" && !active("recruit_first_hero")) return;
  if (event_id == "recruit_second_hero" && !active("recruit_second_hero")) return;
  const auto declared = std::find_if(
      fl::story::generated_content::kEvents.begin(),
      fl::story::generated_content::kEvents.end(),
      [event_id](const auto &event) { return event.id == event_id; });
  if (declared == fl::story::generated_content::kEvents.end()) return;
  if (!facts_.emplace(event_id).second) return;
  if (supports(event_id)) handle(event_id);
  else unhandled_.emplace_back(event_id);
  for (const auto &objective : fl::story::generated_content::kObjectives)
    if (objective.revealed_by == event_id)
      bus_.emit(fl::events::StoryEvent{
          fl::events::ObjectiveRevealed{std::string(objective.id)}});
}

void ObjectiveSystem::handle(std::string_view id) {
  auto &account = account_.account_data();
  if (id == "visitor_destroyed_village") {
    account.log().append_plain(
        "The first Visitor has devastated the village. The pub still stands.");
  } else if (id == "recruit_angry_fellow") {
    auto &party = account.parties().front();
    const auto hero = party.recruit_member("Alderion");
    if (hero != entt::null) {
      auto &stats = account_.reg().get<fl::ecs::components::Stats>(hero);
      stats.hp_ = stats.max_hp_ = 20;
      stats.attack_ = 2;
      party.members().back().grimoire().learn(fl::skills::SkillId::Thump);
      account_.reg().get<fl::ecs::components::PartyMember>(hero)
          .equip_known_skill(fl::skills::SkillId::Thump);
      party.enable_story_starter_fights();
    }
  } else if (id == "recruit_second_hero") {
    auto &party = account.parties().front();
    const auto hero = party.recruit_member("Bramblethorn");
    if (hero != entt::null) {
      auto &stats = account_.reg().get<fl::ecs::components::Stats>(hero);
      stats.hp_ = stats.max_hp_ = 20;
      stats.attack_ = 2;
      party.members().back().grimoire().learn(fl::skills::SkillId::Thump);
      account_.reg().get<fl::ecs::components::PartyMember>(hero)
          .equip_known_skill(fl::skills::SkillId::Thump);
    }
  }
}

int ObjectiveSystem::darrin_marks() const {
  return account_.account_data().parties().front().darrin_marks();
}

bool ObjectiveSystem::can_buy_village_guard_hide() const {
  const auto &party = account_.account_data().parties().front();
  return returned_after_two_ && party.story_shop_pending() &&
         !party.in_combat() && party.darrin_marks() >= 10 &&
         !party.members().empty();
}

bool ObjectiveSystem::buy_village_guard_hide() {
  if (!can_buy_village_guard_hide()) return false;
  auto &party = account_.account_data().parties().front();
  auto ctx = account_.party_context(party);
  if (!fl::ecs::systems::LootDropSystem::grant_unique(
          ctx, fl::loot::generated_content::GearId::VillageGuardHide))
    return false;
  if (!party.spend_darrin_marks(10)) return false;
  const auto item = std::find_if(party.items().begin(), party.items().end(),
      [this](entt::entity entity) {
        const auto *gear = account_.reg().try_get<fl::ecs::components::Equipment>(entity);
        return gear && gear->unique_id() == "village_guard_hide";
      });
  if (item != party.items().end()) {
    party.party_bus().emit(fl::events::PartyEvent{
        fl::events::EquipmentEquipRequested{
            party.members().front().member_id(), fl::loot::EquipmentSlot::chest,
            *item}});
  }
  party.finish_story_shop();
  emit("village_gear_purchased");
  return true;
}

bool ObjectiveSystem::active(std::string_view id) const {
  for (const auto &objective : fl::story::generated_content::kObjectives)
    if (objective.id == id)
      return facts_.contains(std::string(objective.revealed_by)) &&
             !facts_.contains(std::string(objective.completed_by));
  return false;
}

bool ObjectiveSystem::completed(std::string_view id) const {
  for (const auto &objective : fl::story::generated_content::kObjectives)
    if (objective.id == id)
      return facts_.contains(std::string(objective.completed_by));
  return false;
}

bool ObjectiveSystem::can_talk(std::string_view dialogue_id) const {
  if (dialogue_id == "angry_fellow") return active("recruit_first_hero");
  if (dialogue_id == "second_volunteer") return active("recruit_second_hero");
  return false;
}

void ObjectiveSystem::advance_autoplay() {
  if (!autoplay_enabled_ || fl::story::generated_content::kAutoplayRoutes.empty())
    return;
  const auto steps = fl::story::generated_content::kAutoplayRoutes.front().steps;
  while (autoplay_step_index_ < steps.size()) {
    const auto &step = steps[autoplay_step_index_];
    if (!facts_.contains(std::string(step.wait_for))) return;
    bool done = false;
    switch (step.action) {
    case fl::story::generated_content::AutoplayAction::ChooseDialogue: {
      if (!can_talk(step.target)) return;
      const auto *dialogue = fl::story::generated_content::find_dialogue(step.target);
      if (!dialogue) return;
      for (const auto &node : dialogue->nodes)
        for (const auto &choice : node.choices)
          if (choice.id == step.choice && !choice.event_id.empty()) {
            emit(choice.event_id);
            done = true;
          }
      break;
    }
    case fl::story::generated_content::AutoplayAction::ReturnToTown:
      account_.account_data().parties().front().party_bus().emit(
          fl::events::PartyEvent{fl::events::PartyReturnToTownRequested{}});
      done = returned_after_two_;
      break;
    case fl::story::generated_content::AutoplayAction::BuyGear:
      request_purchase(step.target);
      done = facts_.contains("village_gear_purchased");
      break;
    }
    if (!done) return;
    ++autoplay_step_index_;
  }
}

} // namespace fl::primitives
