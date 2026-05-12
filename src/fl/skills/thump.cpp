#include <algorithm>
#include <cmath>
#include <random>

#include "fmt/format.h"

#include "fl/context.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/take_damage.hpp"
#include "fl/primitives/damage.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "thump.hpp"

using namespace fl::skills;

template <typename T>
void dump_exprtk_errors(const exprtk::parser<T> &parser,
                        const std::string &expr_src) {
  using error_t = exprtk::parser_error::type;

  for (std::size_t i = 0; i < parser.error_count(); ++i) {
    const error_t err = parser.get_error(i);
    const auto pos = static_cast<std::size_t>(err.token.position);

    // Build a caret indicator under the offending position.
    std::string caret(expr_src.size(), ' ');
    if (pos < caret.size())
      caret[pos] = '^';

    std::fprintf(stderr,
                 "ExprTk error %zu\n"
                 "  Type    : [%s]\n"
                 "  Message : %s\n"
                 "  Position: %zu\n"
                 "  Source  : %s\n"
                 "             %s\n",
                 i, exprtk::parser_error::to_str(err.mode).c_str(),
                 err.diagnostic.c_str(), pos, expr_src.c_str(), caret.c_str());
  }
}

Thump::Thump() : rng_(std::random_device{}()) {
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
    dump_exprtk_errors(parser_, expr_src);

    throw std::runtime_error("ExprTk compile failed for Thump");
  }
}

int Thump::thump(fl::context::AttackCtx &&ctx, SkillId skill) {
  using fl::ecs::components::Stats;
  auto &dst = ctx.reg().get<Stats>(ctx.defender());

  // Weapon range (placeholder values; wire to your actual weapon data)
  wd_min_ = 1.0;
  wd_max_ = 5.0;

  // If you removed the external one-handed modifier, set sane defaults here
  hit_rate_ = 0.90;   // 90% hit chance
  crit_rate_ = 0.05;  // 5% crit chance
  skill_mult_ = 1.00; // no extra multiplier

  // Random uniforms in [0,1)
  u_roll_ = uni_(rng_);
  u_hit_ = uni_(rng_);
  u_crit_ = uni_(rng_);

  // Evaluate expression
  double dealt = expr_.value();

  // Clamp and round: never deal more than current HP, never negative
  int hp_now = dst.hp_;
  int dmg = static_cast<int>(std::floor(dealt + 0.5));
  dmg = std::clamp(dmg, 0, hp_now);
  ctx.damage().physical = dmg;
  entt::handle attacker_h{ctx.reg(), ctx.attacker()};
  entt::handle defender_h{ctx.reg(), ctx.defender()};

  ctx.log().append_markup(fmt::format("{} used [ability]({}) on {} for "
                                      "[error]({}) damage",
                                      ctx.log().name_tag_for(attacker_h),
                                      fl::skills::name(skill),
                                      ctx.log().name_tag_for(defender_h), dmg));

  fl::ecs::systems::TakeDamage::commit(ctx);

  return dmg;
}
