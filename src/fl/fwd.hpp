#pragma once

namespace fl {
class GrandCentral;
}

namespace fl::context {
struct AccountCtx;
struct AttackCtx;
struct BuildCtx;
struct EntityCtx;
struct PartyCtx;
} // namespace fl::context

namespace fl::ecs::components {
struct ColorOverride;
struct Encounter;
struct InEncounter;
struct IsParty;
struct PartyMember;
struct Stats;
struct Tags;
struct TrackXP;
} // namespace fl::ecs::components

namespace fl::events {
class TimedEventQueue;
struct AccountEvent;
struct AnimationFinished;
struct AttackResolved;
struct LogEvent;
struct PlayerCommandAttack;
struct Tick;
struct TimerCompare;
struct TimerEvent;
struct TimerPolicy;
} // namespace fl::events

namespace fl::fsm {
struct PartyLoop;
struct PartyLoopCtx;
} // namespace fl::fsm

namespace fl::monster {
class FieldMouse;
}

namespace fl::primitives {
class EntityBuilder;
class FancyLogSink;
class Logger;
class RandomHub;
class RandomStream;
struct AccountData;
struct Damage;
struct EncounterBuilder;
struct Logging;
struct MemberData;
struct PartyData;
struct Resistances;
struct Team;
} // namespace fl::primitives

namespace fl::skills {
class Thump;
}

namespace fl::systems {
class GrantXPToParty;
}

namespace fl::widgets {
class FancyLog;
struct Options;
} // namespace fl::widgets
