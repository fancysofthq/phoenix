#include "fancysoft/nxc/module.hh"
#include "fancysoft/nxc/onyx/parser.hh"

namespace Fancysoft::NXC {

void Module::parse() {
  auto lexer = std::make_shared<Onyx::Lexer>(file);
  auto parser = Onyx::Parser(lexer);
  parser.parse(file->cst);
}

} // namespace Fancysoft::NXC
