#pragma once

#include <ostream>

#include "./placement.hh"

namespace Fancysoft {
namespace NXC {

struct Token {
  Placement placement;
  Token(Placement placement) : placement(placement) {}

  /// Return the token name, e.g. `"<Id>"`.
  virtual const char *name() const = 0;

  /// Emprint a token in code, e.g. `'\n'` instead of `"<Newline>"`.
  virtual const void print(std::ostream &) const = 0;

  /// Print the end-user friendly token representation, e.g. `"<Id `foo`>"`.
  virtual const void inspect(std::ostream &) const = 0;

  /// Ditto.
  virtual const std::string inspect() const = 0;
};

} // namespace NXC
} // namespace Fancysoft
