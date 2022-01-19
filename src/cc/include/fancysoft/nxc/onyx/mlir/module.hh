#pragma once

#include "../../mlir/module.hh"
#include "../mlir.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {
namespace MLIR {

struct Module : NXC::MLIR::Module {
  void codegen(llvm::Module *) override;

private:
  Block _implicit_main_function_body;
  std::vector<std::shared_ptr<Function>> _functions;
};

} // namespace MLIR
} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
