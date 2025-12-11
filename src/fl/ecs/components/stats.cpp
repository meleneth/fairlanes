#include <fmt/core.h>

#include "stats.hpp"

using fl::ecs::components::Stats;

Stats::Stats(std::string name) : name_(name) {}

bool Stats::is_alive() { return hp_ > 0; }
