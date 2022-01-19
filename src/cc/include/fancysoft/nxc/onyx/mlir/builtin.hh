#pragma once

#include <variant>

namespace Fancysoft {
namespace NXC {
namespace Onyx {
namespace MLIR {
namespace Builtin {

enum class Function {
  IntAdd,
  IntCmp,
};

namespace Type {

struct Int {
  uint32_t bitsize;
};

using Any = std::variant<Int>;

} // namespace Type

} // namespace Builtin
} // namespace MLIR
} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
