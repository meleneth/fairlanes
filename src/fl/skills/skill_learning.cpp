#include "fl/skills/skill_learning.hpp"

#include <fmt/format.h>

#include <type_traits>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::skills {

bool learn_observed_skill_with_roll(fl::context::PartyCtx &party_ctx,
                                    entt::entity observer, entt::entity user,
                                    SkillKey skill, int roll) {
  if (observer == user || skill.base == SkillId::Observe) {
    return false;
  }

  auto &reg = party_ctx.reg();
  auto *stats = reg.try_get<fl::ecs::components::Stats>(observer);
  auto *party_member = reg.try_get<fl::ecs::components::PartyMember>(observer);
  if (stats == nullptr || party_member == nullptr || !stats->is_alive() ||
      party_member->member_data().grimoire().knows(skill)) {
    return false;
  }

  const int chance = learn_chance_percent(skill);
  if (chance <= 0 || roll > chance) {
    return false;
  }

  if (!party_member->member_data().grimoire().learn(skill)) {
    return false;
  }

  if (party_member->closet().has_open_skill_slot()) {
    (void)party_member->closet().equip_skill(skill);
  }

  party_ctx.party_data().watch_skill_learned_this_combat(observer, skill);

  party_ctx.log().append_markup(fmt::format(
      "[player_name]({}) learned [ability]({})!", stats->name_, name(skill)));
  return true;
}

bool maybe_teach_observed_skill(fl::context::PartyCtx &party_ctx,
                                entt::entity observer, entt::entity user,
                                SkillKey skill) {
  auto rs = party_ctx.rng().stream(
      "encounter/learn-skill",
      static_cast<std::underlying_type_t<entt::entity>>(observer));
  return learn_observed_skill_with_roll(party_ctx, observer, user, skill,
                                        rs.uniform_int<int>(1, 100));
}

void teach_party_from_observed_skill(fl::context::PartyCtx &party_ctx,
                                     entt::entity user, SkillKey skill) {
  for (const auto &member : party_ctx.party_data().members()) {
    maybe_teach_observed_skill(party_ctx, member.member_id(), user, skill);
  }
}

} // namespace fl::skills
