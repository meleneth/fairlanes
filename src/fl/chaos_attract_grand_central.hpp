#pragma once

#include <optional>
#include <memory>

#include <ftxui/component/component.hpp>

namespace fl {

class GrandCentral;

struct ChaosAttractRunOptions {
  std::optional<double> cutoff_seconds;
};

enum class ChaosAttractExit {
  Begin,
  Quit,
};

class ChaosAttractGrandCentral {
public:
  ChaosAttractGrandCentral();
  ~ChaosAttractGrandCentral();

  [[nodiscard]] ChaosAttractExit
  main_loop(ChaosAttractRunOptions opts = {});

private:
  std::unique_ptr<GrandCentral> attract_game_;
  ftxui::Component root_component_;
};

} // namespace fl
