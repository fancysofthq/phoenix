#pragma once

#include <ostream>
#include <sstream>

#include "./placement.hh"

namespace Fancysoft {
namespace NXC {

struct Token {
  Placement placement;
  Token(Placement placement) : placement(placement) {}

  /// Return the token constant name, e.g. `"<Id>"`.
  // virtual static const char *name() = 0;

  /// Emprint a token in code, e.g. `'\n'` instead of `"<Newline>"`.
  virtual const void print(std::ostream &) const = 0;

  /// Print the end-user friendly token representation, e.g. `"<Id `foo`>"`.
  virtual const void inspect(std::ostream &) const = 0;

  std::string inspect() const {
    std::stringstream ss;
    inspect(ss);
    return ss.str();
  };
};

} // namespace NXC
} // namespace Fancysoft
