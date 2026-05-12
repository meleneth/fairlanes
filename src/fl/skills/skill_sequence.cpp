#include "fl/skills/skill_sequence.hpp"

#include <fmt/format.h>

#include <string>

#include "fl/ecs/components/color_override.hpp"
#include "fl/ecs/components/hp_bar_color_override.hpp"
#include "fl/ecs/components/stats.hpp"
#include "fl/ecs/systems/dire_bleed_system.hpp"
#include "fl/lospec500.hpp"
#include "fl/skills/eviscerate.hpp"
#include "fl/skills/skill_learning.hpp"
#include "fl/skills/thump.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl::skills {

SkillSequencer::SkillSequencer(fl::context::PartyCtx &party_ctx,
                               Scheduler &scheduler, FinishTurnFn finish_turn,
                               ClearPendingFn clear_pending)
    : party_ctx_(party_ctx), scheduler_(scheduler),
      finish_turn_(std::move(finish_turn)),
      clear_pending_(std::move(clear_pending)) {}

void SkillSequencer::schedule(entt::entity attacker, entt::entity target,
                              SkillId skill) {
  switch (skill) {
  case SkillId::Eviscerate:
    schedule_eviscerate(attacker, target);
    return;
  case SkillId::Bump:
  case SkillId::Squish:
  case SkillId::Smack:
  case SkillId::Thump:
    schedule_thump_like(attacker, target, skill);
    return;
  case SkillId::Observe:
    schedule_observe(attacker);
    return;
  }
}

void SkillSequencer::schedule_thump_like(entt::entity attacker,
                                         entt::entity target, SkillId skill) {
  auto const kBg = fl::lospec500::color_at(0);
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  const auto skill_name = std::string{name(skill)};

  teach_party_from_observed_skill(party_ctx_, attacker, skill);

  schedule_reek_fade(attacker,
                     fmt::format("{}: attacker red pulse #1", skill_name), 10,
                     20, kRed, kBg);

  schedule_reek_fade(attacker,
                     fmt::format("{}: attacker red pulse #2", skill_name), 30,
                     40, kRed, kBg);

  schedule_reek_fade(target, fmt::format("{}: defender yellow hit", skill_name),
                     50, 70, kYellow, kBg);

  scheduler_.schedule_smelly_in_beats(
      60, fmt::format("{}: apply damage", skill_name),
      [&party_ctx = party_ctx_, attacker, target, skill] {
        fl::skills::Thump thump;
        thump.thump(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target),
            skill);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      71, fmt::format("{}: finish", skill_name),
      [finish_turn, attacker] { finish_turn(attacker); });
}

void SkillSequencer::schedule_eviscerate(entt::entity attacker,
                                         entt::entity target) {
  auto const kBg = fl::lospec500::color_at(0);
  auto const kRed = fl::lospec500::color_at(4);
  auto const kYellow = fl::lospec500::color_at(14);

  teach_party_from_observed_skill(party_ctx_, attacker, SkillId::Eviscerate);

  schedule_reek_fade(attacker, "eviscerate: attacker red slash", 8, 18, kRed,
                     kBg);
  schedule_reek_fade(target, "eviscerate: defender wound", 20, 32, kYellow,
                     kRed);

  scheduler_.schedule_smelly_in_beats_for(
      24, target, "eviscerate: apply dire bleed",
      [&party_ctx = party_ctx_, &scheduler = scheduler_,
       clear_pending = clear_pending_, attacker, target] {
        fl::skills::Eviscerate eviscerate;
        eviscerate.eviscerate(
            fl::context::AttackCtx::make_attack(party_ctx, attacker, target));
        fl::ecs::components::safe_add_hp_bar_color(party_ctx.reg(), target,
                                                   fl::lospec500::color_at(4));
        fl::ecs::systems::DireBleedSystem::bind_cleanup_and_schedule(
            party_ctx, scheduler, target, clear_pending);
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats_for(
      34, attacker, "eviscerate: finish",
      [finish_turn, attacker] { finish_turn(attacker); });
}

void SkillSequencer::schedule_observe(entt::entity attacker) {
  scheduler_.schedule_smelly_in_beats(
      1, "observe: log", [&party_ctx = party_ctx_, attacker] {
        auto &stats = party_ctx.reg().get<fl::ecs::components::Stats>(attacker);
        party_ctx.log().append_markup(fmt::format(
            "[player_name]({}) used [ability](Observe).", stats.name_));
      });

  auto finish_turn = finish_turn_;
  scheduler_.schedule_smelly_in_beats(
      12, "observe: finish",
      [finish_turn, attacker] { finish_turn(attacker); });
}

void SkillSequencer::schedule_reek_fade(entt::entity entity,
                                        std::string_view label, int start_beat,
                                        int end_beat, ftxui::Color from,
                                        ftxui::Color to) {
  auto const duration = end_beat - start_beat;
  if (duration <= 0) {
    return;
  }

  for (int beat = start_beat; beat <= end_beat; ++beat) {
    auto const t =
        static_cast<float>(beat - start_beat) / static_cast<float>(duration);

    auto const color = ftxui::Color::Interpolate(t, from, to);

    scheduler_.schedule_smelly_in_beats_for(
        beat, entity, fmt::format("{}: reek fade beat {}", label, beat),
        [&party_ctx = party_ctx_, entity, color] {
          fl::ecs::components::safe_add_color(party_ctx.reg(), entity, color);
        });
  }

  scheduler_.schedule_smelly_in_beats_for(
      end_beat + 1, entity, fmt::format("{}: reek fade clear", label),
      [&party_ctx = party_ctx_, entity] {
        fl::ecs::components::safe_clear_color(party_ctx.reg(), entity);
      });
}

} // namespace fl::skills
