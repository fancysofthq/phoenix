#pragma once

namespace Fancysoft {
namespace NXC {
namespace Util {

enum class Radix {
  Binary,      ///< E.g.\ 0b101010₂
  Octal,       ///< E.g.\ 0o52₈
  Decimal,     ///< E.g.\ 42₁₀
  Hexadecimal, ///< E.g.\ 0x2A₁₆
};

/// Get radix base, e.g. `10` for `Radix::Decimal`.
inline unsigned short radix_base(Radix radix) {
  switch (radix) {
  case Radix::Binary:
    return 2;
  case Radix::Octal:
    return 8;
  case Radix::Decimal:
    return 10;
  case Radix::Hexadecimal:
    return 16;
  }
}

} // namespace Util
} // namespace NXC
} // namespace Fancysoft
