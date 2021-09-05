#pragma once

namespace Fancysoft {
namespace Util {

enum class Radix {
  Binary,      ///< E.g.\ 0b101010₂
  Octal,       ///< E.g.\ 0o52₈
  Decimal,     ///< E.g.\ 42₁₀
  Hexadecimal, ///< E.g.\ 0x2A₁₆
};

unsigned radix_to_base(Radix);

} // namespace Util
} // namespace Fancysoft
