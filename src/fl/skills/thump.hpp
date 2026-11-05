#pragma once
#include <cmath>

#include <entt/entity/registry.hpp>
#include <exprtk.hpp>

#include "fl/skills/skill.hpp"

namespace fl::context {
struct AttackCtx;
};

namespace fl::skills {
class Thump {
public:
  Thump();

  // Apply one Thump from attacker->defender, returns damage dealt (int).
  int thump(fl::context::AttackCtx&& ctx, SkillKey skill = SkillKey{SkillId::Thump});

  // Tunables
  double hit_rate = 0.90;
  double crit_rate = 0.05;
  double skill_mult = 1.0; // per-skill scalar (start at 1.0)

private:
  using expr_t = exprtk::expression<double>;
  expr_t expr_;
  exprtk::symbol_table<double> sym_;
  exprtk::parser<double> parser_;

  // bound variables
  double wd_min_, wd_max_;
  double hit_rate_, crit_rate_;
  double u_roll_, u_hit_, u_crit_;
  double skill_mult_;
  // double one_handed_mod_;
  double damage_;
};
} // namespace fl::skills
