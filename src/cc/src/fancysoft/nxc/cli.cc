#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <regex>
#include <variant>

#include "fancysoft/nxc/cli.hh"
#include "fancysoft/nxc/exception.hh"
#include "fancysoft/nxc/target.hh"
#include "fancysoft/nxc/workspace.hh"
#include "fancysoft/util/cli.hh"
#include "fancysoft/util/logger.hh"
#include "fancysoft/util/variant.hh"

namespace Fancysoft::NXC {

int CLI::run(int argc, const char **argv) noexcept {
  std::string progname = std::filesystem::path(argv[0]).filename().string();

  try {
    if (argc > 1) {
      if (Util::CLI::is_help(argv[1])) {
        _display_help(progname, "v0");
        return 0;
      }

      for (auto &cmd : {Compile()}) {
        if (cmd.detect(argv[1])) {
          Util::logger.trace("CLI") << "Detected command: " << cmd.name << "\n";

          try {
            return cmd.exec(argc - 2, argv + 2, progname);
          } catch (Util::CLI::Error e) {
            std::cerr << e.what() << "\n";
            return 1;
          }
        } else {
          continue;
        }
      }

      std::cerr << "Unrecognized command `" << argv[1] << "`\n";
      return 1;
    } else {
      _display_help(progname, "v0");
      return 0;
    }
  } catch (char *s) {
    std::cerr << s << "\n";
    exit(1);
  } catch (std::string s) {
    std::cerr << s << "\n";
    exit(1);
  }
}

std::optional<CLI::Compile::Payload::HelpRequest>
CLI::Compile::Payload::parse(int argc, const char **argv) {
  assert(!_parsed);

#ifdef _WIN32
  const static std::regex output_param_regex("\\/output(?:=([\\w\\.\\/-]+))?$");
  const static std::regex output_flag_regex("\\/o([\\w\\.\\/-]+)?$");
  const static char *no_output_param = "/no-output";

  const static char *emit_exe_param = "/emit=exe";
  const static char *emit_mlir_param = "/emit=mlir";
  const static char *emit_llir_param = "/emit=llir";
  const static char *no_emit_param = "/no-emit";

  const static char *emit_exe_flag = "/exe";
  const static char *emit_mlir_flag = "/emlir";
  const static char *emit_llir_flag = "/ellir";

  const static std::regex cache_param_regex("\\/cache=([\\w\\.\\/-]+)$");
  const static std::regex cache_flag_regex("\\/C([\\w\\.\\/-]+)$");
  const static char *no_cache_param = "/no-cache";

#else
#endif

  std::cmatch regex_matches;
  HelpRequest latest_help_request = HelpRequest::General;

  for (int i = 0; i < argc; i++) {
    // TODO: If unsupported flag (begins with /)

    Util::logger.trace("CLI") << "Parsing arg " << argv[i] << "\n";

    // A help request.
    if (Util::CLI::is_help(argv[i])) {
      Util::logger.trace("CLI") << "Requested compile help\n";
      return latest_help_request;
    }

    // The "output" option.
    else if (
        std::regex_match(argv[i], regex_matches, output_param_regex) ||
        std::regex_match(argv[i], regex_matches, output_flag_regex)) {
      if (this->_output.has_value())
        throw Util::CLI::Error("Already specified the output option");
      else {
        std::filesystem::path path(regex_matches[1].str());

        if (path.empty())
          throw Util::CLI::Error("Output path shall not be empty");

        Util::logger.trace("CLI") << "Set `output` to " << path << "\n";
        _output = path;
      }

      latest_help_request = HelpRequest::Output;
    }

    // The "no-output" option.
    else if (!strcmp(argv[i], no_output_param)) {
      if (this->_output.has_value()) {
        throw Util::CLI::Error("Already specified the output option");
      } else {
        Util::logger.trace("CLI") << "Set `output` to stdout\n";
        _output = std::monostate();
      }

      latest_help_request = HelpRequest::Output;
    }

    // The "emit executable" option.
    else if (
        !strcmp(argv[i], emit_exe_param) || !strcmp(argv[i], emit_exe_flag)) {
      if (this->_emit.has_value())
        throw Util::CLI::Error("Already specified the emit option");
      else {
        Util::logger.trace("CLI") << "Set `emit` to `exe`\n";
        _emit = Emit::Exe;
      }

      latest_help_request = HelpRequest::Emit;
    }

    // The "emit MLIR" option.
    else if (
        !strcmp(argv[i], emit_mlir_param) || !strcmp(argv[i], emit_mlir_flag)) {
      if (this->_emit.has_value())
        throw Util::CLI::Error("Already specified the emit option");
      else {
        Util::logger.trace("CLI") << "Set `emit` to `mlir`\n";
        _emit = Emit::MLIR;
      }

      latest_help_request = HelpRequest::Emit;
    }

    // The "emit LLIR" option.
    else if (
        !strcmp(argv[i], emit_llir_param) || !strcmp(argv[i], emit_llir_flag)) {
      if (this->_emit.has_value())
        throw Util::CLI::Error("Already specified the emit option");
      else {
        Util::logger.trace("CLI") << "Set `emit` to `llir`\n";
        _emit = Emit::LLIR;
      }

      latest_help_request = HelpRequest::Emit;
    }

    // The "no-emit" option.
    else if (!strcmp(argv[i], no_emit_param)) {
      if (this->_emit.has_value())
        throw Util::CLI::Error("Already specified the emit option");
      else {
        Util::logger.trace("CLI") << "Set `emit` to `none`\n";
        _emit = std::monostate();
      }

      latest_help_request = HelpRequest::Emit;
    }

    // The "cache" option.
    else if (
        std::regex_match(argv[i], regex_matches, cache_param_regex) ||
        std::regex_match(argv[i], regex_matches, cache_flag_regex)) {
      if (this->_cache.has_value())
        throw Util::CLI::Error("Already specified the cache option");
      else {
        std::filesystem::path path(regex_matches[1].str());

        if (path.empty())
          throw Util::CLI::Error("Cache path shall not be empty");

        Util::logger.trace("CLI") << "Set `cache` to " << path << "\n";
        _cache = path;
      }

      latest_help_request = HelpRequest::Cache;
    }

    // The "no-cache" option.
    else if (!strcmp(argv[i], no_cache_param)) {
      if (this->_cache.has_value())
        throw Util::CLI::Error("Already specified the cache option");
      else {
        Util::logger.trace("CLI") << "Set `cache` to `none`\n";
        _cache = std::monostate();
      }

      latest_help_request = HelpRequest::Cache;
    }

    else if (auto v = CLI::_try_parse_verbosity(argv[i])) {
      if (this->_logger_verbosity.has_value())
        throw Util::CLI::Error("Already specified the logger verbosity option");
      else {
        Util::logger.trace("CLI")
            << "Set `logger verbosity` to "
            << Util::Logger::verbosity_to_string(v.value()) << "\n";

        this->_logger_verbosity = v;
      }

      latest_help_request = HelpRequest::LoggerVerbosity;
    }

    // Input path, the only positional argument.
    else {
      if (_input.has_value())
        throw Util::CLI::Error("Already specified the input path");
      else {
        std::filesystem::path path(argv[i]);

        if (path.empty())
          throw Util::CLI::Error("Input path shall not be empty");

        Util::logger.trace("CLI") << "Set `input` to " << path << "\n";
        _input = path;
      }

      latest_help_request = HelpRequest::General;
    }
  }

  if (!_input.has_value())
    throw Util::CLI::Error("Missing input path");

  _parsed = true;
  return std::nullopt; // No help was requested
}

int CLI::Compile::exec(
    int argc, const char **argv, const std::string progname) const {
  auto payload = Payload();
  auto help_request = payload.parse(argc, argv);

  if (help_request.has_value()) {
    _display_help(help_request.value(), progname);
    return 0;
  }

  auto input = payload.input();

  std::optional<std::filesystem::path> cache;
  if (payload.cache().has_value()) {
    auto var = payload.cache().value();

    if (auto path = std::get_if<std::filesystem::path>(&var)) {
      cache = *path;
    } else {
      // Caching is explicitly disabled.
    }
  } else
    cache = std::filesystem::current_path().append("./.fnxccache/");

  std::optional<Payload::Emit> emit;
  if (payload.emit().has_value()) {
    auto var = payload.emit().value();

    if (auto val = std::get_if<Payload::Emit>(&var)) {
      emit = *val;
    } else {
      // Emitting is explicitly disabled.
    }
  } else
    emit = Payload::Emit::Exe; // Emit an executable by default

  std::variant<std::filesystem::path, std::ostream *, std::monostate> output;

  if (emit.has_value()) {
    // Only if we're emitting anything there is sense to have an output.
    //

    if (payload.output().has_value()) {
      output = payload.output().value();
    } else {
      // There was no output explicitly defined, would set to
      // a default value then, based on input.
      //

      auto path = input;

      switch (emit.value()) {
      case Payload::Emit::Exe:
        output = path.replace_extension(".exe");
        break;
      case Payload::Emit::MLIR:
        output = path.replace_extension(".ml");
        break;
      case Payload::Emit::LLIR:
        output = path.replace_extension(".ll");
        break;
      }
    }
  } else {
    // Emitting is explicitly disabled...
    //

    if (payload.output().has_value()) {
      // ...but output is provided. That's a error.
      throw Util::CLI::Error(
          "Shall not have output set while not emitting anything");
    }
  }

  if (emit.has_value()) {
    switch (emit.value()) {
    case Payload::Emit::Exe: {
      if (std::get_if<std::ostream *>(&output))
        throw Util::CLI::Error(
            "Can not output to stdout when emitting an executable");
      else if (std::get_if<std::monostate>(&output))
        throw Util::CLI::Error(
            "Shall have output enabled when emitting an executable");

      break;
    }
    default:; // Ok
    }
  }

  auto workspace = std::make_shared<Workspace>();
  workspace->root = std::filesystem::current_path();
  workspace->cache_dir = cache;

  Program::CompilationContext context;
  context.target = Target();
  context.target.object_file_format = Target::ObjectFileFormat::COFF;
  context.entry_path = payload.input();

  auto program = Program(context, workspace);

  try {
    if (emit.has_value()) {
      switch (emit.value()) {
      case Payload::Emit::Exe: {
        if (auto path = std::get_if<std::filesystem::path>(&output))
          try {
            program.emit_exe(*path, {}, {});
          } catch (LinkerFailure e) {
            Util::logger.error("Linker") << "Linkage failed:\n" << e.what();
            return 1;
          }
        else
          assert(false && "BUG: Unreacheable");

        break;
      }
      case Payload::Emit::MLIR: {
        if (std::get_if<std::monostate>(&output))
          program.compile_mlir();
        else
          program.emit_mlir(
              Util::Variant::downcast<
                  std::variant<std::filesystem::path, std::ostream *>>(output),
              Program::IROutputFormat::Raw);

        break;
      }
      case Payload::Emit::LLIR:
        if (std::get_if<std::monostate>(&output))
          program.compile_llir();
        else
          program.emit_llir(
              Util::Variant::downcast<
                  std::variant<std::filesystem::path, std::ostream *>>(output),
              Program::IROutputFormat::Raw);

        break;
      }
    } else {
      program.compile_mlir();
    }
  } catch (Panic panic) {
    _print(panic);
    return 1;
  }

  return 0;
}

std::optional<Util::Logger::Verbosity>
CLI::_try_parse_verbosity(const char *arg) {
#ifdef _WIN32
  const static std::regex v_fancy_regex("\\/(v{1,3})$");
  const static std::regex q_fancy_regex("\\/(q{1,3})$");

  const static std::regex v_explicit_index_regex("\\/v(\\d)$");
  const static std::regex v_explicit_level_regex(
      "\\/v(T|D|I|W|E|F|N|t|d|i|w|e|f|n)$");
#else
#error Not implemented
#endif

  std::cmatch regex_matches;

  if (std::regex_match(arg, regex_matches, v_fancy_regex)) {
    return static_cast<Util::Logger::Verbosity>(
        static_cast<int>(Util::Logger::Verbosity::Warn) -
        regex_matches[1].length());
  } else if (std::regex_match(arg, regex_matches, q_fancy_regex)) {
    return static_cast<Util::Logger::Verbosity>(
        static_cast<int>(Util::Logger::Verbosity::Warn) +
        regex_matches[1].length());
  } else if (std::regex_match(arg, regex_matches, v_explicit_index_regex)) {
    return static_cast<Util::Logger::Verbosity>(
        strtol(regex_matches[1].str().c_str(), nullptr, 0));
  } else if (std::regex_match(arg, regex_matches, v_explicit_level_regex)) {
    switch (regex_matches[1].str()[0]) {
    case 'T':
    case 't':
      return Util::Logger::Verbosity::Trace;
    case 'D':
    case 'd':
      return Util::Logger::Verbosity::Debug;
    case 'I':
    case 'i':
      return Util::Logger::Verbosity::Info;
    case 'W':
    case 'w':
      return Util::Logger::Verbosity::Warn;
    case 'E':
    case 'e':
      return Util::Logger::Verbosity::Error;
    case 'F':
    case 'f':
      return Util::Logger::Verbosity::Fatal;
    case 'N':
    case 'n':
      return Util::Logger::Verbosity::None;
    default:
      assert(false);
      throw;
    }
  } else {
    return std::nullopt;
  }
}

void CLI::Compile::_display_help(
    Payload::HelpRequest request, const std::string progname) const {
#ifdef _WIN32
  switch (request) {
  case Payload::HelpRequest::General:
    fmt::print(
        std::cout,
        "{0} compile - Compile an Onyx program\n"
        "\n"
        "Usage:\n"
        "\n"
        "{0} compile <file> [options]\n"
        "{0} c <file> [options]\n"
        "\n"
        "Options:\n"
        "\n"
        "  /output=<file>  Specify output file path\n"
        "  /output         Output the build to stdout\n"
        "  /no-output      Do not output anywhere\n"
        "\n"
        "  /emit=mlir      Emit Onyx MLIR into a single folder\n"
        "  /emit=llir      Emit LLIR into a single folder\n"
        "  /no-emit        Do not emit anything\n"
        "\n"
        "  /cache=<dir>    Specify cache directory path\n"
        "  /no-cache       Disable caching completely\n"
        "\n"
        "  /O0             Do not optimize, enable debugging\n"
        "  /O1             Slightly optimize the build, no debug\n"
        "  /O2             Fairly balanced optimization, no debug\n"
        "  /O3             Apply maximum performance optimization\n"
        "\n"
        "  /M<path>        Add an Onyx module import lookup path\n"
        "  /R<path>        Add an Onyx macro require lookup path\n"
        "\n"
        "  /t<target>      Set the compilation target triple\n"
        "  /m<feature>     Set a compilation target feature\n"
        "\n"
        "  /D<value>       Define a C preprocessor macro\n"
        "  /I<path>        Add a C include lookup path\n"
        "  /L<path>        Add a C library lookup path\n"
        "  /l<path>        Add a C library to link\n"
        "\n"
        "  /v              Set logging verbosity to [I]NFO (4)\n"
        "  /vv             Set logging verbosity to [D]EBUG (5)\n"
        "  /vvv            Set logging verbosity to [T]RACE (6)\n"
        "\n"
        "  /q              Set logging verbosity to [E]RROR (2)\n"
        "  /qq             Set logging verbosity to [F]ATAL (1)\n"
        "  /qqq            Disable logging completely ([N]ONE, 0)\n"
        "\n"
        "  /v<level>       Set verbosity level explicitly\n"
        "\n"
        "  /i              Enable CLI interactivity\n"
        "  /no-color       Disable CLI coloring\n"
        "\n"
        "  /?, /help, /h   Display help\n",
        progname);
    break;
  case Payload::HelpRequest::Output:
    fmt::print(
        std::cout,
        "{0} compile /output - Specify output for compilation\n"
        "\n"
        "By default, a compiled program contents is written to a file at "
        "`<input>.exe`. Depending on the emit and target options, different "
        "extensions may be applied.\n"
        "\n"
        "Usage:\n"
        "\n"
        "  /output=<path>, /o<path>  Specify the output file path\n"
        "  /output,        /o        Output to stdout\n"
        "    The output stream is then likely to be piped. When emitting *IR "
        "without archiving, the files are delimeted with 0x1C (file "
        "separator)\n"
        "  /no-output                Disable the output compeletely\n"
        "    In that case the program is still compiled as it would be "
        "normally, but it's not written to any stream.\n",
        progname);
    break;
  case Payload::HelpRequest::Emit:
    fmt::print(
        std::cout,
        "{0} compile /emit - Set what's being emitted as a compilation "
        "result\n"
        "\n"
        "By default, an Onyx program is written into a single executable "
        "file "
        "in format defined by the compilation target.\n"
        "It is possible to emit an intermediate representation (IR) instead, "
        "such as Onyx MLIR or LLIR. The compilation process would halt upon "
        "a "
        "completed emission.\n"
        "\n"
        "IRs are emitted into a single file by default, e.g. "
        "`<input>.nxmlir`. "
        "The output location may be altered using the `/output` option.\n"
        "\n"
        "The output format may be specified after a colon, e.g. "
        "`/emit=mlir:tar`; implicitly `raw` by default. The `raw` format "
        "separates modules with 0x1c (file separator).\n"
        "\n"
        "Usage:\n"
        "\n"
        "  /emit=<emit format>[:<output format>]\n"
        "\n"
        "  /emit=exe,  /exe    Emit a single executable (default)\n"
        "  /emit=mlir, /emlir  Emit MLIR modules\n"
        "  /emit=llir, /ellir  Emit LLIR modules\n",
        progname);
    break;
  case Payload::HelpRequest::Cache:
    fmt::print(
        std::cout,
        "{0} compile /cache - Set the directory for storing cache\n"
        "\n"
        "The compiler may use caching to speed up the build process. By "
        "default, the cache folder is `<pwd>/.fnxcache/`.\n"
        "\n"
        "Usage:\n"
        "\n"
        "  /cache=<path>, /C<path>  Specify the cache folder\n"
        "  /no-cache                Disable caching completely\n",
        progname);
    break;
  case Payload::HelpRequest::LoggerVerbosity:
    fmt::print(
        std::cout,
        "{0} compile /v - Set the logger verbosity\n"
        "\n",
        "There are 7 levels of logger verbosity defined, from the most "
        "verbose "
        "to the least: TRACE, DEBUG, INFO, WARN, ERROR, FATAL, NONE. They "
        "are "
        "numbered from 0 to 6 accordingly."
        "\n"
        "By default the logger verbosity is set to WARN. It can be overriden "
        "by setting the LOG_LEVEL environment variable, e.g. LOG_LEVEL=INFO "
        "or "
        "LOG_LEVEL=2."
        "\n"
        "An explicit CLI option would have the highest precedence. There are "
        "multiple ways to set the verbosity via CLI:\n"
        "\n"
        "  /v<L>  Where L is the desired verbosity level: either full, "
        "single-letter or index. For example, `/v1`, `/vd`, `/vD`, `/vDEBUG` "
        "are equal.\n"
        "\n"
        "  /v     Set level to INFO (WARN - 1)\n"
        "  /vv    Set level to DEBUG (WARN - 2)\n"
        "  /vvv   Set level to TRACE (WARN - 3)\n"
        "  /q     Set level to ERROR (WARN + 1)\n"
        "  /qq    Set level to FATAL (WARN + 2)\n"
        "  /qqq   Set level to NONE (WARN + 3)\n",
        progname);
    break;
  }
#else
#endif
};

void CLI::_display_help(const std::string progname, const std::string version) {
  fmt::print(
      std::cout,
#ifdef _WIN32
      "The Fancy Onyx compiler {0}\n"
      "\n"
      "Usage: {1} <command> [options]\n"
      "\n"
      "Available commands:\n"
      "\n"
      "  compile <file>  Compile an Onyx program\n"
      "  format <file>   Format an Onyx source file\n"
      "  daemon          Launch a daemon instance\n"
      "\n"
      "  version         Print the compiler version\n"
      "  license         Print the license information\n"
      "\n"
      "Common options:\n"
      "\n"
      "  /?, /help, /h   Display context-aware help\n"
#else
      "The Fancy Onyx compiler\n"
      "\n"
      "Commands:\n"
      "\n"
      "  compile <file>  Compile an Onyx program\n"
      "  parse <file>    Parse an Onyx source file AST\n"
      "  format <file>   Format an Onyx source file\n"
      "  lsp             Launch the Onyx LSP instance\n"
      "\n"
      "Options:\n"
      "\n"
      "  --help, -h      Display context-aware help\n";
#endif
      ,
      version,
      progname);
}

} // namespace Fancysoft::NXC
