#pragma once

#include <iostream>
#include <string>

#include "./null_stream.hh"

namespace Fancysoft {
namespace Util {

class Logger {
public:
  enum class Verbosity {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
  };

  Verbosity verbosity;

  bool enable_time_output = true;
  bool enable_thread_id_output = true;

  Logger(Verbosity, std::ostream &);

#define DEF_LOGGING(LEVEL) std::ostream &LEVEL(const char *context = NULL);

  DEF_LOGGING(fatal)
  DEF_LOGGING(error)
  DEF_LOGGING(warn)
  DEF_LOGGING(info)
  DEF_LOGGING(debug)
  DEF_LOGGING(trace)

#undef DEF_LOGGING

private:
  std::ostream &_output;
  NullStream _null_stream;
  void _output_header(Verbosity, const char *context = NULL);
  void _output_time();
};

extern Logger logger;

} // namespace Util
} // namespace Fancysoft
