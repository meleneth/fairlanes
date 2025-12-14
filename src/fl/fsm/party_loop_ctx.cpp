#include "party_loop_ctx.hpp"

#include <entt/entt.hpp>

#include "fl/context.hpp"
#include "fl/primitives/account_data.hpp"
#include "fl/primitives/party_data.hpp"
#include "fl/primitives/random_hub.hpp"

namespace fl::fsm {

fl::context::PartyCtx PartyLoopCtx::party_ctx() const {
  // This returns your reference-y PartyCtx by value,
  // internally binding refs to these pointers.
  return fl::context::PartyCtx{*reg_, *rng_, *account_, *party_};
}

} // namespace fl::fsm
