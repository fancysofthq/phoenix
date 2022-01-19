#pragma once

#include <ostream>
#include <sstream>

#include <fmt/ostream.h>

namespace Fancysoft {
namespace NXC {

/// An generic tree node.
struct Node {
  static void print_tab(std::ostream &stream, unsigned indent) {
    stream << std::string(indent, '\t');
  }

  // static std::string inspect_prefix(unsigned short indent) {
  //   return std::string(indent * 3, ' ') + "|-";
  // }

  // static std::string inspect_attribute_prefix(unsigned short indent) {
  //   return std::string(indent * 3 + 3, ' ');
  // }

  /// The node's name, e.g. `"Root"`.
  virtual const char *node_name() const = 0;

  /// Print the node into *stream* with given *indent*.
  virtual void print(std::ostream &stream, unsigned indent = 0) const = 0;

  /// @see @link print(std::ostream) @endlink.
  std::string print(unsigned indent = 0) const {
    std::stringstream ss;
    print(ss, indent);
    return ss.str();
  }

  /// Print some short node representation into *stream* to aid tracing,
  /// e.g. ``<CCall $puts[1]> ``. Outputs `node_name()` by default.
  virtual void trace(std::ostream &stream) const {
    fmt::print(stream, "<{0}>", node_name());
  }

  /// @see @link trace(std::ostream) @endlink.
  std::string trace() const {
    std::stringstream ss;
    trace(ss);
    return ss.str();
  }
};

} // namespace NXC
} // namespace Fancysoft
