#pragma once

#include <llvm/IR/Module.h>

#include "../file.hh"
#include "./ast.hh"
#include "./cst.hh"
#include "./mlir.hh"

namespace Fancysoft {
namespace Phoenix {

struct Program;

namespace C {

/// A C source file.
struct File : Phoenix::File {
  File(std::filesystem::path path, std::weak_ptr<Program> program) :
      Phoenix::File(path), _program(program) {}

  Position parse() override {
    // TODO: Skip re-parsing if the file contents hasn't changed.
    if (_parsed)
      unparse();

    // TODO: Actual parsing.

    _parsed = true;
  }

  bool unparse() override {
    _cst = nullptr;
    return Unit::unparse();
  }

  void print(std::ostream &) const override;

  /// Compile the file's AST.
  void compile();

  /// Lower the file's MLIR to *llvm_module*.
  void lower(std::unique_ptr<llvm::Module> llvm_module);

protected:
  friend AST;

  /// The LLVM module linked to this file, if any.
  std::unique_ptr<llvm::Module> _llvm_module;

  /// A pointer to the containing program.
  ///
  /// NOTE: The same file may be opened by multiple program simultaneously,
  /// which implies different ASTs etc. For example, the same codebase may have
  /// both server and client programs, which reuse some of the files.
  std::weak_ptr<Program> _program;

  /// The file's local C CST.
  std::unique_ptr<CST> _cst;

  /// The file's static C AST.
  std::unique_ptr<AST> _ast;

  /// The file's static C MLIR.
  std::unique_ptr<MLIR> _mlir;
};

} // namespace C
} // namespace Phoenix
} // namespace Fancysoft
