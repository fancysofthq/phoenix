#include <assert.h>
#include <memory>

#include "fancysoft/nxc/c/block.hh"
#include "fancysoft/nxc/c/parser.hh"

namespace Fancysoft::NXC::C {

Position Block::parse() {
  assert(!_parsed);
  auto lexer = std::make_shared<Lexer>(shared_from_this());
  Parser parser(lexer);
  _ast = std::move(parser.parse(true));
  _parsed = true;
  return lexer->cursor();
}

} // namespace Fancysoft::NXC::C
