#pragma once
#include <memory>
#include <string>

namespace fairlanes::widgets {
class FancyLog;
}
namespace fairlanes::fsm {
struct PartyLoopCtx;
}

namespace fairlanes::ecs::components {

using fairlanes::widgets::FancyLog;
// Marks an entity as the Selected Account (it's a UI thing)
struct SelectedAccount {};

} // namespace fairlanes::ecs::components
