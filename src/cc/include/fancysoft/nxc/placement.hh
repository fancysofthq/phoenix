#pragma once

#include <ostream>
#include <vector>

#include "./location.hh"

namespace Fancysoft {
namespace NXC {

struct Unit;

/// A placement within a compilation unit.
struct Placement {
  std::shared_ptr<Unit> unit;
  Location location;

  Placement(std::shared_ptr<Unit> unit, Location location) :
      unit(unit), location(location) {}

  /// Return the full path to the placement, starting with the containing unit.
  const std::vector<Placement> path() const;

  /// Output the full placement, so that a end-user can be pointed precisely.
  void debug(std::ostream &stream) const;
};

} // namespace NXC
} // namespace Fancysoft
