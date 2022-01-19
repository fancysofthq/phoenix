#pragma once

#include <optional>
#include <ostream>
#include <vector>

namespace Fancysoft {
namespace Phoenix {

struct Unit;

/// A position, i.e. row and column, beginning with `0`.
struct Position {
  uint32_t row;
  uint32_t col;

  Position(uint32_t row = 0, uint32_t col = 0) : row(row), col(col) {}

  inline Position operator+(Position another) {
    if (another.row == 0) {
      return Position(row, col + another.col);
    } else {
      return Position(row + another.row, another.col);
    }
  }
};

/// A spanning location comprised of two `Position` s.
struct Location {
  Position start;
  std::optional<Position> end;

  Location(Position start, std::optional<Position> end = std::nullopt) :
      start(start), end(end) {}
};

/// A `Location` within a compilation unit.
struct Placement {
  std::shared_ptr<Unit> unit;
  Location location;

  Placement(std::shared_ptr<Unit> unit, Location location) :
      unit(unit), location(location) {}

  /// Return the full path to the placement, starting with the containing unit.
  /// TODO: What?
  const std::vector<Placement> path() const;

  /// Output the full placement, so that a end-user can be pointed precisely.
  void debug(std::ostream &stream) const;
};

} // namespace Phoenix
} // namespace Fancysoft
