#pragma once

#include <map>
#include <memory>

#include "./module.hh"

namespace Fancysoft {
namespace NXC {

class Program {
  std::map<std::filesystem::path, std::shared_ptr<Module>> _modules;

public:
  void compile(std::filesystem::path entry);
};

} // namespace NXC
} // namespace Fancysoft
