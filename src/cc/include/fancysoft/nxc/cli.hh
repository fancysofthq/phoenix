#pragma once

#include <variant>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "../util/cli.hh"
#include "../util/logger.hh"
#include "./exception.hh"
#include "./program.hh"

namespace Fancysoft {
namespace NXC {

struct CLI {
  /// Run the CLI.
  int run(int argc, const char **argv) noexcept;

private:
  /// The command to compile an Onyx program.
  struct Compile : Util::CLI::Command {
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
      std::optional<Util::Logger::Verbosity> logger_verbosity() const {
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
      std::optional<Util::Logger::Verbosity> _logger_verbosity;
    };

    Compile() : Command("compile", 'c') {}

    int exec(
        int argc, const char **argv, const std::string progname) const override;

  private:
    void _display_help(Payload::HelpRequest, const std::string progname) const;
  };

  static std::optional<Util::Logger::Verbosity>
  _try_parse_verbosity(const char *arg);

  static void
  _display_help(const std::string progname, const std::string version);

  static void _print(Panic panic) {
    auto &out = Util::logger.error();
    out << "Panic! " << panic.what();

    if (panic.placement.has_value()) {
      out << "\n";
      panic.placement->debug(out);
    }

    for (auto &note : panic.notes) {
      out << "\nNote: " << note.message << "\n";

      if (note.placement.has_value()) {
        out << "\n";
        note.placement->debug(out);
      }
    }
  }
};

} // namespace NXC
} // namespace Fancysoft
