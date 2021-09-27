#pragma once

#include "../file.hh"
#include "../module.hh"
#include "./ast.hh"
#include "fancysoft/nxc/program.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// An Onyx source file.
struct File : NXC::File, Module, std::enable_shared_from_this<File> {
  File(std::filesystem::path path, Program *program) :
      NXC::File(path), Module(program) {}

  /// Parse the file.
  Position parse() override;

  /// Compile the file. Would parse implicitly if not parsed yet.
  void compile() override;

  const AST *ast() { return _ast.get(); }

private:
  std::unique_ptr<const AST> _ast;
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
