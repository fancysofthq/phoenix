#pragma once

#include <memory>

#include "./placement.hh"
#include "./unit.hh"

namespace Fancysoft {
namespace NXC {

/// A virtual block of code. It doesn't need an AST, but CST.
template <class CSTT> struct Block : Unit {
  /// Placement within the parent unit.
  Placement placement;

  /// The CST root representing the exact unit structure.
  std::shared_ptr<CSTT> cst = std::make_shared<CSTT>();

  Block(Placement placement) : placement(placement){};

  virtual std::istream &source_stream() override = 0;
  // virtual std::string source() const override;
};

} // namespace NXC
} // namespace Fancysoft
