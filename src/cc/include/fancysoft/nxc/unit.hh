#pragma once

#include <ostream>

namespace Fancysoft {
namespace NXC {

/// A compilation unit. It may be either physical or virtual.
///
/// Known units:
///
///   * `Onyx::File`
///   * `C::Block`
///
struct Unit {
  /// Get this unit's source code stream pointer.
  /// TODO: Make it virtually writeable for macros.
  virtual std::istream &source_stream() = 0;

  // /// Get this unit's source code.
  // virtual std::string source() const;
};

} // namespace NXC
} // namespace Fancysoft
