#include <filesystem>
#include <iostream>
#include <stdlib.h>

#include "fancysoft/nxc/panic.hh"
#include "fancysoft/nxc/program.hh"
#include "fancysoft/util/logger.hh"

using namespace Fancysoft::Util;

Logger setup_logger() {
  Logger::Verbosity verbosity;

  if (const char *env = std::getenv("LOG_LEVEL")) {
    if (strcmp(env, "TRACE"))
      verbosity = Logger::Verbosity::Trace;
    else if (strcmp(env, "DEBUG"))
      verbosity = Logger::Verbosity::Debug;
    else if (strcmp(env, "INFO"))
      verbosity = Logger::Verbosity::Info;
    else if (strcmp(env, "WARN"))
      verbosity = Logger::Verbosity::Warn;
    else if (strcmp(env, "ERROR"))
      verbosity = Logger::Verbosity::Error;
    else if (strcmp(env, "FATAL"))
      verbosity = Logger::Verbosity::Fatal;
  } else {
#ifndef NDEBUG
    // For debug builds, the debug level is the default one.
    verbosity = Logger::Verbosity::Debug;
#else
    // For release builds, the info level is the default one.
    verbosity = Logger::Verbosity::Info;
#endif
  }

  return Logger(verbosity, std::cerr);
}

Logger Fancysoft::Util::logger = setup_logger();

/// A compiled program emission options.
enum class Emit { Exe, HLIR, LLIR };

static std::string help_string =
    "The Fancy Onyx compiler\n"
    "\n"
    "Commands:\n"
    "\n"
    "  [h]elp           -- Display this help\n"
    "  [c]ompile <file> -- Compile an Onyx program\n"
    "\n"
    "    Flags:\n"
    "\n"
    "      -exe -- Emit a single executable file (default)\n"
    "      -ehl -- Emit Onyx HLIR into current directory\n"
    "      -ell -- Emit LLVM modules into current directory\n";

int main(int argc, char *argv[]) {
  Fancysoft::Util::logger.enable_thread_id_output = false;
  Fancysoft::Util::logger.enable_time_output = false;

  try {
    std::filesystem::path progname = argv[0];
    logger.debug() << "Compiler path: " << progname << std::endl;

    if (argc == 1) {
      std::cout << help_string;
      exit(EXIT_SUCCESS);
    }

    std::string command = argv[1];

    if (!command.compare("h") || !command.compare("help")) {
      std::cout << help_string;
      exit(EXIT_SUCCESS);
    } else if (!command.compare("c") || !command.compare("compile")) {
      if (argc == 2)
        throw "An entry file path expected";

      std::filesystem::path entry_path = argv[2];
      entry_path = progname.parent_path() / entry_path;
      logger.debug() << "Compiling " << entry_path << std::endl;

      Emit emit = Emit::Exe;
      std::filesystem::path output_path = "./";

      if (argc > 3) {
        for (int i = 2; i < argc; i++) {
          std::string flag = argv[i];

          if (flag.compare("-ehl")) {
            emit = Emit::HLIR;
          } else
            throw "Unrecognized flag " + flag;
        }
      }

      Fancysoft::NXC::Program program(entry_path);
      program.compile();

      switch (emit) {
      case Emit::HLIR: {
        logger.debug() << "Emitting HLIR to " << output_path << std::endl;
        program.emit_hlir(output_path);
        break;
      }
      default:
        throw "Only HLIR emission (-ehl) is currently implemented";
      }
    } else {
      throw "Unrecognized command: " + command;
    }
  } catch (const Fancysoft::NXC::Panic panic) {
    logger.error() << "Panic! " << panic.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}
