#pragma once

#include <map>
#include <memory>
#include <string>
#include <variant>

namespace Fancysoft {
namespace NXC {
namespace HLIR {

struct CDecl {};
struct FunctionSpecialization {};
struct ForeignFunctionSpecialization {};

struct VarDecl {};

struct Assignment {
  std::shared_ptr<VarDecl> assignee;
};

struct Call {
  std::variant<CDecl> callee;
};

struct FunctionDef {
  std::map<std::string, VarDecl> declared_variables;
};

struct Root {
  std::shared_ptr<FunctionDef> implicit_main_function;
};

} // namespace HLIR
} // namespace NXC
} // namespace Fancysoft
