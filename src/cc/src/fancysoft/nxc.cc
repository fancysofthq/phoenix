#include "fancysoft/nxc/cli.hh"

Fancysoft::Util::Logger setup_default_logger() {
  Fancysoft::Util::Logger::Verbosity verbosity;

  if (const char *env = std::getenv("LOG_LEVEL")) {
    if (!strcmp(env, "TRACE"))
      verbosity = Fancysoft::Util::Logger::Verbosity::Trace;
    else if (!strcmp(env, "DEBUG"))
      verbosity = Fancysoft::Util::Logger::Verbosity::Debug;
    else if (!strcmp(env, "INFO"))
      verbosity = Fancysoft::Util::Logger::Verbosity::Info;
    else if (!strcmp(env, "WARN"))
      verbosity = Fancysoft::Util::Logger::Verbosity::Warn;
    else if (!strcmp(env, "ERROR"))
      verbosity = Fancysoft::Util::Logger::Verbosity::Error;
    else if (!strcmp(env, "FATAL"))
      verbosity = Fancysoft::Util::Logger::Verbosity::Fatal;
    else if (!strcmp(env, "NONE"))
      verbosity = Fancysoft::Util::Logger::Verbosity::None;
  } else {
#ifndef NDEBUG
    // For debug builds, the DEBUG level is the default one.
    verbosity = Fancysoft::Util::Logger::Verbosity::Debug;
#else
    // For release builds, the WARN level is the default one.
    verbosity = Fancysoft::Util::Logger::Verbosity::WARN;
#endif
  }

  return Fancysoft::Util::Logger(verbosity, std::cerr);
}

Fancysoft::Util::Logger Fancysoft::Util::logger = setup_default_logger();

int main(int argc, const char *argv[]) {
  Fancysoft::Util::logger.enable_thread_id_output = false;
  Fancysoft::Util::logger.enable_time_output = false;
  Fancysoft::NXC::CLI cli{};
  return cli.run(argc, argv);
}
