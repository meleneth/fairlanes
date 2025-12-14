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

PartyData::PartyData(fl::context::AccountCtx &ctx, std::string name)
    : party_id_(ctx.reg().create()),
      log_(std::make_shared<fl::widgets::FancyLog>()) {

  // Assuming your AccountCtx can provide PartyCtx for this PartyData:
  auto pctx =
      ctx.party_context(*this); // returns fl::context::PartyCtx by value

  fl::fsm::PartyLoopCtx loop_ctx{
      .reg_ = &pctx.reg(),
      .rng_ = &pctx.rng_,
      .account_ = &pctx.account_data.account_id_,
      .party_ = &pctx.party_, // if PartyCtx holds PartyData& party_;
  };

  ctx.reg().emplace<fl::ecs::components::IsParty>(loop_ctx, std::move(name),
                                                  ctx.account_.account_id_);
}

} // namespace fl::primitives
