// grand_central.hpp (or better: fl/primitives/account_data.hpp)
#pragma once

#include <array>
#include <deque>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <entt/entity/registry.hpp>

#include "fl/fwd.hpp"
#include "fl/primitives/discovery_journal.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/moon_calendar.hpp"
#include "fl/primitives/raid_data.hpp"
#include "fl/primitives/raid_statistics.hpp"
#include "fl/primitives/tower_balance.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "party_data.hpp"

namespace fl::primitives {

enum class RecruitmentOpportunityState { Locked, Available, Claimed };

struct RecruitmentOpportunity {
  std::string id;
  std::string biome_id;
  int floor{0};
  std::size_t destination_party{0};
  RecruitmentOpportunityState state{RecruitmentOpportunityState::Locked};
};

struct AccountData {
public:
  explicit AccountData(entt::entity account_id, bool record_progress = true,
                       TowerBalance tower_balance = {})
      : account_id_{account_id}, log_{std::make_unique<fl::widgets::FancyLog>()},
        statistics_{std::make_unique<RaidStatistics>(*raid_bus_, account_id, record_progress)},
        tower_balance_{tower_balance} {}

  AccountData(AccountData&&) noexcept = default;
  AccountData& operator=(AccountData&&) noexcept = default;

  AccountData(const AccountData&) = delete;
  AccountData& operator=(const AccountData&) = delete;

  const RaidRecords& raid_records() const {
    return statistics_->records();
  }
  RaidData* raid() {
    return raid_.get();
  }
  const RaidData* raid() const {
    return raid_.get();
  }
  bool in_raid() const {
    return raid_ && raid_->active();
  }
  fl::events::RaidBus& raid_bus() {
    return *raid_bus_;
  }
  bool start_raid(fl::context::AccountCtx ctx, std::span<const fl::monster::MonsterKind> enemies) {
    if (in_raid() || parties_.empty() || enemies.empty()) return false;
    normal_party_battle_mode_ = false;
    raid_.reset();
    raid_ = std::make_unique<RaidData>(ctx, *raid_bus_, ++raid_id_, enemies);
    raid_->announce_started();
    return true;
  }
  bool spawn_visitor(fl::context::AccountCtx ctx);
  bool normal_party_battle_mode() const noexcept {
    return normal_party_battle_mode_;
  }

  static constexpr std::uint64_t kVisitorPeriodBeats =
      MoonCalendar::kVisitorCycleDays * WorldClock::kBeatsPerCalendarDay;
  WorldClock& calendar() {
    return *calendar_;
  }
  const WorldClock& calendar() const {
    return *calendar_;
  }
  std::uint64_t beats_until_visitor() const {
    return next_visitor_beat_ > calendar_->elapsed_beats()
               ? next_visitor_beat_ - calendar_->elapsed_beats()
               : 0;
  }
  void hook_to_beat(fl::context::AccountCtx ctx, seerin::BeatBus& beats, WorldClock& world_clock,
                    bool enable_visitors = true);
  void advance_beat(fl::context::AccountCtx ctx, bool enable_visitors = true);

  // --- capability-style accessors ---
  entt::entity account_id() const {
    return account_id_;
  }

  fl::widgets::FancyLog& log() const {
    // FL_ASSERT(log_);
    return *log_;
  }

  std::deque<PartyData>& parties() {
    return parties_;
  }
  const std::deque<PartyData>& parties() const {
    return parties_;
  }

  PartyData& party(std::size_t idx) {
    return parties_.at(idx);
  }
  const PartyData& party(std::size_t idx) const {
    return parties_.at(idx);
  }
  const TowerBalance& tower_balance() const noexcept {
    return tower_balance_;
  }
  MemberData& create_character(entt::entity member_id, std::string name);
  void enable_roster_transfers();
  bool request_roster_transfer(fl::context::AccountCtx ctx, entt::entity member,
                               std::size_t destination_party);
  std::size_t character_count() const noexcept {
    return characters_.size();
  }
  void enable_discoveries(entt::registry& reg);
  DiscoveryJournal& discoveries() noexcept {
    return discoveries_;
  }
  const DiscoveryJournal& discoveries() const noexcept {
    return discoveries_;
  }
  void enable_origin_recruitment();
  void resolve_recruitment_events(fl::context::AccountCtx ctx);
  bool claim_recruitment(fl::context::AccountCtx ctx, std::string_view opportunity_id);
  bool recruit_story_hero(fl::context::AccountCtx ctx, std::string_view opportunity_id,
                          std::string_view name);
  const std::vector<RecruitmentOpportunity>& recruitment_opportunities() const noexcept {
    return recruitment_opportunities_;
  }

private:
  entt::entity account_id_{entt::null};
  std::unique_ptr<fl::widgets::FancyLog> log_{};

  // Declared before parties so stable character records outlive roster views.
  std::deque<MemberData> characters_;
  std::deque<PartyData> parties_{}; // owned parties
  DiscoveryJournal discoveries_;
  std::unique_ptr<DiscoveryJournalListener> discovery_listener_;
  std::unique_ptr<fl::events::RaidBus> raid_bus_{std::make_unique<fl::events::RaidBus>()};
  std::unique_ptr<RaidStatistics> statistics_;
  TowerBalance tower_balance_{};
  std::uint64_t raid_id_{0};
  std::unique_ptr<WorldClock> calendar_{std::make_unique<WorldClock>()};
  std::uint64_t next_visitor_beat_{0};
  seerin::BeatSub progression_sub_{};
  bool try_start_visitor(fl::context::AccountCtx ctx);
  void bind_recruitment_party(std::size_t party_index);
  void bind_boundary_party(std::size_t party_index);
  void process_pending_transfers(fl::context::AccountCtx ctx);
  bool commit_transfer(fl::context::AccountCtx ctx, entt::entity member, std::size_t source_party,
                       std::size_t destination_party);
  void refresh_transfer_blocks();
  bool ensure_party_unlocked(fl::context::AccountCtx ctx, std::size_t party_index);
  void unlock_floor_opportunities(fl::context::AccountCtx ctx);
  void unlock_completion_opportunities(fl::context::AccountCtx ctx, std::size_t party_index);
  void complete_roster(fl::context::AccountCtx ctx, std::size_t party_index);
  fl::events::PartyBus& progression_bus();
  std::unique_ptr<RaidData> raid_; // destroyed before participants and event bus
  bool recruitment_enabled_{false};
  std::vector<RecruitmentOpportunity> recruitment_opportunities_;
  std::vector<fl::events::ScopedPartyListener> recruitment_subscriptions_;
  std::vector<fl::events::ScopedPartyListener> boundary_subscriptions_;
  struct PendingTransfer {
    entt::entity member{entt::null};
    std::size_t source_party{0};
    std::size_t destination_party{0};
  };
  std::vector<PendingTransfer> pending_transfers_;
  bool roster_transfers_enabled_{false};
  bool normal_party_battle_mode_{false};
  std::array<bool, 5> completed_rosters_{};
  std::unordered_set<std::string> claimed_story_recruits_;
  std::unordered_set<std::string> committed_floor_clears_;
  std::vector<fl::events::TowerFloorCleared> pending_floor_clears_;
};

} // namespace fl::primitives
