#pragma once

#include <memory>

#include "./c/ast.hh"
#include "./onyx/ast.hh"
#include "./onyx/file.hh"

namespace Fancysoft {
namespace NXC {

class Module {
  std::unique_ptr<Onyx::AST::Root> _onyx_ast;
  std::unique_ptr<C::AST::Root> _c_ast;

  bool _parsed;
  bool _compiled;

public:
  const std::shared_ptr<Onyx::File> file;

  Module(std::filesystem::path path) :
      file(std::make_shared<Onyx::File>(path)) {}

  /// Parse the modules's contents into CST.
  ///
  /// Future note: a non-Lua macro implementation would be able to report
  /// indentation without actually evaluating the code.
  void parse();
  bool parsed() const { return _parsed; };

  /// Compile the module's CST into an AST with macros expanded. After this
  /// step, the module's entities are ready to be imported and specialized. Note
  /// that the implicit top-level main function is specialized in the end of
  /// this step, meaning that HLIR is already being written into.
  void compile();
  bool compiled() const { return _compiled; };

  /// Translate the module's HLIR into LLIR, returning an LLVM module instance.
  // void translate();
};

} // namespace NXC
} // namespace Fancysoft
