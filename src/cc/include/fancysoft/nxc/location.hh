#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <fmt/core.h>

#include "./position.hh"

namespace Fancysoft {
namespace NXC {

/// A location within an abstract compilation unit.
struct Location {
  Position start;
  std::optional<Position> end;

  Location(Position start, std::optional<Position> end = std::nullopt) :
      start(start), end(end) {}
};

} // namespace NXC
} // namespace Fancysoft
