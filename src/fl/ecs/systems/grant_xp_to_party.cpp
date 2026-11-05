#include "fmt/format.h"
#include <entt/entity/handle.hpp>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/components/track_xp.hpp"
#include "fl/ecs/systems/experience_award_policy.hpp"
#include "fl/primitives/encounter_data.hpp"
#include "fl/widgets/fancy_log.hpp"

#include "grant_xp_to_party.hpp"

namespace fl::systems {
void GrantXPToParty::commit_victory(const fl::context::PartyCtx& ctx,
                                    const fl::primitives::EncounterData& encounter) {
  using namespace fl::ecs::components;
  constexpr int kBaseRewardPerEnemy = 256;
  const int base_reward =
      static_cast<int>(encounter.attackers().members().size()) * kBaseRewardPerEnemy;
  std::vector<int> enemy_levels;
  enemy_levels.reserve(encounter.attackers().members().size());
  for (const auto enemy : encounter.attackers().members()) {
    if (const auto* track = ctx.reg().try_get<TrackXP>(enemy))
      enemy_levels.push_back(track->level_);
  }

  for (const auto entity : encounter.defenders().members()) {
    auto* member = ctx.reg().try_get<PartyMember>(entity);
    auto* track = ctx.reg().try_get<TrackXP>(entity);
    auto* stats = ctx.reg().try_get<Stats>(entity);
    if (member == nullptr || track == nullptr || stats == nullptr || stats->hp_ <= 0 ||
        member->party_ != ctx.self())
      continue;
    const int amount =
        ExperienceAwardPolicy::for_character(base_reward, track->level_, enemy_levels, 0);
    entt::handle handle{ctx.reg(), entity};
    track->add_xp(handle, amount);
    ctx.bus().emit(fl::events::PartyEvent{fl::events::PartyGainedXP{entity, amount}});
    ctx.log().append_markup(fmt::format("[name]({}) received [xp]({}) XP.", stats->name_, amount));
  }
}

} // namespace fl::systems
