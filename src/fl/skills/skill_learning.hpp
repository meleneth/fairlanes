#pragma once

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/skills/skill.hpp"

namespace fl::skills {

enum class LearnObservedSkillResult {
  Learned,
  SameEntity,
  MissingObserver,
  ObserverDead,
  AlreadyKnown,
  NoObserveEquipped,
  InsufficientObserveRank,
  ObserveProgressionSkip,
  RollFailed,
};

LearnObservedSkillResult
learn_observed_skill_result_with_roll(fl::context::PartyCtx &party_ctx,
                                      entt::entity observer, entt::entity user,
                                      SkillKey skill, int roll);

bool learn_observed_skill_with_roll(fl::context::PartyCtx &party_ctx,
                                    entt::entity observer, entt::entity user,
                                    SkillKey skill, int roll);

bool maybe_teach_observed_skill(fl::context::PartyCtx &party_ctx,
                                entt::entity observer, entt::entity user,
                                SkillKey skill);

void teach_party_from_observed_skill(fl::context::PartyCtx &party_ctx,
                                     entt::entity user, SkillKey skill);

} // namespace fl::skills
