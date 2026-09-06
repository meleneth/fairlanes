#pragma once

#include "fl/skills/skill.hpp"
#include "fl/targeting/possible_targets.hpp"
#include "fl/targeting/status_filters.hpp"
#include <entt/entt.hpp>
#include <functional>
#include <optional>
#include <span>

namespace fl::monster {

enum class RuleTarget { Self, Ally, Enemy };
enum class RuleSubject { Actor, Target };
enum class RulePredicate { HasStatus, MissingStatus, HpBelow, HpAbove };
using RuleStatus = fl::targeting::TargetStatus;

struct RuleCondition {
  RuleSubject subject{RuleSubject::Target};
  RulePredicate predicate{RulePredicate::HasStatus};
  RuleStatus status{RuleStatus::None};
  int percent{0};
};

struct DecisionRule {
  fl::skills::SkillId skill;
  RuleTarget target{RuleTarget::Enemy};
  int chance_percent{100};
  std::span<const RuleCondition> conditions;
};

struct SkillDecision {
  fl::skills::SkillKey skill;
  entt::entity target{entt::null};
};

bool has_rule_status(entt::registry &reg, entt::entity entity,
                     RuleStatus status);

std::optional<SkillDecision> evaluate_rules(
    entt::registry &reg, entt::entity actor, fl::targeting::TargetRange allies,
    fl::targeting::TargetRange enemies, std::span<const DecisionRule> rules,
    const std::function<int()> &roll_percent);

// The first passing rule wins. Conditions are ANDed on the same candidate.
// Roll once per eligible rule, never once per target. Empty/no match means
// wait.
std::optional<SkillDecision>
evaluate_rules(entt::registry &reg, entt::entity actor,
               std::span<const entt::entity> allies,
               std::span<const entt::entity> enemies,
               std::span<const DecisionRule> rules,
               const std::function<int()> &roll_percent);

} // namespace fl::monster
