#pragma once

#include <optional>
#include <stdexcept>

namespace Fancysoft {
namespace Util {

namespace CLI {
/// A CLI error thrown when, for example, a user issued a malformed command.
struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// A generic high-level CLI command, e.g. `myprogram compile`.
struct Command {
  /// The command name, e.g. `"compile"`.
  const char *const name;

  /// The command shortcut alias, if any, e.g. `"c"`.
  const std::optional<char> shortcut;

  Command(const char *const name, std::optional<char> shortcut = std::nullopt) :
      name(name), shortcut(shortcut) {}

  /// Return true if the *arg* equals to either `name` or `shortcut`.
  bool detect(const char *arg) const {
    if (!strcmp(arg, name))
      return true;
    else if (shortcut.has_value())
      return arg[0] == shortcut.value();
    else
      return false;
  }

  /// Execute the command and return the resulting (exit) code.
  virtual int
  exec(int argc, const char **argv, const std::string progname) const = 0;
};

/// Check if *arg* is an OS-specific help request option, e.g. `/?`.
static bool is_help(const char *arg) {
#ifdef _WIN32
  return !strcmp(arg, "/?") || !strcmp(arg, "/h") || !strcmp(arg, "/help");
#else
  return !strcmp(arg, "-h") || !strcmp(arg, "--help");
#endif
}

} // namespace CLI
} // namespace Util
} // namespace Fancysoft
