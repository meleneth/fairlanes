#include <fmt/format.h>

#include <algorithm>
#include <iterator>

#include "fl/context.hpp"
#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/combat_affinity.hpp"
#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/components/visual_effects.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/ecs/systems/grant_xp_to_party.hpp"
#include "fl/ecs/systems/loot_drop.hpp"
#include "fl/ecs/systems/manual_equipment.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop_machine.hpp"
#include "fl/generated/floor_content.hpp"
#include "fl/generated/gear_content.hpp"
#include "fl/generated/monster_content.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/skills/skill.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"
#include "sr/beat_bus.hpp"

namespace fl::primitives {

namespace {

std::string_view origin_biome_display(std::string_view biome_id) {
  if (biome_id == "origin_woodland_tower") return "Origin Woodland Tower";
  if (biome_id == "origin_plains_tower") return "Origin Plains Tower";
  if (biome_id == "origin_murky_swamp_tower") return "Origin Murky Swamp Tower";
  if (biome_id == "origin_caves_tower") return "Origin Caves Tower";
  if (biome_id == "origin_desert_tower") return "Origin Desert Tower";
  return biome_id;
}

} // namespace

bool PartyRoster::remove(entt::entity member) {
  const auto found = std::ranges::find_if(
      members_, [member](const MemberData* candidate) { return candidate->member_id() == member; });
  if (found == members_.end()) return false;
  members_.erase(found);
  return true;
}

bool PartyRoster::contains(entt::entity member) const noexcept {
  return std::ranges::any_of(
      members_, [member](const MemberData* candidate) { return candidate->member_id() == member; });
}

PartyData::PartyData(entt::entity party_id, fl::context::AccountCtx& account_ctx, std::string name)
    : party_id_(party_id), name_(std::move(name)),
      account_id_(account_ctx.account_data().account_id()),
      log_(std::make_unique<fl::widgets::FancyLog>()), party_ctx_(account_ctx.party_context(*this)),
      party_loop_machine_(std::make_unique<fl::fsm::PartyLoopMachine>(party_ctx_)) {
  // Optional: early log breadcrumb
  log_->append_markup(fmt::format("Party created: id={} name={}", (int)party_id_, name_));
  town_revitalize_sub_ = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyRevitalizeRequested>,
      [this](const fl::events::PartyRevitalizeRequested&) { revitalize_members(); }};
  loot_drop_sub_ = fl::ecs::systems::LootDropSystem::bind_listener(party_ctx_);
  manual_equipment_sub_ = fl::ecs::systems::ManualEquipmentSystem::bind_listener(party_ctx_);
  tower_progress_sub_ = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyVictory>,
      [this](const fl::events::PartyVictory&) { commit_tower_victory(); }};
}

void PartyData::commit_tower_victory() {
  if (in_raid() || !encounter_data_ || encounter_tower_biome_.empty()) return;

  auto* tower = tower_state(encounter_tower_biome_);
  if (tower == nullptr || !tower->unlocked) return;

  const bool frontier = encounter_tower_floor_ == tower->latest_floor &&
                        encounter_tower_floor_ > tower->cleared_floor;
  if (frontier) {
    tower->cleared_floor = encounter_tower_floor_;
    party_bus_.emit(fl::events::PartyEvent{
        fl::events::TowerFloorCleared{encounter_tower_biome_, encounter_tower_floor_}});

    if (encounter_tower_floor_ == 9) unlock_next_origin_biome(encounter_tower_biome_);
    if (encounter_tower_biome_ == "origin_plains_tower" && encounter_tower_floor_ == 1 &&
        !plains_reward_awarded_) {
      fl::ecs::systems::LootDropSystem::grant_unique(
          party_ctx_, fl::loot::generated_content::GearId::MossboundCape);
      plains_reward_awarded_ = true;
      log_->append_plain("Plains floor 1 reward: Mossbound Cape. Equip it in the party inventory.");
    }

    if (encounter_tower_floor_ == 100) {
      tower_complete_waiting_ = !repeat_selected_floor_;
      party_bus_.emit(fl::events::PartyEvent{fl::events::TowerCompleted{encounter_tower_biome_}});
    } else {
      tower->latest_floor = encounter_tower_floor_ + 1;
      if (!repeat_selected_floor_) tower->selected_floor = tower->latest_floor;
    }
  }
  tower_floor_attempts_ = 0;
  revitalize_members();
}

std::optional<PartyData::TowerRoute> PartyData::tower_route() const noexcept {
  const auto* tower = tower_state(tower_biome_);
  if (tower && tower->unlocked) return TowerRoute{tower->biome_id, tower->selected_floor};
  return std::nullopt;
}

void PartyData::begin_tower_encounter() {
  encounter_tower_biome_ = tower_biome_;
  const auto route = tower_route();
  encounter_tower_floor_ = route ? route->floor : 0;
  if (route) ++tower_floor_attempts_;
}

int PartyData::latest_tower_floor(std::string_view biome_id) const noexcept {
  const auto* tower = tower_state(biome_id);
  return tower && tower->unlocked ? tower->latest_floor : 0;
}

int PartyData::highest_tower_floor_cleared(std::string_view biome_id) const noexcept {
  const auto* tower = tower_state(biome_id);
  return tower && tower->unlocked ? tower->cleared_floor : 0;
}

PartyData::OriginTowerState* PartyData::tower_state(std::string_view biome_id) noexcept {
  const auto found = std::ranges::find(origin_towers_, biome_id, &OriginTowerState::biome_id);
  return found == origin_towers_.end() ? nullptr : &*found;
}

const PartyData::OriginTowerState*
PartyData::tower_state(std::string_view biome_id) const noexcept {
  const auto found = std::ranges::find(origin_towers_, biome_id, &OriginTowerState::biome_id);
  return found == origin_towers_.end() ? nullptr : &*found;
}

bool PartyData::biome_unlocked(std::string_view biome_id) const noexcept {
  const auto* tower = tower_state(biome_id);
  return tower != nullptr && tower->unlocked;
}

std::size_t PartyData::unlocked_origin_biome_count() const noexcept {
  return static_cast<std::size_t>(
      std::ranges::count(origin_towers_, true, &OriginTowerState::unlocked));
}

void PartyData::unlock_next_origin_biome(std::string_view cleared_biome) {
  const auto current = std::ranges::find(kOriginBiomeIds, cleared_biome);
  if (current == kOriginBiomeIds.end() || std::next(current) == kOriginBiomeIds.end()) return;
  auto* next = tower_state(*std::next(current));
  if (next == nullptr || next->unlocked) return;
  next->unlocked = true;
  party_bus_.emit(fl::events::PartyEvent{fl::events::BiomeUnlocked{std::string(next->biome_id)}});
  log_->append_plain(fmt::format("{} unlocked. {} progression continues until a new destination "
                                 "is selected.",
                                 origin_biome_display(next->biome_id),
                                 origin_biome_display(cleared_biome)));
}

bool PartyData::select_tower(std::string_view biome_id, std::optional<int> floor) {
  if (tower_state(biome_id) == nullptr) {
    log_->append_plain("Tower destination rejected: unknown biome.");
    return false;
  }
  const int latest = latest_tower_floor(biome_id);
  if (latest == 0 || (floor && (*floor < 1 || *floor > latest))) {
    log_->append_plain("Tower destination rejected: floor is not accessible to this party.");
    return false;
  }
  const int selected_floor = floor.value_or(latest);
  const bool repeat = floor.has_value() && selected_floor <= highest_tower_floor_cleared(biome_id);
  return request_tower_destination(biome_id, selected_floor, repeat);
}

bool PartyData::request_tower_destination(std::string_view biome_id, int selected_floor,
                                          bool repeat) {
  party_bus_.emit(fl::events::PartyEvent{
      fl::events::TowerDestinationRequested{std::string(biome_id), selected_floor}});
  if (has_encounter()) {
    pending_tower_destination_ =
        PendingTowerDestination{std::string(biome_id), selected_floor, repeat};
    return true;
  }
  return commit_tower_destination(biome_id, selected_floor, repeat);
}

bool PartyData::commit_tower_destination(std::string_view biome_id, int selected_floor,
                                         bool repeat) {
  const auto current = tower_route();
  const bool changed = tower_biome_ != biome_id || !current || current->floor != selected_floor;
  tower_biome_ = biome_id;
  tower_state(biome_id)->selected_floor = selected_floor;
  if (changed) tower_floor_attempts_ = 0;
  tower_choice_pending_ = false;
  tower_complete_waiting_ = false;
  repeat_selected_floor_ = repeat;
  party_bus_.emit(fl::events::PartyEvent{
      fl::events::TowerDestinationChanged{std::string(biome_id), selected_floor}});
  return true;
}

void PartyData::apply_pending_tower_destination() {
  if (!pending_tower_destination_) return;
  auto pending = std::move(*pending_tower_destination_);
  pending_tower_destination_.reset();
  commit_tower_destination(pending.biome_id, pending.floor, pending.repeat);
}

entt::entity PartyData::recruit_member(std::string name) {
  if (members_.size() >= 5) return entt::null;
  auto& reg = party_ctx_.reg();
  auto& member = party_ctx_.account_data().create_character(reg.create(), std::move(name));
  members_.add(member);
  const auto id = member.member_id();
  reg.emplace<fl::ecs::components::Stats>(id, member.name());
  member.hook_level_progression(reg);
  reg.emplace<fl::ecs::components::TrackXP>(id, party_ctx_.entity_context(id), 0);
  reg.emplace<fl::ecs::components::ActiveCombatAffinity>(id);
  reg.emplace<fl::ecs::components::PartyMember>(id, party_ctx_.entity_context(id), member.name(),
                                                party_id_, member);
  party_bus_.emit(fl::events::PartyEvent{fl::events::MemberJoined{id}});
  log_->append_plain(std::string(member.name()) + " joined the party.");
  return id;
}

entt::entity PartyData::recruit_origin_member(std::string name) {
  const auto hero = recruit_member(std::move(name));
  if (hero == entt::null) return entt::null;
  auto& stats = party_ctx_.reg().get<fl::ecs::components::Stats>(hero);
  stats.hp_ = stats.max_hp_ = 20;
  stats.attack_ = 2;
  members_.back().grimoire().learn(fl::skills::SkillId::Thump);
  party_ctx_.reg().get<fl::ecs::components::PartyMember>(hero).equip_known_skill(
      fl::skills::SkillId::Thump);
  return hero;
}

PartyData::~PartyData() {
  // EncounterData owns listeners into MemberData combatant buses. Clear it
  // before members_ is destroyed so listener handles disconnect from live buses.
  encounter_data_.reset();
}

void PartyData::hook_to_beat(seerin::BeatBus& gc_beat_bus) {
  gc_forward_sub_ =
      gc_beat_bus.subscribe<seerin::Beat>([this](const seerin::Beat&) { advance_beat(); });
}

void PartyData::advance_beat() {
  if (in_raid()) return;
  party_beat_bus_.emit(seerin::Beat{});
  party_loop_machine_->beat_event();
}

bool PartyData::needs_town() {
  return all_members_dead() || town_penalty_beats_remaining_ > 0;
}

bool PartyData::all_members_dead() const {
  using fl::ecs::components::Stats;

  bool has_members = false;
  for (const auto& member : members_) {
    has_members = true;
    if (const auto* stats = party_ctx_.reg().try_get<Stats>(member.member_id())) {
      if (stats->hp_ > 0) {
        return false;
      }
    }
  }

  return has_members;
}

void PartyData::revitalize_members() {
  using fl::ecs::components::Stats;

  for (const auto& member : members_) {
    if (auto* stats = party_ctx_.reg().try_get<Stats>(member.member_id())) {
      const auto old_hp = stats->hp_;
      stats->hp_ =
          fl::ecs::systems::effective_combat_stats(party_ctx_.reg(), member.member_id()).max_hp;
      party_bus_.emit(
          fl::events::PartyEvent{fl::events::PartyHealed{member.member_id(), stats->hp_ - old_hp}});
    }
  }
}

void PartyData::start_town_penalty() {
  town_penalty_beats_remaining_ = kTownPenaltyBeats;
}

void PartyData::leave_combat() {
  if (in_raid() || !encounter_data_ || leaving_combat_) {
    return;
  }
  leaving_combat_ = true;

  if (!all_members_dead()) {
    fl::systems::GrantXPToParty::commit_victory(party_ctx_, *encounter_data_);
    party_bus_.emit(fl::events::PartyEvent{fl::events::PartyVictory{}});
  }

  cleanup_encounter();
  leaving_combat_ = false;
}

void PartyData::return_to_town() {
  if (in_raid() || leaving_combat_) {
    return;
  }
  leaving_combat_ = true;

  party_bus_.emit(fl::events::PartyEvent{fl::events::PartyReturnedToTown{}});
  if (encounter_data_) {
    cleanup_encounter();
  }
  restore_observe_to_open_first_slots();
  log_->append_plain("Returned to town.");

  leaving_combat_ = false;
}

void PartyData::restore_observe_to_open_first_slots() {
  using fl::ecs::components::PartyMember;

  const fl::skills::SkillKey observe{fl::skills::SkillId::Observe};
  for (auto& member : members_) {
    auto* party_member = party_ctx_.reg().try_get<PartyMember>(member.member_id());
    if (party_member == nullptr || party_member->closet().skill_slots[0].has_value()) {
      continue;
    }
    member.grimoire().learn(observe);
    party_member->closet().skill_slots[0] = observe;
  }
}

void PartyData::summon_to_raid() {
  if (in_raid() || !encounter_data_ || leaving_combat_) return;
  leaving_combat_ = true;
  party_bus_.emit(fl::events::PartyEvent{fl::events::PartySummonedToRaid{}});
  cleanup_encounter();
  leaving_combat_ = false;
}

void PartyData::cleanup_encounter() {
  // Status lifetime listeners require the old encounter/scheduler to remain
  // alive while they disconnect and clear their owned effect entities.
  party_bus_.emit(fl::events::PartyEvent{fl::events::PartyLeftCombat{}});
  clear_combat_visuals();
  encounter_data_->clear_pending_events();
  encounter_data_->finalize();
  encounter_data_.reset();
  apply_pending_tower_destination();
  apply_pending_farming_plan();
  party_bus_.emit(fl::events::PartyEvent{fl::events::PartyEncounterBoundary{party_id_}});
}

void PartyData::clear_combat_visuals() {
  using namespace fl::ecs::components;
  for (const auto& member : members_) {
    auto entity = member.member_id();
    if (!party_ctx_.reg().valid(entity)) continue;
    party_ctx_.reg()
        .remove<DamageFlash, ActiveGlow, CombatantDecals, CombatantUnderlayDecals, StatusTint,
                ColorOverride, ResolvedColorOverride, ResolvedHPBarColorOverride,
                ResolvedBackgroundColorOverride>(entity);
  }
}

void PartyData::join_raid(EncounterData& encounter) {
  summon_to_raid();
  raid_encounter_ = &encounter;
}

void PartyData::resolve_raid(bool victory) {
  if (!in_raid()) return;
  party_bus_.emit(fl::events::PartyEvent{fl::events::PartyRaidResolved{victory}});
  if (victory) party_bus_.emit(fl::events::PartyEvent{fl::events::PartyVictory{}});
  party_bus_.emit(fl::events::PartyEvent{fl::events::PartyLeftCombat{}});
  raid_encounter_ = nullptr;
  clear_combat_visuals();
  party_loop_machine_ = std::make_unique<fl::fsm::PartyLoopMachine>(party_ctx_);
  if (all_members_dead()) party_bus_.emit(fl::events::PartyEvent{fl::events::PartyWiped{}});
}

void PartyData::watch_skill_learned_this_combat(entt::entity member, fl::skills::SkillKey skill) {
  if (!in_combat()) {
    return;
  }

  pending_learned_skills_.push_back(PendingLearnedSkill{.member = member, .skill = skill});
  auto it = std::prev(pending_learned_skills_.end());

  it->wipe_sub =
      fl::events::ScopedPartyListener{party_bus_, std::in_place_type<fl::events::PartyWiped>,
                                      [this, it](const fl::events::PartyWiped&) {
                                        if (in_raid()) return;
                                        resolve_pending_learned_skill(it, false);
                                      }};

  it->raid_resolved_sub =
      fl::events::ScopedPartyListener{party_bus_, std::in_place_type<fl::events::PartyRaidResolved>,
                                      [this, it](const fl::events::PartyRaidResolved& event) {
                                        resolve_pending_learned_skill(it, event.victory);
                                      }};

  it->summoned_sub = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartySummonedToRaid>,
      [this, it](const fl::events::PartySummonedToRaid&) {
        resolve_pending_learned_skill(it, true);
      }};

  it->victory_sub = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyVictory>,
      [this, it](const fl::events::PartyVictory&) { resolve_pending_learned_skill(it, true); }};

  it->returned_to_town_sub = fl::events::ScopedPartyListener{
      party_bus_, std::in_place_type<fl::events::PartyReturnedToTown>,
      [this, it](const fl::events::PartyReturnedToTown&) {
        resolve_pending_learned_skill(it, false);
      }};
}

bool PartyData::drop_skill(entt::entity member, std::size_t slot_index) {
  using fl::ecs::components::PartyMember;

  auto* party_member = party_ctx_.reg().try_get<PartyMember>(member);
  if (party_member == nullptr || slot_index == 0 ||
      slot_index >= party_member->closet().skill_slots.size()) {
    return false;
  }

  auto& slot = party_member->closet().skill_slots[slot_index];
  if (!slot.has_value()) {
    return false;
  }

  const auto skill = *slot;
  slot.reset();
  party_member->member_data().grimoire().unlearn(skill);
  log_->append_markup(fmt::format("[player_name]({}) dropped [ability]({}).",
                                  party_member->member_data().name(),
                                  fl::skills::display_name(skill)));
  return true;
}

std::optional<PartyData::TeacherEncounter>
PartyData::find_accessible_teacher(entt::entity member, fl::skills::SkillKey skill) const {
  if (!members_.contains(member)) return std::nullopt;

  std::optional<TeacherEncounter> best;
  for (const auto biome : kOriginBiomeIds) {
    const int accessible = latest_tower_floor(biome);
    for (int floor = 1; floor <= accessible; ++floor) {
      const auto* lineup = fl::primitives::generated_content::floor_lineup(biome, floor);
      if (lineup == nullptr) continue;
      for (std::size_t index = 0; index < lineup->monster_count; ++index) {
        const auto monster = lineup->monsters[index];
        const auto skills = fl::monster::generated_content::known_skills(monster);
        if (std::ranges::find(skills, skill.base) == skills.end()) continue;
        if (!best || floor > best->floor)
          best = TeacherEncounter{std::string(biome), floor, monster};
      }
    }
  }
  return best;
}

bool PartyData::farm_for_forgotten_skill(entt::entity member, fl::skills::SkillKey skill) {
  if (!members_.contains(member)) return false;
  const auto data = std::ranges::find_if(
      members_, [member](const MemberData& candidate) { return candidate.member_id() == member; });
  if (data == members_.end() || !data->grimoire().forgotten(skill)) return false;
  const auto teacher = find_accessible_teacher(member, skill);
  if (!teacher) {
    log_->append_markup(fmt::format("No accessible teacher can demonstrate [ability]({}).",
                                    fl::skills::display_name(skill)));
    return false;
  }
  log_->append_markup(fmt::format("Farming {} floor {} for [ability]({}); learning still requires "
                                  "Observe.",
                                  teacher->biome_id, teacher->floor,
                                  fl::skills::display_name(skill)));
  return request_tower_destination(teacher->biome_id, teacher->floor, true);
}

void PartyData::resolve_pending_learned_skill(std::list<PendingLearnedSkill>::iterator it,
                                              bool keep_skill) {
  if (it == pending_learned_skills_.end()) {
    return;
  }

  auto member = it->member;
  auto skill = it->skill;

  it->wipe_sub.reset();
  it->victory_sub.reset();
  it->returned_to_town_sub.reset();
  it->summoned_sub.reset();
  it->raid_resolved_sub.reset();
  pending_learned_skills_.erase(it);

  if (keep_skill) {
    return;
  }

  auto* party_member = party_ctx_.reg().try_get<fl::ecs::components::PartyMember>(member);
  if (party_member != nullptr) {
    party_member->member_data().grimoire().unlearn(skill);
    party_member->closet().unequip_skill(skill);
  }

  if (auto* stats = party_ctx_.reg().try_get<fl::ecs::components::Stats>(member)) {
    log_->append_markup(fmt::format("[player_name]({}) couldn't quite figure out [ability]({})",
                                    stats->name_, fl::skills::display_name(skill)));
  }
}

void PartyData::select_grimoire_discipline(GrimoireDiscipline discipline) {
  grimoire_discipline_ = discipline;
  if (!farm_focus_selected_) {
    farming_plan_ =
        make_farming_plan(grimoire_discipline_, farming_choice_advice().recommended_focus);
  }
  log_->append_markup(fmt::format("Starting grimoire discipline set: [ability]({}).",
                                  display_name(grimoire_discipline_)));
}

FarmingChoiceAdvice PartyData::farming_choice_advice() const noexcept {
  return make_farming_choice_advice(grimoire_discipline_, previous_farm_focus_);
}

void PartyData::apply_recommended_farming_plan() {
  const auto advice = farming_choice_advice();
  select_farming_plan(advice.discipline, advice.recommended_focus);
}

void PartyData::select_farming_plan(GrimoireDiscipline discipline, FarmFocus focus) {
  const auto plan = make_farming_plan(discipline, focus);
  party_bus_.emit(fl::events::PartyEvent{
      fl::events::FarmingTargetRequested{std::string(display_name(plan.focus))}});
  if (has_encounter()) {
    pending_farming_plan_ = plan;
    return;
  }
  commit_farming_plan(plan);
}

void PartyData::commit_farming_plan(FarmingPlan plan) {
  if (farm_focus_selected_) {
    previous_farm_focus_ = farming_plan_.focus;
  }
  grimoire_discipline_ = plan.discipline;
  farming_plan_ = plan;
  farm_focus_selected_ = true;
  log_->append_markup(
      fmt::format("Farming focus set: [ability]({}) discipline, [ability]({}) focus; "
                  "reward path is [xp]({}).",
                  display_name(farming_plan_.discipline), display_name(farming_plan_.focus),
                  display_name(farming_plan_.reward_class)));
  party_bus_.emit(fl::events::PartyEvent{
      fl::events::FarmingTargetChanged{std::string(display_name(farming_plan_.focus))}});
}

void PartyData::apply_pending_farming_plan() {
  if (!pending_farming_plan_) return;
  const auto plan = *pending_farming_plan_;
  pending_farming_plan_.reset();
  commit_farming_plan(plan);
}

void PartyData::tick_town_penalty() {
  if (town_penalty_beats_remaining_ > 0) {
    --town_penalty_beats_remaining_;
  }
}

EncounterData& PartyData::create_encounter() {
  /*log_->append_markup(fmt::format("Party id={} encounter_data={}",
                                   (int)party_id_,
                                   static_cast<void *>(encounter_data_.get())));
                                   */
  encounter_data_ = std::make_unique<EncounterData>(&party_ctx_);
  encounter_data_->innervate_event_system();

  return *encounter_data_;
}

void PartyData::add_item(entt::entity item) {
  inventory_.push_back(item);
}

std::span<const entt::entity> PartyData::items() const noexcept {
  return inventory_;
}

void PartyData::replace_items(std::vector<entt::entity> items) {
  inventory_ = std::move(items);
}
} // namespace fl::primitives
