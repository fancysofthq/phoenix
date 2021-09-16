#pragma once

#include <optional>

#include "./position.hh"

namespace Fancysoft {
namespace NXC {

/// A spanning location.
struct Location {
  Position start;
  std::optional<Position> end;

  Location(Position start, std::optional<Position> end = std::nullopt) :
      start(start), end(end) {}
};

} // namespace NXC
} // namespace Fancysoft
