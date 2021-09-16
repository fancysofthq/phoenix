#pragma once

#include "./placement.hh"
#include "./unit.hh"

namespace Fancysoft {
namespace NXC {

/// A virtual block unit to read source code from.
struct Block : Unit {
  /// Placement within the parent unit.
  Placement placement;

  Block(Placement placement) : placement(placement){};

  virtual Position parse() override = 0;
  virtual std::istream &source_stream() override = 0;
};

} // namespace NXC
} // namespace Fancysoft
