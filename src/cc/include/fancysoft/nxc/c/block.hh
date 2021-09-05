#pragma once

#include "../block.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

/// A physical block of C code located in an Onyx source file.
struct Block : NXC::Block<CST::Root> {
  Block(Placement placement, std::istream &stream) :
      NXC::Block<CST::Root>(placement), _stream(stream) {}

  Position parse(std::shared_ptr<Block> unit_ptr);

  std::istream &source_stream() override;

private:
  std::istream &_stream;
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
