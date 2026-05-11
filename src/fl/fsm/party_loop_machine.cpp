#include "fl/fsm/party_loop_machine.hpp"
#include "fl/context.hpp"
#include "fl/events/party_bus.hpp"
#include "fl/fsm/party_loop.hpp"
#include "fl/widgets/fancy_log.hpp"
#include "sr/atb_events.hpp"
#include <boost/sml.hpp>

namespace fl::fsm {

struct PartyLoopMachine::Impl {
  fl::context::PartyCtx *ctx{};
  boost::sml::sm<fl::fsm::PartyLoop> sm_;
  fl::events::ScopedPartyListener party_wiped_sub_;

  explicit Impl(fl::context::PartyCtx &c)
      : ctx(&c), sm_(c),
        party_wiped_sub_(c.bus(), std::in_place_type<fl::events::PartyWiped>,
                         [this](const fl::events::PartyWiped &) {
                           sm_.process_event(fl::fsm::PartyWipedEvent{});
                         }) {}
};

PartyLoopMachine::PartyLoopMachine(fl::context::PartyCtx &ctx)
    : impl_(std::make_unique<Impl>(ctx)) {}

PartyLoopMachine::~PartyLoopMachine() = default;
PartyLoopMachine::PartyLoopMachine(PartyLoopMachine &&) noexcept = default;
PartyLoopMachine &
PartyLoopMachine::operator=(PartyLoopMachine &&) noexcept = default;

void PartyLoopMachine::beat_event() {
  //  impl_->ctx->log().append_markup(
  //    "[magenta](PartyLoopMachine) Received beat event.");
  impl_->sm_.process_event(fl::fsm::NextEvent{});
}

void PartyLoopMachine::dispatch_party_bus() {}

} // namespace fl::fsm
