#include <fmt/core.h>
#include <fmt/ostream.h>

#include "fancysoft/nxc/c/block.hh"
#include "fancysoft/nxc/onyx/file.hh"
#include "fancysoft/nxc/placement.hh"

namespace Fancysoft::NXC {

const std::vector<Placement> Placement::path() const {
  std::vector<Placement> path = {*this};

  auto unit = this->unit;
  if (C::Block *block = dynamic_cast<C::Block *>(unit.get())) {
    path.push_back(block->placement);
  }

  return path;
}

void Placement::debug(std::ostream &stream) const {
  for (auto &element : this->path()) {
    auto unit = element.unit;

    if (C::Block *block = dynamic_cast<C::Block *>(unit.get())) {
      fmt::print(
          stream,
          "In C block at {}:{}\n",
          element.location.start.row + 1,
          element.location.start.col + 1);
    } else if (Onyx::File *file = dynamic_cast<Onyx::File *>(unit.get())) {
      fmt::print(
          stream,
          "At {}:{}:{}\n",
          file->path.string(),
          element.location.start.row + 1,
          element.location.start.col + 1);
    }
  }
}

} // namespace Fancysoft::NXC
