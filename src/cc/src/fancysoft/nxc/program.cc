#include <iostream>
#include <memory>

#include "fancysoft/nxc/onyx/lexer.hh"
#include "fancysoft/nxc/onyx/parser.hh"
#include "fancysoft/nxc/program.hh"

namespace Fancysoft::NXC {

void Program::compile(std::filesystem::path entry) {
  const auto module = std::make_shared<Module>(entry);
  this->_modules.emplace(entry, module);
  module->parse();
  module->file->cst->inspect(std::cerr);
}

} // namespace Fancysoft::NXC
