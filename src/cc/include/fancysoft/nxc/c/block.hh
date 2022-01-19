#pragma once

#include <memory>

#include "../logger.hh"
#include "../unit.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {

struct Program;

namespace C {

/// A block of freestanding C code usually located in an Onyx source file.
struct Block : NXC::Block, std::enable_shared_from_this<Block> {
  Block(
      Placement placement,
      std::istream &stream,
      std::shared_ptr<Logger> logger) :
      NXC::Block(placement), _stream(stream), _logger(logger) {}

  std::istream &source_stream() override { return _stream; }
  Position parse() override;
  const CST *cst() const { return _cst.get(); }

private:
  std::shared_ptr<Logger> _logger;
  std::istream &_stream;
  std::unique_ptr<const CST> _cst;
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
