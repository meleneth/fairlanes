#include "fl/primitives/account_data.hpp"

#include <deque>
#include <entt/entt.hpp>

#include "fl/events/account_bus.hpp"
#include "fl/fwd.hpp"
#include "fl/primitives/fancy_log_sink.hpp"

/*
#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"
#include "party_data.hpp"
#include "random_hub.hpp"
*/

namespace fl::primitives {

AccountData::AccountData(entt::entity account_id)
    : account_id_(account_id), log_(std::make_shared<fl::widgets::FancyLog>()) {
}

} // namespace fl::primitives
