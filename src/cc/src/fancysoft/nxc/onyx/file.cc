#include <memory>

#include "fancysoft/nxc/onyx/file.hh"
#include "fancysoft/nxc/onyx/parser.hh"

namespace Fancysoft::NXC::Onyx {

Position File::parse() {
  assert(!_parsed);

  auto lexer = std::make_shared<Lexer>(shared_from_this());
  Parser parser(lexer);

  _ast = move(parser.parse());
  _parsed = true;

  return lexer->cursor();
}

void File::compile(Program *program) {
  assert(!_compiled);

  if (!_parsed)
    parse();

  _hlir = std::make_shared<const HLIR>(program, _ast.get());
  _compiled = true;
}

} // namespace Fancysoft::NXC::Onyx
