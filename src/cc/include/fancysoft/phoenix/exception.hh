#pragma once

// This file defines Phoenix exceptions, i.e. when something's wrong with the
// compiler itself.
//

#include <assert.h>
#include <stdexcept>

namespace Fancysoft {
namespace Phoenix {

struct LinkerFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Thrown when a compiler feature is not implemented yet.
struct NotImplemented : std::runtime_error {
  using std::runtime_error::runtime_error;

  struct Location {
    const char *file;
    const int line;
    const int column;

    Location(const char *file, int line, int column) :
        file(file), line(line), column(column) {}
  };

  const Location location;

  // TODO: Make it cross-platform.
  NotImplemented(
      const std::string message = "Not implemented yet",
      const char *file = __builtin_FILE(),
      int line = __builtin_LINE(),
      int column = __builtin_COLUMN()) :
      runtime_error(message), location(file, line, column) {}
};

// TODO: Make it cross-platform.
#define __PHOENIX__UNREACHEABLE(message)                                       \
  assert(false && message);                                                    \
  __builtin_unreachable();

} // namespace Phoenix
} // namespace Fancysoft
