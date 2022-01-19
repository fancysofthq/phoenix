#pragma once

#include <assert.h>
#include <optional>
#include <ostream>
#include <stdexcept>

#include "./placement.hh"

namespace Fancysoft {
namespace NXC {

/// A compiler panics when the program is ill-formed.
struct Panic : std::runtime_error {
  /// A panic may contain multiple notes to aid in resolving the problem.
  struct Note {
    /// The note message.
    std::string message;

    /// The note placement.
    std::optional<Placement> placement;

    Note(
        std::string message,
        std::optional<Placement> placement = std::nullopt) :
        message(message), placement(placement) {}
  };

  /// A panic optionally points to the origin source.
  std::optional<Placement> placement;

  /// The list of optional panic notes.
  std::vector<Note> notes;

  Panic(
      std::string message,
      std::optional<Placement> placement = std::nullopt,
      std::vector<Note> notes = {}) :
      runtime_error(message), placement(placement), notes(notes) {}
};

struct LinkerFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Thrown when a compiler feature is not implemented yet.
struct Unimplemented : std::runtime_error {
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
  Unimplemented(
      const std::string message = "Unimplemented",
      const char *file = __builtin_FILE(),
      int line = __builtin_LINE(),
      int column = __builtin_COLUMN()) :
      runtime_error(message), location(file, line, column) {}
};

// TODO: Make it cross-platform.
#define __FNX__UNREACHEABLE(message)                                           \
  assert(false && message);                                                    \
  __builtin_unreachable();

} // namespace NXC
} // namespace Fancysoft
