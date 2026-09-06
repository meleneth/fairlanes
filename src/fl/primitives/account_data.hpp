// grand_central.hpp (or better: fl/primitives/account_data.hpp)
#pragma once

#include <deque>
#include <memory>

#include <entt/entt.hpp>

#include "fl/fwd.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"
#include "fl/primitives/raid_data.hpp"
#include "fl/primitives/moon_calendar.hpp"
#include "fl/primitives/raid_statistics.hpp"

namespace fl::primitives {

struct AccountData {
public:
  explicit AccountData(entt::entity account_id, bool record_progress = true)
      : account_id_{account_id},
        log_{std::make_unique<fl::widgets::FancyLog>()},
        statistics_{std::make_unique<RaidStatistics>(*raid_bus_, account_id, record_progress)} {}

  AccountData(AccountData &&) noexcept = default;
  AccountData &operator=(AccountData &&) noexcept = default;

  AccountData(const AccountData &) = delete;
  AccountData &operator=(const AccountData &) = delete;

  const RaidRecords &raid_records() const { return statistics_->records(); }
  RaidData *raid() { return raid_.get(); }
  const RaidData *raid() const { return raid_.get(); }
  bool in_raid() const { return raid_ && raid_->active(); }
  fl::events::RaidBus &raid_bus() { return *raid_bus_; }
  bool start_raid(fl::context::AccountCtx ctx, std::span<const fl::monster::MonsterKind> enemies) {
    if (in_raid() || parties_.empty() || enemies.empty()) return false;
    raid_.reset();
    raid_ = std::make_unique<RaidData>(ctx, *raid_bus_, ++raid_id_, enemies);
    raid_->announce_started();
    return true;
  }

  static constexpr std::uint64_t kVisitorPeriodBeats =
      MoonCalendar::kVisitorCycleDays * WorldClock::kBeatsPerCalendarDay;
  WorldClock &calendar() { return *calendar_; }
  const WorldClock &calendar() const { return *calendar_; }
  std::uint64_t beats_until_visitor() const {
    return next_visitor_beat_ > calendar_->elapsed_beats()
               ? next_visitor_beat_ - calendar_->elapsed_beats() : 0;
  }
  void hook_to_beat(fl::context::AccountCtx ctx, seerin::BeatBus &beats,
                    WorldClock &world_clock, bool enable_visitors = true);
  void advance_beat(fl::context::AccountCtx ctx, bool enable_visitors = true);

  // --- capability-style accessors ---
  entt::entity account_id() const { return account_id_; }

  fl::widgets::FancyLog &log() const {
    // FL_ASSERT(log_);
    return *log_;
  }

  std::deque<PartyData> &parties() { return parties_; }
  const std::deque<PartyData> &parties() const { return parties_; }

  PartyData &party(std::size_t idx) { return parties_.at(idx); }
  const PartyData &party(std::size_t idx) const { return parties_.at(idx); }

private:
  entt::entity account_id_{entt::null};
  std::unique_ptr<fl::widgets::FancyLog> log_{};

  std::deque<PartyData> parties_{}; // owned parties
  std::unique_ptr<fl::events::RaidBus> raid_bus_{std::make_unique<fl::events::RaidBus>()};
  std::unique_ptr<RaidStatistics> statistics_;
  std::uint64_t raid_id_{0};
  std::unique_ptr<WorldClock> calendar_{std::make_unique<WorldClock>()};
  std::uint64_t next_visitor_beat_{0};
  seerin::BeatSub progression_sub_{};
  bool try_start_visitor(fl::context::AccountCtx ctx);
  std::unique_ptr<RaidData> raid_; // destroyed before participants and event bus
};

} // namespace fl::primitives
