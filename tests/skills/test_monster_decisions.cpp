#include <array>
#include <catch2/catch_test_macros.hpp>

#include "fl/ecs/components/combat_status.hpp"
#include "fl/ecs/components/skill_slots.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/monsters/decision_rule.hpp"

namespace {
using namespace fl::monster;
using fl::ecs::components::CombatStatusKind;
using fl::skills::SkillId;

entt::entity combatant(entt::registry &reg) {
  const auto entity = reg.create();
  reg.emplace<fl::ecs::components::Stats>(entity);
  reg.emplace<fl::ecs::components::SkillSlots>(entity);
  return entity;
}
void status(entt::registry &reg, entt::entity entity, CombatStatusKind kind) {
  auto &effect = reg.get_or_emplace<fl::ecs::components::CombatStatuses>(entity)
                     .effects.emplace_back();
  effect.kind = kind;
}
} // namespace

TEST_CASE("Monster buffs target missing allies and fall through when covered",
          "[monster-rules]") {
  entt::registry reg;
  const auto actor = combatant(reg);
  const auto ally = combatant(reg);
  const auto enemy = combatant(reg);
  auto &skills = reg.get<fl::ecs::components::SkillSlots>(actor);
  skills.learn(SkillId::CinderVeil);
  skills.learn(SkillId::Thump);
  const std::array allies{actor, ally};
  const std::array enemies{enemy};
  const std::array missing{RuleCondition{
      RuleSubject::Target, RulePredicate::MissingStatus, RuleStatus::Shield}};
  const std::array rules{
      DecisionRule{SkillId::CinderVeil, RuleTarget::Ally, 100, missing},
      DecisionRule{SkillId::Thump, RuleTarget::Enemy, 100, {}}};
  int rolls = 0;
  const auto roll = [&] {
    ++rolls;
    return 100;
  };
  status(reg, actor, CombatStatusKind::Shield);
  auto choice = evaluate_rules(reg, actor, allies, enemies, rules, roll);
  REQUIRE(choice);
  REQUIRE(choice->skill.base == SkillId::CinderVeil);
  REQUIRE(choice->target == ally);
  status(reg, ally, CombatStatusKind::Shield);
  choice = evaluate_rules(reg, actor, allies, enemies, rules, roll);
  REQUIRE(choice->skill.base == SkillId::Thump);
  REQUIRE(choice->target == enemy);
  REQUIRE(rolls == 0);
}

TEST_CASE(
    "Percentage rules roll once after eligibility and use exact boundaries",
    "[monster-rules]") {
  entt::registry reg;
  const auto actor = combatant(reg);
  const auto enemy = combatant(reg);
  auto &skills = reg.get<fl::ecs::components::SkillSlots>(actor);
  skills.learn(SkillId::Flee);
  skills.learn(SkillId::Thump);
  const std::array allies{actor};
  const std::array enemies{enemy};
  const std::array wounded{RuleCondition{
      RuleSubject::Actor, RulePredicate::HpBelow, RuleStatus::None, 50}};
  const std::array rules{
      DecisionRule{SkillId::Flee, RuleTarget::Self, 30, wounded},
      DecisionRule{SkillId::Thump, RuleTarget::Enemy, 100, {}}};
  int rolls = 0;
  int value = 30;
  const auto roll = [&] {
    ++rolls;
    return value;
  };
  REQUIRE(
      evaluate_rules(reg, actor, allies, enemies, rules, roll)->skill.base ==
      SkillId::Thump);
  REQUIRE(rolls == 0);
  reg.get<fl::ecs::components::Stats>(actor).hp_ = 1;
  REQUIRE(
      evaluate_rules(reg, actor, allies, enemies, rules, roll)->skill.base ==
      SkillId::Flee);
  REQUIRE(rolls == 1);
  value = 31;
  REQUIRE(
      evaluate_rules(reg, actor, allies, enemies, rules, roll)->skill.base ==
      SkillId::Thump);
  REQUIRE(rolls == 2);
}

TEST_CASE("Combo guards select the same living enemy that carries the status",
          "[monster-rules]") {
  entt::registry reg;
  const auto actor = combatant(reg);
  const auto clean = combatant(reg);
  const auto burning = combatant(reg);
  auto &skills = reg.get<fl::ecs::components::SkillSlots>(actor);
  skills.learn(SkillId::FlameStrike);
  const std::array allies{actor};
  const std::array enemies{clean, burning};
  const std::array conditions{
      RuleCondition{RuleSubject::Target, RulePredicate::HasStatus,
                    RuleStatus::Burn},
      RuleCondition{RuleSubject::Target, RulePredicate::HpBelow,
                    RuleStatus::None, 80}};
  const std::array rules{
      DecisionRule{SkillId::FlameStrike, RuleTarget::Enemy, 100, conditions}};
  const auto roll = [] { return 1; };
  REQUIRE_FALSE(evaluate_rules(reg, actor, allies, enemies, rules, roll));
  status(reg, burning, CombatStatusKind::Burn);
  REQUIRE_FALSE(evaluate_rules(reg, actor, allies, enemies, rules, roll));
  reg.get<fl::ecs::components::Stats>(burning).hp_ = 1;
  REQUIRE(evaluate_rules(reg, actor, allies, enemies, rules, roll)->target ==
          burning);
  status(reg, actor, CombatStatusKind::Silence);
  REQUIRE_FALSE(evaluate_rules(reg, actor, allies, enemies, rules, roll));
  reg.remove<fl::ecs::components::CombatStatuses>(actor);
  reg.get<fl::ecs::components::Stats>(burning).hp_ = 0;
  REQUIRE_FALSE(evaluate_rules(reg, actor, allies, enemies, rules, roll));
}

TEST_CASE("Disabled and unavailable rules do not roll or invent skills",
          "[monster-rules]") {
  entt::registry reg;
  const auto actor = combatant(reg);
  const std::array allies{actor};
  const std::array rules{
      DecisionRule{SkillId::Flee, RuleTarget::Self, 50, {}},
      DecisionRule{SkillId::Observe, RuleTarget::Self, 0, {}}};
  reg.get<fl::ecs::components::SkillSlots>(actor).learn(SkillId::Observe);
  REQUIRE_FALSE(evaluate_rules(reg, actor, allies, {}, rules, [] {
    FAIL("ineligible rules must not roll");
    return 1;
  }));
}

TEST_CASE("Monster rule evaluation consumes lazy encounter candidates",
          "[monster-rules][targeting]") {
  entt::registry reg;
  auto actor = combatant(reg);
  auto wrong_status = combatant(reg);
  auto wanted = combatant(reg);
  auto dead = combatant(reg);
  reg.get<fl::ecs::components::SkillSlots>(actor).learn(SkillId::Cinderburst);
  status(reg, wanted, CombatStatusKind::Burn);
  status(reg, dead, CombatStatusKind::Burn);
  reg.get<fl::ecs::components::Stats>(dead).hp_ = 0;
  const std::array allies{actor};
  const std::array enemies{wrong_status, dead, wanted};
  fl::targeting::PossibleTargets targets{reg, allies, enemies};
  const std::array conditions{
      RuleCondition{RuleSubject::Target, RulePredicate::HasStatus,
                    RuleStatus::Burn},
      RuleCondition{RuleSubject::Target, RulePredicate::MissingStatus,
                    RuleStatus::Shield}};
  const std::array rules{
      DecisionRule{SkillId::Cinderburst, RuleTarget::Enemy, 100, conditions}};
  auto choose = [&] {
    return evaluate_rules(reg, actor, targets.FriendlyPossibleTargets(actor),
                          targets.EnemyPossibleTargets(actor), rules,
                          [] { return 1; });
  };
  REQUIRE(choose()->target == wanted);
  status(reg, wanted, CombatStatusKind::Shield);
  REQUIRE_FALSE(choose());
  reg.destroy(wanted);
  REQUIRE_FALSE(choose());
}
