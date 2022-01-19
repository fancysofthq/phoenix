#pragma once

#include "llvm/IR/Module.h"

namespace Fancysoft {
namespace NXC {
namespace MLIR {

/// A generic MLIR module containing concrete specializations.
struct Module {
  /// Lower to an LLVM module, appending LLIR to it.
  virtual void codegen(llvm::Module *) = 0;
};

} // namespace MLIR
} // namespace NXC
} // namespace Fancysoft
