#pragma once

#include <memory>
#include <ostream>

#include "./placement.hh"

namespace Fancysoft {
namespace Phoenix {

/// A compilation unit containing source code to be parsed. It may be, for
/// example, a source file or code emitted from a macro.
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

  /// Output the unit's contents into a stream.
  virtual void print(std::ostream &) const = 0;

protected:
  bool _parsed = false;
};

/// A virtual block unit contained in some other unit.
struct Block : Unit {
  /// The block's placement within the containing unit.
  Placement placement;

  Block(Placement placement) : placement(placement){};

  virtual std::istream &source_stream() override = 0;
  virtual Position parse() override = 0;
  virtual void print(std::ostream &) const override = 0;
};

} // namespace Phoenix
} // namespace Fancysoft
