// encounter_loop.cpp

#include "encounter_loop.hpp"

namespace seerin::enc {

EncounterLoop::EncounterLoop(seerin::Bus<InEvent> &in,
                             seerin::Bus<OutEvent> &out,
                             ICombatResolver &resolver,
                             std::vector<CombatantId> combatants,
                             seerin::atb::Config atb_cfg)
    : out_(out), resolver_(resolver), combatants_(std::move(combatants)),
      in_handle_(in.on([this](const InEvent &ev) { on_in(ev); })) {
  // Determinism anchor: stable combatant driving order
  std::sort(combatants_.begin(), combatants_.end());
  combatants_.erase(std::unique(combatants_.begin(), combatants_.end()),
                    combatants_.end());

  // Create slots and subscribe to each combatant's output, capturing who.
  slot_out_handles_.reserve(combatants_.size());

  for (auto who : combatants_) {
    auto slot = std::make_unique<Slot>(atb_cfg);

    slot_out_handles_.push_back(
        slot->out.on([this, who](const seerin::atb::OutputEvent &ev) {
          on_combatant_out(who, ev);
        }));

    slots_.emplace(who, std::move(slot));
  }
}

void EncounterLoop::on_in(const InEvent &ev) {
  std::visit(seerin::overloaded{
                 [this](const Beat &b) { handle_beat(b); },
                 [this](const CommandChosen &c) { handle_command_chosen(c); },
                 [this](const ApplyStun &s) { handle_apply_stun(s); },
                 [this](const Kill &k) { handle_kill(k); },
                 [this](const Revive &r) { handle_revive(r); }},
             ev);
}

void EncounterLoop::handle_beat(const Beat &b) {
  if (b.dt.count() <= 0)
    return;

  // Drive all combatants in stable order.
  for (auto who : combatants_) {
    forward_to_combatant(who, seerin::atb::Event{seerin::atb::Beat{b.dt}});
  }

  // After beat, if no active action is running, we can request a command if
  // needed.
  maybe_request_command();
}

void EncounterLoop::handle_command_chosen(const CommandChosen &c) {
  // Single-active constraint
  if (active_.has_value())
    return;

  // Must match the currently requested combatant
  if (!awaiting_command_.has_value())
    return;
  if (*awaiting_command_ != c.who)
    return;

  // Must be at front of ready queue
  if (ready_.empty())
    return;
  if (ready_.front() != c.who)
    return;

  // Pop from ready queue
  const bool popped = ready_.pop_front_if(c.who);
  assert(popped);

  awaiting_command_.reset();
  emit_ready_queue_changed();

  // Start action
  set_active(c.who);
  forward_to_combatant(
      c.who, seerin::atb::Event{seerin::atb::CommandSelected{c.action_time}});
}

void EncounterLoop::handle_apply_stun(const ApplyStun &s) {
  forward_to_combatant(
      s.who, seerin::atb::Event{seerin::atb::StunApplied{s.duration}});
}

void EncounterLoop::handle_kill(const Kill &k) {
  // If killed, remove from ready queue and clear active/awaiting if it matches.
  ready_.remove(k.who);

  if (awaiting_command_ && *awaiting_command_ == k.who) {
    awaiting_command_.reset();
  }

  if (active_ && *active_ == k.who) {
    // Engine will also transition; we clear active to keep pipeline unblocked.
    set_active(std::nullopt);
  }

  forward_to_combatant(k.who, seerin::atb::Event{seerin::atb::Killed{}});
  emit_ready_queue_changed();
  maybe_request_command();
}

void EncounterLoop::handle_revive(const Revive &r) {
  forward_to_combatant(
      r.who, seerin::atb::Event{seerin::atb::Revived{r.initial_charge_units}});
}

void EncounterLoop::on_combatant_out(CombatantId who,
                                     const seerin::atb::OutputEvent &ev) {
  std::visit(
      seerin::overloaded{[this, who](const seerin::atb::BecameReady &) {
                           enqueue_ready(who);
                           emit_ready_queue_changed();
                           maybe_request_command();
                         },

                         [this, who](const seerin::atb::ActionFinished &) {
                           // This is the "done being active" signal in
                           // mono-active mode. Enforce: only the active
                           // combatant can end active.
                           if (active_.has_value()) {
                             assert(*active_ == who);
                           }
                           set_active(std::nullopt);

                           // Resolve + apply rule effects
                           ResolveContext ctx{};
                           auto effects = resolver_.resolve_action(who, ctx);
                           for (const auto &e : effects) {
                             on_in(e); // deterministic re-entry
                           }

                           out_.emit(OutEvent{ActionResolved{who}});
                           maybe_request_command();
                         },

                         [this, who](const seerin::atb::StateChanged &sc) {
                           // Optional trace bridge
                           out_.emit(OutEvent{
                               Trace{who, static_cast<std::uint8_t>(sc.from),
                                     static_cast<std::uint8_t>(sc.to)}});
                         },

                         [this, who](const seerin::atb::ActionStarted &) {
                           // Optional: active is set when we accepted the
                           // command, so this is informational. If you prefer
                           // "active begins only when engine confirms", move
                           // set_active() here.
                           (void)who;
                         }},
      ev);
}

void EncounterLoop::enqueue_ready(CombatantId who) {
  // Ignore dead/unknown combatants (unknown means not in encounter)
  if (slots_.find(who) == slots_.end())
    return;

  ready_.push_back_unique(who);

  // If we had requested a command for someone else (shouldn't happen in
  // mono-active), keep it strict: don't change the request until it's
  // satisfied/cancelled.
}

void EncounterLoop::maybe_request_command() {
  // If an action is active, we do not request commands.
  if (active_.has_value())
    return;

  // If we already requested a command, do nothing.
  if (awaiting_command_.has_value())
    return;

  // Request next command if someone is ready.
  if (ready_.empty())
    return;

  awaiting_command_ = ready_.front();
  out_.emit(OutEvent{NeedCommand{*awaiting_command_}});
}

void EncounterLoop::emit_ready_queue_changed() {
  out_.emit(OutEvent{ReadyQueueChanged{ready_.snapshot()}});
}

void EncounterLoop::set_active(std::optional<CombatantId> who) {
  if (active_ == who)
    return;
  active_ = who;
  if (active_)
    out_.emit(OutEvent{ActiveChanged{*active_, true}});
  // If we cleared active, we don’t know who to report; emit a separate event if
  // desired. For now, emit "is_active=false" only when we know who was active.
  // If you want, store prev and emit that.
}

void EncounterLoop::forward_to_combatant(CombatantId who,
                                         const seerin::atb::Event &ev) {
  auto it = slots_.find(who);
  if (it == slots_.end())
    return;
  it->second->in.emit(ev);
}

} // namespace seerin::enc
