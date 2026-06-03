#include "fl/skills/skill_learning.hpp"

#include <fmt/format.h>

#include <optional>
#include <type_traits>

#include "fl/ecs/components/party_member.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/skills/grimoire.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::skills {
namespace {

struct ObserveLearningCtx {
  fl::context::PartyCtx &party_ctx;
  entt::entity observer{entt::null};
  entt::entity user{entt::null};
  SkillKey skill{SkillId::Observe};
  int roll{0};
  fl::ecs::components::Stats *stats{};
  fl::ecs::components::PartyMember *party_member{};
};

std::optional<SkillRank> highest_equipped_observe_rank(
    const fl::ecs::components::PartyMember &party_member) noexcept {
  auto highest = std::optional<SkillRank>{};
  for (const auto equipped : party_member.closet().skill_slots) {
    if (!equipped.has_value() || equipped->base != SkillId::Observe) {
      continue;
    }

    if (!highest.has_value() || highest.value() < equipped->rank) {
      highest = equipped->rank;
    }
  }
  return highest;
}

bool knows_previous_observe_rank(const Grimoire &grimoire,
                                 SkillRank rank) noexcept {
  if (rank.value() == SkillRank::kMin) {
    return true;
  }

  return grimoire.knows(
      SkillKey{SkillId::Observe, SkillRank::require(rank.value() - 1)});
}

bool observe_progression_would_skip(
    const fl::ecs::components::PartyMember &party_member,
    SkillKey skill) noexcept {
  const auto equipped_rank = highest_equipped_observe_rank(party_member);
  if (!equipped_rank.has_value()) {
    return false;
  }

  const auto &grimoire = party_member.member_data().grimoire();
  return skill.rank.value() > equipped_rank->value() + 1 ||
         !knows_previous_observe_rank(grimoire, skill.rank);
}

bool observed_skill_exceeds_observe_rank(
    const fl::ecs::components::PartyMember &party_member,
    SkillKey skill) noexcept {
  const auto equipped_rank = highest_equipped_observe_rank(party_member);
  return equipped_rank.has_value() && !(skill.rank <= *equipped_rank);
}

bool roll_failed(SkillKey skill, int roll) noexcept {
  const int chance = learn_chance_percent(skill);
  return chance <= 0 || roll > chance;
}

void append_blocked_learning_log(fl::context::PartyCtx &party_ctx,
                                 const fl::ecs::components::Stats &stats,
                                 SkillKey skill,
                                 LearnObservedSkillResult result) {
  switch (result) {
  case LearnObservedSkillResult::NoObserveEquipped:
    party_ctx.log().append_markup(fmt::format(
        "[player_name]({}) couldn't study [ability]({}): no Observe equipped.",
        stats.name_, display_name(skill)));
    return;
  case LearnObservedSkillResult::InsufficientObserveRank:
    party_ctx.log().append_markup(
        fmt::format("[player_name]({}) couldn't study [ability]({}): Observe "
                    "rank is too low.",
                    stats.name_, display_name(skill)));
    return;
  case LearnObservedSkillResult::ObserveProgressionSkip:
    party_ctx.log().append_markup(
        fmt::format("[player_name]({}) couldn't study [ability]({}): Observe "
                    "ranks must be learned in order.",
                    stats.name_, display_name(skill)));
    return;
  case LearnObservedSkillResult::Learned:
  case LearnObservedSkillResult::SameEntity:
  case LearnObservedSkillResult::MissingObserver:
  case LearnObservedSkillResult::ObserverDead:
  case LearnObservedSkillResult::AlreadyKnown:
  case LearnObservedSkillResult::RollFailed:
    return;
  }
}

LearnObservedSkillResult
evaluate_observed_skill_learning(const ObserveLearningCtx &ctx) noexcept {
  if (ctx.observer == ctx.user) {
    return LearnObservedSkillResult::SameEntity;
  }

  if (ctx.stats == nullptr || ctx.party_member == nullptr) {
    return LearnObservedSkillResult::MissingObserver;
  }

  if (!ctx.stats->is_alive()) {
    return LearnObservedSkillResult::ObserverDead;
  }

  const auto &grimoire = ctx.party_member->member_data().grimoire();
  if (grimoire.knows(ctx.skill)) {
    return LearnObservedSkillResult::AlreadyKnown;
  }

  const bool observes_observe = ctx.skill.base == SkillId::Observe;
  const auto observe_rank = highest_equipped_observe_rank(*ctx.party_member);
  if (!observe_rank.has_value()) {
    return LearnObservedSkillResult::NoObserveEquipped;
  }

  if (observes_observe) {
    if (observe_progression_would_skip(*ctx.party_member, ctx.skill)) {
      return LearnObservedSkillResult::ObserveProgressionSkip;
    }
    return LearnObservedSkillResult::Learned;
  }

  if (observed_skill_exceeds_observe_rank(*ctx.party_member, ctx.skill)) {
    return LearnObservedSkillResult::InsufficientObserveRank;
  }

  if (roll_failed(ctx.skill, ctx.roll)) {
    return LearnObservedSkillResult::RollFailed;
  }

  return LearnObservedSkillResult::Learned;
}

LearnObservedSkillResult
apply_observed_skill_learning(ObserveLearningCtx &ctx) {
  auto &grimoire = ctx.party_member->member_data().grimoire();
  if (!grimoire.learn(ctx.skill)) {
    return LearnObservedSkillResult::AlreadyKnown;
  }

  if (ctx.party_member->closet().has_open_skill_slot()) {
    (void)ctx.party_member->equip_known_skill(ctx.skill);
  }

  ctx.party_ctx.party_data().watch_skill_learned_this_combat(ctx.observer,
                                                             ctx.skill);

  ctx.party_ctx.log().append_markup(
      fmt::format("[player_name]({}) learned [ability]({})!", ctx.stats->name_,
                  display_name(ctx.skill)));
  return LearnObservedSkillResult::Learned;
}

} // namespace

LearnObservedSkillResult
learn_observed_skill_result_with_roll(fl::context::PartyCtx &party_ctx,
                                      entt::entity observer, entt::entity user,
                                      SkillKey skill, int roll) {
  auto &reg = party_ctx.reg();
  ObserveLearningCtx ctx{
      .party_ctx = party_ctx,
      .observer = observer,
      .user = user,
      .skill = skill,
      .roll = roll,
      .stats = reg.try_get<fl::ecs::components::Stats>(observer),
      .party_member = reg.try_get<fl::ecs::components::PartyMember>(observer),
  };

  const auto result = evaluate_observed_skill_learning(ctx);
  if (result != LearnObservedSkillResult::Learned) {
    if (ctx.stats != nullptr) {
      append_blocked_learning_log(party_ctx, *ctx.stats, skill, result);
    }
    return result;
  }

  return apply_observed_skill_learning(ctx);
}

bool learn_observed_skill_with_roll(fl::context::PartyCtx &party_ctx,
                                    entt::entity observer, entt::entity user,
                                    SkillKey skill, int roll) {
  return learn_observed_skill_result_with_roll(party_ctx, observer, user, skill,
                                               roll) ==
         LearnObservedSkillResult::Learned;
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
