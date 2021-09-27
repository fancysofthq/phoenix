#include <memory>

#include "fancysoft/nxc/onyx/file.hh"
#include "fancysoft/nxc/onyx/parser.hh"
#include "fancysoft/util/logger.hh"

namespace Fancysoft::NXC::Onyx {

Position File::parse() {
  assert(!_parsed);
  Util::logger.debug("File") << "Parsing " << this->path << "\n";

  auto lexer = std::make_shared<Lexer>(shared_from_this());
  Parser parser(lexer);

  _ast = move(parser.parse());
  _parsed = true;

  Util::logger.trace("File") << "Parsed " << this->path << "\n";
  return lexer->cursor();
}

void File::compile() {
  assert(!compiled());
  Util::logger.debug("File") << "Compiling " << this->path << "\n";

  if (!_parsed)
    parse();

  _mlir = std::make_unique<MLIR>(_ast.get(), _program);
  Util::logger.trace("File") << "Compiled " << this->path << "\n";
}

} // namespace Fancysoft::NXC::Onyx
