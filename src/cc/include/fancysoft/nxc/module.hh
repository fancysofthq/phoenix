#pragma once

#include <memory>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "./mlir.hh"

namespace Fancysoft {
namespace NXC {

struct Program;

/// A module to be compiled into MLIR.
struct Module {
  Module(Program *program) : _program(program) {}

  /// Compile the module. Shall not be already `compiled()`.
  virtual void compile() = 0;

  /// Check if the module is compiled.
  bool compiled() const { return !!_mlir; };

  /// Return MLIR pointer if `compiled()`.
  MLIR *mlir() const { return _mlir.get(); }

  /// Lower the MLIR to LLIR.
  void lower(std::unique_ptr<llvm::Module> llvm_module) {
    assert(!_llir);
    this->_llir = move(llvm_module);
    this->_mlir->lower(_llir.get());
  }

  /// Check if the MLIR is lowered to LLIR.
  bool lowered() const { return !!_llir; }

  /// Return the LLVM module pointer if `lowered()`.
  llvm::Module *llir() const { return _llir.get(); }

protected:
  Program *_program;

  /// Set after `compile()` is called.
  std::unique_ptr<MLIR> _mlir;

  /// Set after `lower()` is called.
  std::unique_ptr<llvm::Module> _llir;
};

} // namespace NXC
} // namespace Fancysoft
