#pragma once

#include <ostream>
#include <sstream>

#include "./placement.hh"

namespace Fancysoft {
namespace Phoenix {

/// An abstract source code token.
struct Token {
  Placement placement;
  Token(Placement placement) : placement(placement) {}

  /// Return the token name to display to the end-user, e.g. `"Id"`.
  virtual const char *token_name() const = 0;

  /// Emprint a token in code, e.g. `'\n'`.
  virtual const void print(std::ostream &) const = 0;

  /// @see @link print(std::ostream) @endlink.
  std::string print(unsigned indent = 0) const {
    std::stringstream ss;
    print(ss);
    return ss.str();
  }
};

} // namespace Phoenix
} // namespace Fancysoft
