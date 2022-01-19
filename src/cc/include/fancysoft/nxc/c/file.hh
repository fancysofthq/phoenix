#pragma once

#include <memory>

#include "../file.hh"
#include "./ast.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

namespace MLIR {
struct Module;
}

/// A C source file.
struct File : NXC::File {
  Position parse() override;

  /// Lower `_mlir` to LLIR.
  void codegen(llvm::Module *);

private:
  /// The file's C CST.
  std::unique_ptr<CST> _cst;

  /// The file's C AST containing exclusively static linkage entities.
  /// Global entities are located in some global AST instead.
  std::unique_ptr<AST> _ast;

  /// The C MLIR module containing specializations for this source file.
  /// Note that it also contains specializations for the global AST.
  std::unique_ptr<MLIR::Module> _mlir;
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
