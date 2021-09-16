#pragma once

#include <memory>

#include "../block.hh"
#include "./ast.hh"

namespace Fancysoft {
namespace NXC {

struct Program;

namespace C {

/// A block of C code located in an Onyx source file.
struct Block : NXC::Block, std::enable_shared_from_this<Block> {
  Block(Placement placement, std::istream &stream) :
      NXC::Block(placement), _stream(stream) {}

  std::istream &source_stream() override { return _stream; }
  Position parse() override;

  const AST *ast() const { return _ast.get(); }

private:
  std::istream &_stream;
  std::unique_ptr<const AST> _ast;
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
