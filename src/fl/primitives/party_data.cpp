#include "party_data.hpp"

#include <entt/entt.hpp>
#include <vector>

#include "fl/context.hpp"
#include "fl/ecs/components/is_party.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop.hpp"
#include "fl/fsm/party_loop_ctx.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "logging.hpp"
#include "random_hub.hpp"

namespace fl::primitives {

PartyData::PartyData(entt::entity party_id)
    : party_id_(party_id), log_(std::make_shared<fl::widgets::FancyLog>()) {

  // auto party_loop_ctx = ctx.party_context().party_loop_context();
}

void PartyData::init_party(fl::fsm::PartyLoopCtx &party_loop_ctx,
                           std::string name) {
  party_loop_ctx.reg().emplace<fl::ecs::components::IsParty>(
      party_id_, party_loop_ctx, std::move(name),
      party_loop_ctx.account_->account_id_);
}

} // namespace fl::primitives
