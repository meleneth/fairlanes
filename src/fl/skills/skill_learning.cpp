#include "fl/skills/skill_learning.hpp"

#include <fmt/format.h>

#include <boost/sml.hpp>

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
namespace sml = boost::sml;

struct EvaluateLearning {};
struct Idle {};
struct Done {};

struct ObserveLearningCtx {
  fl::context::PartyCtx &party_ctx;
  entt::entity observer{entt::null};
  entt::entity user{entt::null};
  SkillKey skill{SkillId::Observe};
  int roll{0};
  fl::ecs::components::Stats *stats{};
  fl::ecs::components::PartyMember *party_member{};
  LearnObservedSkillResult result{LearnObservedSkillResult::RollFailed};
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

struct ObserveLearning {
  struct Ops {
    static bool same_entity(const ObserveLearningCtx &ctx) noexcept {
      return ctx.observer == ctx.user;
    }

    static bool missing_observer(const ObserveLearningCtx &ctx) noexcept {
      return ctx.stats == nullptr || ctx.party_member == nullptr;
    }

    static bool observer_dead(const ObserveLearningCtx &ctx) noexcept {
      return !ctx.stats->is_alive();
    }

    static bool already_known(const ObserveLearningCtx &ctx) noexcept {
      return ctx.party_member->member_data().grimoire().knows(ctx.skill);
    }

    static bool observes_observe(const ObserveLearningCtx &ctx) noexcept {
      return ctx.skill.base == SkillId::Observe;
    }

    static bool observes_non_observe(const ObserveLearningCtx &ctx) noexcept {
      return ctx.skill.base != SkillId::Observe;
    }

    static bool no_observe_equipped(const ObserveLearningCtx &ctx) noexcept {
      return !highest_equipped_observe_rank(*ctx.party_member).has_value();
    }

    static bool
    observe_progression_would_skip(const ObserveLearningCtx &ctx) noexcept {
      const auto equipped_rank =
          highest_equipped_observe_rank(*ctx.party_member);
      if (!equipped_rank.has_value()) {
        return false;
      }

      const auto &grimoire = ctx.party_member->member_data().grimoire();
      return ctx.skill.rank.value() > equipped_rank->value() + 1 ||
             !knows_previous_observe_rank(grimoire, ctx.skill.rank);
    }

    static bool observed_skill_exceeds_observe_rank(
        const ObserveLearningCtx &ctx) noexcept {
      const auto equipped_rank =
          highest_equipped_observe_rank(*ctx.party_member);
      return equipped_rank.has_value() && !(ctx.skill.rank <= *equipped_rank);
    }

    static bool roll_failed(const ObserveLearningCtx &ctx) noexcept {
      const int chance = learn_chance_percent(ctx.skill);
      return chance <= 0 || ctx.roll > chance;
    }

    static void set_same_entity(ObserveLearningCtx &ctx) noexcept {
      ctx.result = LearnObservedSkillResult::SameEntity;
    }

    static void set_missing_observer(ObserveLearningCtx &ctx) noexcept {
      ctx.result = LearnObservedSkillResult::MissingObserver;
    }

    static void set_observer_dead(ObserveLearningCtx &ctx) noexcept {
      ctx.result = LearnObservedSkillResult::ObserverDead;
    }

    static void set_already_known(ObserveLearningCtx &ctx) noexcept {
      ctx.result = LearnObservedSkillResult::AlreadyKnown;
    }

    static void block_no_observe(ObserveLearningCtx &ctx) {
      ctx.result = LearnObservedSkillResult::NoObserveEquipped;
      append_blocked_learning_log(ctx.party_ctx, *ctx.stats, ctx.skill,
                                  ctx.result);
    }

    static void block_insufficient_observe(ObserveLearningCtx &ctx) {
      ctx.result = LearnObservedSkillResult::InsufficientObserveRank;
      append_blocked_learning_log(ctx.party_ctx, *ctx.stats, ctx.skill,
                                  ctx.result);
    }

    static void block_progression_skip(ObserveLearningCtx &ctx) {
      ctx.result = LearnObservedSkillResult::ObserveProgressionSkip;
      append_blocked_learning_log(ctx.party_ctx, *ctx.stats, ctx.skill,
                                  ctx.result);
    }

    static void set_roll_failed(ObserveLearningCtx &ctx) noexcept {
      ctx.result = LearnObservedSkillResult::RollFailed;
    }

    static void learn(ObserveLearningCtx &ctx) {
      auto &grimoire = ctx.party_member->member_data().grimoire();
      if (!grimoire.learn(ctx.skill)) {
        ctx.result = LearnObservedSkillResult::AlreadyKnown;
        return;
      }

      if (ctx.party_member->closet().has_open_skill_slot()) {
        (void)ctx.party_member->equip_known_skill(ctx.skill);
      }

      ctx.party_ctx.party_data().watch_skill_learned_this_combat(ctx.observer,
                                                                 ctx.skill);

      ctx.party_ctx.log().append_markup(
          fmt::format("[player_name]({}) learned [ability]({})!",
                      ctx.stats->name_, display_name(ctx.skill)));
      ctx.result = LearnObservedSkillResult::Learned;
    }
  };

  auto operator()() const {
    using namespace sml;

    const auto same_entity = [](const ObserveLearningCtx &ctx) {
      return Ops::same_entity(ctx);
    };
    const auto missing_observer = [](const ObserveLearningCtx &ctx) {
      return Ops::missing_observer(ctx);
    };
    const auto observer_dead = [](const ObserveLearningCtx &ctx) {
      return Ops::observer_dead(ctx);
    };
    const auto already_known = [](const ObserveLearningCtx &ctx) {
      return Ops::already_known(ctx);
    };
    const auto observe_no_observe = [](const ObserveLearningCtx &ctx) {
      return Ops::observes_observe(ctx) && Ops::no_observe_equipped(ctx);
    };
    const auto observe_progression_skip = [](const ObserveLearningCtx &ctx) {
      return Ops::observes_observe(ctx) &&
             Ops::observe_progression_would_skip(ctx);
    };
    const auto observe_learnable = [](const ObserveLearningCtx &ctx) {
      return Ops::observes_observe(ctx);
    };
    const auto non_observe_no_observe = [](const ObserveLearningCtx &ctx) {
      return Ops::observes_non_observe(ctx) && Ops::no_observe_equipped(ctx);
    };
    const auto non_observe_insufficient = [](const ObserveLearningCtx &ctx) {
      return Ops::observes_non_observe(ctx) &&
             Ops::observed_skill_exceeds_observe_rank(ctx);
    };
    const auto non_observe_roll_failed = [](const ObserveLearningCtx &ctx) {
      return Ops::observes_non_observe(ctx) && Ops::roll_failed(ctx);
    };

    const auto set_same_entity = [](ObserveLearningCtx &ctx) {
      Ops::set_same_entity(ctx);
    };
    const auto set_missing_observer = [](ObserveLearningCtx &ctx) {
      Ops::set_missing_observer(ctx);
    };
    const auto set_observer_dead = [](ObserveLearningCtx &ctx) {
      Ops::set_observer_dead(ctx);
    };
    const auto set_already_known = [](ObserveLearningCtx &ctx) {
      Ops::set_already_known(ctx);
    };
    const auto block_no_observe = [](ObserveLearningCtx &ctx) {
      Ops::block_no_observe(ctx);
    };
    const auto block_insufficient_observe = [](ObserveLearningCtx &ctx) {
      Ops::block_insufficient_observe(ctx);
    };
    const auto block_progression_skip = [](ObserveLearningCtx &ctx) {
      Ops::block_progression_skip(ctx);
    };
    const auto set_roll_failed = [](ObserveLearningCtx &ctx) {
      Ops::set_roll_failed(ctx);
    };
    const auto learn = [](ObserveLearningCtx &ctx) { Ops::learn(ctx); };

    return make_transition_table(
        *state<Idle> + event<EvaluateLearning>[same_entity] / set_same_entity =
            state<Done>,
        state<Idle> + event<EvaluateLearning>[missing_observer] /
                          set_missing_observer = state<Done>,
        state<Idle> + event<EvaluateLearning>[observer_dead] /
                          set_observer_dead = state<Done>,
        state<Idle> + event<EvaluateLearning>[already_known] /
                          set_already_known = state<Done>,

        state<Idle> + event<EvaluateLearning>[observe_no_observe] /
                          block_no_observe = state<Done>,
        state<Idle> + event<EvaluateLearning>[observe_progression_skip] /
                          block_progression_skip = state<Done>,
        state<Idle> + event<EvaluateLearning>[observe_learnable] / learn =
            state<Done>,

        state<Idle> + event<EvaluateLearning>[non_observe_no_observe] /
                          block_no_observe = state<Done>,
        state<Idle> + event<EvaluateLearning>[non_observe_insufficient] /
                          block_insufficient_observe = state<Done>,
        state<Idle> + event<EvaluateLearning>[non_observe_roll_failed] /
                          set_roll_failed = state<Done>,
        state<Idle> + event<EvaluateLearning> / learn = state<Done>);
  }
};

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

  boost::sml::sm<ObserveLearning> sm{ctx};
  sm.process_event(EvaluateLearning{});
  return ctx.result;
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
