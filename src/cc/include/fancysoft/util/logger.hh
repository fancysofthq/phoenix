#pragma once

#include <iostream>
#include <string>
#include <variant>
#include <vector>

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
    None,
  };

  Verbosity verbosity;
  static const char *verbosity_to_string(Verbosity verbosity);

  bool enable_time_output = true;
  bool enable_thread_id_output = true;

  Logger(Verbosity, std::ostream &);

#define DEF_LOGGING(LEVEL)                                                     \
  std::ostream &LEVEL(const char *context = NULL);                             \
  std::ostream &LEVEL(const std::vector<const char *> context);

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
  void _output_header(
      Verbosity, std::variant<const char *, std::vector<const char *>> context);
  void _output_time();
};

extern Logger logger;

} // namespace Util
} // namespace Fancysoft
