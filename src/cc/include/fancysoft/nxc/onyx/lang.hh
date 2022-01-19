#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// Onyx language features.
namespace Lang {

enum class Storage { Undefined, Static, Instance };
enum class Safety { Unsafe, Fragile, Threadsafe };

static const std::string safety_string(Safety safety, bool bang = false) {
  switch (safety) {
  case Safety::Unsafe:
    return bang ? "unsafe!" : "unsafe";
  case Safety::Fragile:
    return bang ? "fragile!" : "fragile";
  case Safety::Threadsafe:
    return bang ? "threadsafe!" : "threadsafe";
  }
}

/// A declaration action, such as "declaration" or "implementation".
enum class Action { Decl, Impl, Def, Reimpl };

/// A variable writeability.
enum class Writeability { Let, Final };

enum class LiteralRestriction {
  Bool, ///< `\bool`
  UInt, ///< `\uint`
};

/// A full-path identifier, e.g. `Foo::Bar<T: U>:baz`.
struct Id {
  struct Element {
    enum Access {
      Self,     ///< Empty, errornous for succeeding elements.
      Static,   ///< `::`
      Instance, ///< `:`
      Member,   ///< `.`
    };

    const std::string name;
    const std::optional<Access> access;
    const std::vector<Id> template_vargs;
    const std::unordered_map<std::string, Id> template_kwargs;

    Element(
        std::string name,
        std::optional<Access> access = std::nullopt,
        std::vector<Id> template_vargs = {},
        std::unordered_map<std::string, Id> template_kwargs = {}) :
        name(name),
        access(access),
        template_vargs(template_vargs),
        template_kwargs(template_kwargs) {}
  };

  using Path = std::vector<Element>;
  const Path path;

  Id(Path path) : path(path) {}

  /// Return a copy of self with the first path element removed.
  Id shift() const { return std::vector(path.begin() + 1, path.end()); }

  /// Return a copy of self with the last path element removed.
  Id pop() const { return std::vector(path.begin(), path.end() - 1); }
};

} // namespace Lang
} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
