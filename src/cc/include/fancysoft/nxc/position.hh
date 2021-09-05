#pragma once

#include <stdint.h>

namespace Fancysoft {
namespace NXC {

struct Position {
  uint32_t row;
  uint32_t col;

  Position(uint32_t row = 0, uint32_t col = 0) : row(row), col(col) {}

  inline Position operator+(Position another) {
    if (another.row == 0) {
      return Position(row, col + another.col);
    } else {
      return Position(row + another.row, another.col);
    }
  }
};

} // namespace NXC
} // namespace Fancysoft
