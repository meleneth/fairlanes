#pragma once
// INTERNAL: Do not include from engine code.

#include <deque>
#include <memory>
#include <mutex>

#include <entt/entt.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <tracy/Tracy.hpp>

#include "fl/events/beat_bus.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/logging.hpp"
#include "fl/primitives/random_hub.hpp"


/*#include "fl/context.hpp"
#include "fl/monsters/register_monsters.hpp"
#include "fl/primitives/fancy_log_sink.hpp"
#include "fl/primitives/hero_names.hpp"
#include "fl/primitives/member_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/party_names.hpp"
#include "fl/widgets/fancy_log.hpp"
*/

namespace fl {
class GrandCentral {
public:
  uint8_t num_accounts_{8};
  uint8_t num_parties_per_account_{5};
  uint8_t num_members_per_party_{5};
  entt::registry reg_;
  fl::primitives::RandomHub rng_;
  fl::primitives::LogBus log_bus_;
  fl::primitives::Logger logger_;
  std::mutex frame_mutex;
  std::deque<fl::primitives::AccountData> accounts_;
  std::shared_ptr<fl::widgets::FancyLog> fancy_log_;
  std::unique_ptr<fl::primitives::FancyLogSink> fancy_log_sink_;
  fl::events::BeatBus beat_bus_;

  void _create_initial_accounts();
  GrandCentral(uint8_t num_accounts, uint8_t num_parties_per_account,
               uint8_t num_members_per_party);
  ftxui::Component root_component();
  void innervate_event_system();

  fl::context::AccountCtx account_context(std::size_t idx);
  fl::context::AccountCtx account_context(fl::primitives::AccountData &account);

  void bootstrap_logs();
  void main_loop();
};

} // namespace fl
