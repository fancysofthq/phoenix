#pragma once

#include <ostream>

namespace Fancysoft {
namespace NXC {

struct Node {
  /// The node's name, e.g. `"<Root>"`.
  virtual const char *node_name() const = 0;

  /// Inspect the node into *stream* with given *indent*.
  virtual void
  inspect(std::ostream &stream, unsigned short indent = 0) const = 0;
};

} // namespace NXC
} // namespace Fancysoft
