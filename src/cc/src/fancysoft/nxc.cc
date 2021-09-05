#include <filesystem>
#include <iostream>

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

int main(int argc, char *argv[]) {
  Fancysoft::Util::logger.enable_thread_id_output = false;
  Fancysoft::Util::logger.enable_time_output = false;

  try {
    std::filesystem::path progname = argv[0];
    logger.debug() << "Compiler path: " << progname << std::endl;

    std::filesystem::path entry_path;

    switch (argc) {
    case 2:
      entry_path = argv[1];
      entry_path = progname.parent_path() / entry_path;
      break;
    default:
      throw "The compiler expects exactly one argument";
    }

    logger.debug() << "Compiling " << entry_path << std::endl;

    Fancysoft::NXC::Program program;
    program.compile(entry_path);
  } catch (const char *&s) {
    logger.fatal() << s << std::endl;
    exit(1);
  } catch (const std::string *&s) {
    logger.fatal() << s << std::endl;
    exit(1);
  } catch (const Fancysoft::NXC::Panic panic) {
    logger.error() << "Panic! " << panic.what() << std::endl;
    exit(1);
  } catch (...) {
    logger.fatal() << "Unhandled exception" << std::endl;
    exit(1);
  }
}
