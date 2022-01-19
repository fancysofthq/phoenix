#pragma once

#include "../util/node.hh"

namespace Fancysoft {
namespace Phoenix {
namespace C {

/// The C Abstract Syntax Tree (with possible Onyx code injections).
struct AST : Util::Node {
  const char *node_name() const override { return "Root"; }
  void print(std::ostream &stream, unsigned indent = 0) const override;
};

} // namespace C
} // namespace Phoenix
} // namespace Fancysoft
