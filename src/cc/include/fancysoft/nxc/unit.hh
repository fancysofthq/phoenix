#pragma once

#include <memory>
#include <ostream>

#include "./placement.hh"

namespace Fancysoft {
namespace NXC {

/// A compilation unit containing some source code to be parsed.
/// It may be, for example, a source file or macro context.
struct Unit {
  /// Get this unit's source code stream pointer.
  /// TODO: Make it virtually writeable for macros.
  virtual std::istream &source_stream() = 0;

  /// Parse the unit into some sort of CST. Returns the latest read position
  /// within the unit's source code. Subsequent calls would re-parse.
  virtual Position parse() = 0;

  /// Check if the unit has already been parsed.
  bool parsed() const { return _parsed; }

  /// Invalidate the unit's CST. Returns `true` if it was parsed before.
  virtual bool unparse() {
    auto prev = parsed();
    _parsed = false;
    return prev;
  }

protected:
  bool _parsed = false;
};

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
