#pragma once

#include <string>

namespace Fancysoft {
namespace NXC {

enum class Safety { Unsafe, Fragile, Threadsafe };

static const std::string safety_name(Safety safety) {
  switch (safety) {
  case Safety::Unsafe:
    return "unsafe";
  case Safety::Fragile:
    return "fragile";
  case Safety::Threadsafe:
    return "threadsafe";
  }
}

} // namespace NXC
} // namespace Fancysoft
