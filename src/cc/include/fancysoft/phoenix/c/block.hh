#pragma once

#include <memory>

#include "../unit.hh"
#include "../util/logger.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace Phoenix {

struct Program;

namespace C {

/// A block of freestanding C code located in an Onyx source file.
struct Block : Phoenix::Block, std::enable_shared_from_this<Block> {
  Block(
      Placement placement,
      std::istream &stream,
      std::shared_ptr<Util::Logger> logger) :
      Phoenix::Block(placement), _stream(stream), _logger(logger) {}

  std::istream &source_stream() override { return _stream; }
  Position parse() override;
  const CST *cst() const { return _cst.get(); }
  void print(std::ostream &) const override;

private:
  std::shared_ptr<Util::Logger> _logger;
  std::istream &_stream;
  std::unique_ptr<const CST> _cst;
};

} // namespace C
} // namespace Phoenix
} // namespace Fancysoft
