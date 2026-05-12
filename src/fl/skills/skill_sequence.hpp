#pragma once

#include <functional>
#include <string_view>

#include <entt/entt.hpp>
#include <ftxui/screen/color.hpp>

#include "fl/context.hpp"
#include "fl/skills/skill.hpp"
#include "sr/atb_events.hpp"
#include "sr/timed_scheduler.hpp"

namespace fl::skills {

class SkillSequencer {
public:
  using Scheduler = seerin::TimedScheduler<seerin::AtbOutEvent>;
  using FinishTurnFn = std::function<void(entt::entity)>;
  using ClearPendingFn = std::function<void(entt::entity)>;

  SkillSequencer(fl::context::PartyCtx &party_ctx, Scheduler &scheduler,
                 FinishTurnFn finish_turn, ClearPendingFn clear_pending);

  void schedule(entt::entity attacker, entt::entity target, SkillId skill);

private:
  void schedule_thump_like(entt::entity attacker, entt::entity target,
                           SkillId skill);
  void schedule_eviscerate(entt::entity attacker, entt::entity target);
  void schedule_observe(entt::entity attacker);
  void schedule_reek_fade(entt::entity entity, std::string_view label,
                          int start_beat, int end_beat, ftxui::Color from,
                          ftxui::Color to);

  fl::context::PartyCtx &party_ctx_;
  Scheduler &scheduler_;
  FinishTurnFn finish_turn_;
  ClearPendingFn clear_pending_;
};

} // namespace fl::skills
