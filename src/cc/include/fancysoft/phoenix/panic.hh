#pragma once

// This file defines Onyx panic thrown when a program is ill-formed.
//

#include "./placement.hh"
#include <stdexcept>

namespace Fancysoft {
namespace Phoenix {

/// The compiler panics when a program is ill-formed.
struct Panic : std::runtime_error {
  /// The standardized panic identifier.
  enum ID {
    UnexpectedEOF = 1,
    UndeclaredReference = 2,
    DeclarationCategoryMismatch = 3,
    AlreadyDeclared = 4,
  };

  /// A panic may contain multiple notes to aid resolving the issue.
  struct Note {
    /// The note message.
    std::string message;

    /// The note placement in source code.
    std::optional<Placement> placement;

    Note(std::string message, std::optional<Placement> placement = std::nullopt) :
        message(message), placement(placement) {}
  };

  /// The panic itself, without notes.
  Note self;

  /// The list of optional panic notes.
  std::vector<Note> notes;

  Panic(
      ID id,
      std::string message,
      std::optional<Placement> placement = std::nullopt,
      std::vector<Note> notes = {}) :
      std::runtime_error(message), self({message, placement}), notes(notes) {}
};

} // namespace Phoenix
} // namespace Fancysoft
