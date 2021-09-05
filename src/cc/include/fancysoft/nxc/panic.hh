#pragma once

#include <optional>
#include <ostream>
#include <stdexcept>

#include <fmt/core.h>

#include "./placement.hh"

namespace Fancysoft {
namespace NXC {

struct Panic : std::runtime_error {
  const std::optional<Placement> placement;

  Panic(
      std::string message,
      std::optional<Placement> placement = std::nullopt) :
      runtime_error(message), placement(placement) {}

  void print(std::ostream);
};

} // namespace NXC
} // namespace Fancysoft
