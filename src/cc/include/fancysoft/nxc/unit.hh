#pragma once

#include <memory>
#include <ostream>

#include "./position.hh"

namespace Fancysoft {
namespace NXC {

/// A compilation unit containing some source code.
struct Unit {
  /// Get this unit's source code stream pointer.
  /// TODO: Make it virtually writeable for macros.
  virtual std::istream &source_stream() = 0;

  /// Parse the unit into some sort of AST. Returns the latest read position
  /// within the unit's source code.
  virtual Position parse() = 0;

  /// Check if the unit has already been parsed.
  /// A `parse()` call shall throw if already parsed.
  bool parsed() const { return _parsed; }

protected:
  bool _parsed = false;
};

} // namespace NXC
} // namespace Fancysoft
