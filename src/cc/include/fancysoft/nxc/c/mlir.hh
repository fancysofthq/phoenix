#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../mlir/module.hh"
#include "./lang.hh"

namespace Fancysoft {
namespace NXC {
namespace C {
namespace MLIR {

struct Type {
  Lang::BuiltinType type;
  unsigned pointer_depth;
};

struct Function {
  std::string name;
  Type return_type;
  std::vector<Type> args;
  bool varg;
};

// TODO: Variables.

struct Module : NXC::MLIR::Module {
  std::unordered_map<std::string, std::shared_ptr<Function>> functions;
};

} // namespace MLIR
} // namespace C
} // namespace NXC
} // namespace Fancysoft
