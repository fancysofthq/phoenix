#include <memory>

#include "fancysoft/nxc/c/block.hh"
#include "fancysoft/nxc/c/parser.hh"

namespace Fancysoft::NXC::C {

std::istream &Block::source_stream() { return _stream; }

Position Block::parse(std::shared_ptr<Block> unit_ptr) {
  auto lexer = std::make_shared<Lexer>(unit_ptr);
  Parser parser(lexer);
  parser.parse(this->cst, true);
  return lexer->cursor();
}

} // namespace Fancysoft::NXC::C
