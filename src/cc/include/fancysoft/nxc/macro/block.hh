#pragma once

#include "../block.hh"

namespace Fancysoft {
namespace NXC {
namespace Macro {

struct Block : NXC::Block {
  bool immediate;
  bool emitting;

  Block(Placement placement) : NXC::Block(placement){};

  virtual Position parse() override = 0;
  virtual std::istream &source_stream() override = 0;
};

} // namespace Macro
} // namespace NXC
} // namespace Fancysoft
