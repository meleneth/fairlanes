#include "fl/primitives/account_data.hpp"
#include "fl/generated/monster_content.hpp"

namespace fl::primitives {

void AccountData::hook_to_beat(fl::context::AccountCtx ctx,
                               seerin::BeatBus &beats, WorldClock &world_clock,
                               bool enable_visitors) {
  progression_sub_ = beats.subscribe<seerin::Beat>(
      [this, ctx, &world_clock, enable_visitors](const seerin::Beat &) {
        calendar_->set_beat_rate_multiplier(world_clock.beat_rate_multiplier());
        advance_beat(ctx, enable_visitors);
      });
  // Day zero is a real arrival, before ordinary party progression starts.
  if (enable_visitors) try_start_visitor(ctx);
}

bool AccountData::try_start_visitor(fl::context::AccountCtx ctx) {
  if (in_raid() || calendar_->elapsed_beats() < next_visitor_beat_) return false;
  if (!start_raid(ctx, fl::monster::generated_content::raid_bosses())) return false;
  // Consume this occurrence; elapsed arrivals never become banked attempts.
  next_visitor_beat_ =
      (calendar_->elapsed_beats() / kVisitorPeriodBeats + 1) * kVisitorPeriodBeats;
  return true;
}

void AccountData::advance_beat(fl::context::AccountCtx ctx, bool enable_visitors) {
  if (in_raid()) {
    raid_->tick();
    return;
  }
  if (enable_visitors && try_start_visitor(ctx)) return;
  calendar_->advance_beat();
  if (enable_visitors && try_start_visitor(ctx)) return;
  for (auto &party : parties_) party.advance_beat();
}

} // namespace fl::primitives
