#pragma once

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/skills/skill.hpp"

namespace fl::skills {

bool learn_observed_skill_with_roll(fl::context::PartyCtx &party_ctx,
                                    entt::entity observer, entt::entity user,
                                    SkillId skill, int roll);

bool maybe_teach_observed_skill(fl::context::PartyCtx &party_ctx,
                                entt::entity observer, entt::entity user,
                                SkillId skill);

void teach_party_from_observed_skill(fl::context::PartyCtx &party_ctx,
                                     entt::entity user, SkillId skill);

} // namespace fl::skills
