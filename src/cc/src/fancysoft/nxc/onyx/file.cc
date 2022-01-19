#include <memory>

#include "fancysoft/nxc/logger.hh"
#include "fancysoft/nxc/onyx/file.hh"
#include "fancysoft/nxc/onyx/parser.hh"

namespace Fancysoft::NXC::Onyx {

Position File::parse() {
  unparse();
  _logger->sdebug() << "Parsing " << this->path << "\n";

  auto lexer = Lexer(shared_from_this(), _logger->dup("lexer"));
  Parser parser(&lexer, _logger->dup("parser"));

  _cst = move(parser.parse());
  _parsed = true;

  _logger->strace() << "Parsed " << this->path << "\n";
  return lexer.cursor();
}

void File::compile() {
  uncompile();
  _logger->sdebug() << "Compiling " << this->path << "\n";

  if (!_parsed)
    parse();

  _ast = std::make_unique<AST>(_cst.get(), _logger->dup("ast"));
  _logger->strace() << "Compiled " << this->path << "\n";
}

} // namespace Fancysoft::NXC::Onyx
