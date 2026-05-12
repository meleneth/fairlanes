#include "fl/skills/skill_learning.hpp"

#include <fmt/format.h>

#include <type_traits>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::skills {

bool learn_observed_skill_with_roll(fl::context::PartyCtx &party_ctx,
                                    entt::entity observer, entt::entity user,
                                    SkillId skill, int roll) {
  if (observer == user || skill == SkillId::Observe) {
    return false;
  }

  auto &reg = party_ctx.reg();
  auto *slots = reg.try_get<fl::ecs::components::SkillSlots>(observer);
  auto *stats = reg.try_get<fl::ecs::components::Stats>(observer);
  if (slots == nullptr || stats == nullptr ||
      !reg.any_of<fl::ecs::components::PartyMember>(observer) ||
      !stats->is_alive() || slots->knows(skill) || !slots->has_open_slot()) {
    return false;
  }

  const int chance = learn_chance_percent(skill);
  if (chance <= 0 || roll > chance) {
    return false;
  }

  if (!slots->learn(skill)) {
    return false;
  }

  party_ctx.log().append_markup(fmt::format(
      "[player_name]({}) learned [ability]({})!", stats->name_, name(skill)));
  return true;
}

bool maybe_teach_observed_skill(fl::context::PartyCtx &party_ctx,
                                entt::entity observer, entt::entity user,
                                SkillId skill) {
  auto rs = party_ctx.rng().stream(
      "encounter/learn-skill",
      static_cast<std::underlying_type_t<entt::entity>>(observer));
  return learn_observed_skill_with_roll(party_ctx, observer, user, skill,
                                        rs.uniform_int<int>(1, 100));
}

void teach_party_from_observed_skill(fl::context::PartyCtx &party_ctx,
                                     entt::entity user, SkillId skill) {
  for (const auto &member : party_ctx.party_data().members()) {
    maybe_teach_observed_skill(party_ctx, member.member_id(), user, skill);
  }
}

} // namespace fl::skills
