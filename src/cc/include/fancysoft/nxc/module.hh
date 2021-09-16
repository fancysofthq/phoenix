#pragma once

#include <memory>

namespace Fancysoft {
namespace NXC {

struct Program;

/// A module to be compiled.
struct Module {
  /// Compile the module. Would throw if already `compiled()`.
  virtual void compile(Program *) = 0;

  /// Check if the module is compiled.
  bool compiled() const { return _compiled; };

protected:
  bool _compiled = false;
};

} // namespace NXC
} // namespace Fancysoft
