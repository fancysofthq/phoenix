#pragma once

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <variant>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "./exception.hh"
#include "./logger.hh"
#include "./program.hh"

namespace Fancysoft {
namespace NXC {

struct CLI {
  /// Run the CLI.
  int run(int argc, const char **argv) noexcept;

private:
  /// A CLI error thrown when, for example, a user issued a malformed command.
  struct Error : std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  /// A generic high-level CLI command, e.g. `myprogram compile`.
  struct _Command {
    /// The command name, e.g. `"compile"`.
    const char *const name;

    /// The command shortcut alias, if any, e.g. `"c"`.
    const std::optional<char> shortcut;

    _Command(
        const char *const name, std::optional<char> shortcut = std::nullopt) :
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

  /// The command to compile an Onyx program.
  struct _Compile : _Command {
    /// The compile command payload.
    /// Note that it shall NOT be aware of any default values.
    struct Payload {
      /// Type of the emitted result of a compilation.
      enum class Emit {
        Exe,  ///< Emit an executable file, `--emit=exe`.
        MLIR, ///< Emit an MLIR archive, `--emit=mlir`.
        LLIR, ///< Emit an LLIR archive, `--emit=llir`.
      };

      /// An issued help request, e.g. `compile /emit /?`.
      enum class HelpRequest {
        General,
        Output,
        Emit,
        Cache,
        LoggerVerbosity,
      };

      /// Upon an issued help request, halts parsing and returns the latest
      /// option help request. Shall only be parsed once.
      std::optional<HelpRequest> parse(int argc, const char **argv);

      /// Get the parsed input path.
      std::filesystem::path input() const {
        assert(_parsed);
        return _input.value();
      }

      /// Get the parsed output path, if any. The path, if set, is guaranteed to
      /// be non-empty. The monostate implies an explicitly disabled output
      /// (i.e. `--no-output`).
      std::optional<
          std::variant<std::filesystem::path, std::ostream *, std::monostate>>
      output() const {
        assert(_parsed);
        return _output;
      }

      /// Get the parsed emit option, if any. The monostate implies an
      /// explicitly disabled emitting (i.e. `--no-emit`).
      std::optional<std::variant<Emit, std::monostate>> emit() const {
        assert(_parsed);
        return _emit;
      }

      /// Get the parsed cache path, if any. The monostate implies an explicitly
      /// disabled caching (i.e. `--no-cache`).
      std::optional<std::variant<std::filesystem::path, std::monostate>>
      cache() const {
        assert(_parsed);
        return _cache;
      }

      /// Get the parsed logger verbosity, if any.
      std::optional<Logger::Verbosity> logger_verbosity() const {
        assert(_parsed);
        return _logger_verbosity;
      }

    private:
      bool _parsed = false;

      std::optional<std::filesystem::path> _input;

      std::optional<
          std::variant<std::filesystem::path, std::ostream *, std::monostate>>
          _output;

      std::optional<std::variant<Emit, std::monostate>> _emit;
      std::optional<std::variant<std::filesystem::path, std::monostate>> _cache;
      std::optional<Logger::Verbosity> _logger_verbosity;
    };

    _Compile() : _Command("compile", 'c') {}

    int exec(
        int argc, const char **argv, const std::string progname) const override;

  private:
    void _display_help(Payload::HelpRequest, const std::string progname) const;
  };

  /// The command to parse a source file AST.
  struct _Parse : _Command {
    _Parse() : _Command("parse", 'p') {}

    int exec(
        int argc, const char **argv, const std::string progname) const override;

  private:
    void _display_help(const std::string progname) const;
  };

  static std::optional<Logger::Verbosity> _try_parse_verbosity(const char *arg);

  static void
  _display_help(const std::string progname, const std::string version);

  static void _print(Panic panic);

  /// Check if *arg* is an OS-specific help request option, e.g. `/?`.
  static bool _is_help(const char *arg) {
#ifdef _WIN32
    return !strcmp(arg, "/?") || !strcmp(arg, "/h") || !strcmp(arg, "/help");
#else
    return !strcmp(arg, "-h") || !strcmp(arg, "--help");
#endif
  }
};

} // namespace NXC
} // namespace Fancysoft
