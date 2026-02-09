#include "fl/fsm/party_loop_machine.hpp"
#include "fl/context.hpp"
#include "fl/fsm/party_loop.hpp"
#include "sr/atb_events.hpp"

#include <boost/sml.hpp>

namespace fl::fsm {

struct PartyLoopMachine::Impl {
  fl::context::PartyCtx *ctx{};
  boost::sml::sm<fl::fsm::PartyLoop> sm_;
  explicit Impl(fl::context::PartyCtx &c) : ctx(&c), sm_(c) {}
};

PartyLoopMachine::PartyLoopMachine(fl::context::PartyCtx &ctx)
    : impl_(std::make_unique<Impl>(ctx)) {}

PartyLoopMachine::~PartyLoopMachine() = default;
PartyLoopMachine::PartyLoopMachine(PartyLoopMachine &&) noexcept = default;
PartyLoopMachine &
PartyLoopMachine::operator=(PartyLoopMachine &&) noexcept = default;

void PartyLoopMachine::beat_event() {
  impl_->sm_.process_event(seerin::Beat{});
}

void PartyLoopMachine::dispatch_party_bus() {}

} // namespace fl::fsm
