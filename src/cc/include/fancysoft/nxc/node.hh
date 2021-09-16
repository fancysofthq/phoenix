#pragma once

#include <ostream>
#include <sstream>

namespace Fancysoft {
namespace NXC {

/// An abstract tree node.
struct Node {
  /// The node's name, e.g. `"<Root>"`.
  virtual const char *node_name() const = 0;

  /// Inspect the node into *stream* with given *indent*.
  virtual void
  inspect(std::ostream &stream, unsigned short indent = 0) const = 0;

  std::string inspect() const {
    std::stringstream ss;
    inspect(ss);
    return ss.str();
  }
};

} // namespace NXC
} // namespace Fancysoft
