#include <ios>
#include <iostream>
#include <memory>

#include "fancysoft/nxc/program.hh"
#include "fancysoft/util/logger.hh"

namespace Fancysoft::NXC {

Program::Program(std::filesystem::path entry) {
  const auto module = std::make_shared<Onyx::File>(entry);
  _modules[entry] = module;
  _entry_module = module;
}

void Program::compile() {
  assert(!_compiled);
  _entry_module->compile(this);
  _compiled = true;
}

void Program::emit_hlir(std::filesystem::path output_dir) {
  assert(_compiled);

  for (auto &module : _modules) {
    auto path = module.first;
    path += ".nxhlir";

    Util::logger.debug("Program") << "Emitting HLIR to " << path << std::endl;

    auto file = std::ofstream(path, std::ios::trunc);
    if (!file.good())
      throw "Could not open " + path.string() + " for writing";

    module.second->hlir()->write(file);
    Util::logger.trace("Program")
        << "Successfully written HLIR to " << path << std::endl;
  }
}

} // namespace Fancysoft::NXC
