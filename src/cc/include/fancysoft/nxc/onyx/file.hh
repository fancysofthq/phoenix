#pragma once

#include "../file.hh"
#include "../logger.hh"
#include "./ast.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// An Onyx source file.
struct File : NXC::File, std::enable_shared_from_this<File> {
  File(
      std::filesystem::path path,
      std::weak_ptr<Program> program,
      std::shared_ptr<Logger> logger) :
      NXC::File(path), Module(program), _logger(logger) {}

  /// Parse the file's CST. Subsequent calls would trigger re-parsing.
  Position parse() override;

  bool unparse() override {
    this->_cst = nullptr;
    return Unit::unparse();
  }

  /// Compile the file's AST. Would `parse()` if not parsed yet. Subsequent
  /// calls would trigger re-compilation.
  void compile() override;

  bool uncompile() override {
    this->_ast = nullptr;
    return Module::uncompile();
  }

  /// Generate LLIR from the file's AST. Would `compile()` if not compiled yet.
  void codegen(std::unique_ptr<llvm::Module>) override;

  const CST *cst() { return _cst.get(); }
  AST *ast() { return _ast.get(); }

private:
  std::shared_ptr<Logger> _logger;
  std::unique_ptr<const CST> _cst;
  std::unique_ptr<AST> _ast;
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
