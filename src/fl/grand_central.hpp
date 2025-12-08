#pragma once
// INTERNAL: Do not include from engine code.

#include <deque>
#include <memory>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>

#include "fl/context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"

#include "fl/primitives/fancy_log_sink.hpp"
#include "fl/primitives/logging.hpp"
#include "fl/primitives/random_hub.hpp"
#include "fl/widgets/fancy_log.hpp"

namespace fl {

class GrandCentral {
public:
  entt::registry reg_;
  fl::primitives::RandomHub rng_;

  fl::primitives::LogBus log_bus_;
  fl::primitives::Logger logger_;
  uint8_t num_accounts_{8};
  uint8_t num_parties_per_account_{5};

  std::vector<fl::primitives::AccountData> accounts_;

  // UI pieces
  std::shared_ptr<fl::widgets::FancyLog> fancy_log_;
  std::unique_ptr<fl::primitives::FancyLogSink> fancy_log_sink_;

  GrandCentral(int num_accounts, int num_parties_per_account)
      : num_accounts_(num_accounts),
        num_parties_per_account_(num_parties_per_account), reg_(), rng_(),
        log_bus_(), logger_{log_bus_},
        fancy_log_(std::make_shared<fl::widgets::FancyLog>()),
        fancy_log_sink_(std::make_unique<fl::primitives::FancyLogSink>(
            log_bus_, *fancy_log_, fl::primitives::LogLevel::trace)) {}

  // Convenience accessor if you want the root FTXUI component:
  ftxui::Component root_component() { return fancy_log_; }

  fl::context::AccountCtx account_context(std::size_t idx) {
    return fl::context::AccountCtx{reg_, rng_, accounts_.at(idx)};
  }

  // Little helper to show logs are working
  void bootstrap_logs() {
    logger_.info("[name](GrandCentral) online.");
    logger_.debug("Debug: registry currently empty.");
    logger_.warn("No encounters loaded yet.");
  }
};

} // namespace fl
