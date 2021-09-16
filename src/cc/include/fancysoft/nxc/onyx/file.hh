#pragma once

#include "../file.hh"
#include "../module.hh"
#include "./hlir.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// An Onyx source file.
struct File : NXC::File, Module, std::enable_shared_from_this<File> {
  using NXC::File::File;

  /// Parse the file.
  Position parse() override;

  /// Compile the file. Would parse implicitly if not parsed yet.
  void compile(Program *) override;

  const AST *ast() { return _ast.get(); }
  const HLIR *hlir() { return _hlir.get(); }

private:
  std::unique_ptr<const AST> _ast;
  std::shared_ptr<const HLIR> _hlir;
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
