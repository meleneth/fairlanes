#include <entt/entity/handle.hpp>
#include <algorithm>
#include <cmath>
#include <random>

#include "fmt/format.h"

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/effective_combat_stats.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "thump.hpp"

using namespace fl::skills;

namespace {

int thump_rank_damage_bonus(SkillKey skill) noexcept {
  if (skill.base != SkillId::Thump) {
    return 0;
  }

  return (skill.rank.value() - SkillRank::kMin) * 3;
}

} // namespace

template <typename T>
std::string exprtk_error_report(const exprtk::parser<T> &parser,
                                const std::string &expr_src) {
  using error_t = exprtk::parser_error::type;

  std::string report;
  for (std::size_t i = 0; i < parser.error_count(); ++i) {
    const error_t err = parser.get_error(i);
    const auto pos = static_cast<std::size_t>(err.token.position);

    std::string caret(expr_src.size(), ' ');
    if (pos < caret.size()) {
      caret[pos] = '^';
    }

    report += fmt::format("ExprTk error {}\n"
                          "  Type    : [{}]\n"
                          "  Message : {}\n"
                          "  Position: {}\n"
                          "  Source  : {}\n"
                          "             {}\n",
                          i, exprtk::parser_error::to_str(err.mode),
                          err.diagnostic, pos, expr_src, caret);
  }
  return report;
}

Thump::Thump() {
  // Bind variables to the symbol table
  sym_.add_variable("wd_min", wd_min_);
  sym_.add_variable("wd_max", wd_max_);
  sym_.add_variable("hit_rate", hit_rate_);
  sym_.add_variable("crit_rate", crit_rate_);
  sym_.add_variable("u_roll", u_roll_);
  sym_.add_variable("u_hit", u_hit_);
  sym_.add_variable("u_crit", u_crit_);
  sym_.add_variable("skill_mult", skill_mult_);
  sym_.add_variable("damage", damage_);
  sym_.add_constants();

  expr_.register_symbol_table(sym_);

  const std::string expr_src =
      "var base := (wd_min + (wd_max - wd_min) * u_roll) * skill_mult;"
      "var hit  := (u_hit  < hit_rate);"
      "var crit := (u_crit < crit_rate);"
      "damage := hit ? (base * (1 + crit)) : 0;";

  if (!parser_.compile(expr_src, expr_)) {
    throw std::runtime_error("ExprTk compile failed for Thump\n" +
                             exprtk_error_report(parser_, expr_src));
  }
}

int Thump::thump(fl::context::AttackCtx &&ctx, SkillKey skill) {
  // Weapon range (placeholder values; wire to your actual weapon data)
  wd_min_ = 1.0;
  wd_max_ = 5.0;

  // If you removed the external one-handed modifier, set sane defaults here
  hit_rate_ = 1.00;   // keep the first ranked slice deterministic
  crit_rate_ = 0.00;  // rank differences should be visible without crit noise
  skill_mult_ = 1.00; // no extra multiplier

  // Random uniforms in [0,1)
  auto random = ctx.rng().stream("combat/thump", entt::to_integral(ctx.attacker()));
  u_roll_ = std::generate_canonical<double, 53>(random.eng());
  u_hit_ = std::generate_canonical<double, 53>(random.eng());
  u_crit_ = std::generate_canonical<double, 53>(random.eng());

  // Evaluate expression
  double dealt = expr_.value() + thump_rank_damage_bonus(skill);

  // Round the raw packet before the shared damage path applies mitigation.
  int dmg = static_cast<int>(std::floor(dealt + 0.5));
  dmg = std::max(0, dmg);
  if (skill.base == SkillId::Thump && dmg > 0) {
    dmg = std::max(0, dmg +
                          fl::ecs::systems::effective_combat_stats(
                              ctx.reg(), ctx.attacker()).attack);
  }
  ctx.damage().physical = dmg;
  ctx.set_hit_kind(fl::primitives::HitKind::Direct);
  entt::handle attacker_h{ctx.reg(), ctx.attacker()};
  entt::handle defender_h{ctx.reg(), ctx.defender()};

  ctx.log().append_markup(fmt::format("{} used [ability]({}) on {} for "
                                      "[error]({}) damage",
                                      ctx.log().name_tag_for(attacker_h),
                                      fl::skills::display_name(skill),
                                      ctx.log().name_tag_for(defender_h), dmg));

  return fl::ecs::systems::TakeDamage::commit(ctx);
}
