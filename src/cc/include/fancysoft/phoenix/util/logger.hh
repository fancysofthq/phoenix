#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "./null_stream.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Util {

struct Logger : std::enable_shared_from_this<Logger> {
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

  const std::vector<const char *> path;
  bool enable_time_output = true;
  bool enable_thread_id_output = true;

  Logger(
      Verbosity verbosity,
      std::ostream &output,
      std::vector<const char *> path = {},
      std::shared_ptr<const Logger> parent = nullptr) :
      verbosity(verbosity), _output(output), path(path), _parent(parent) {}

  std::shared_ptr<Logger> fork(const char *sub_path) const;
  std::shared_ptr<Logger> fork(std::vector<const char *> sub_path) const;

  // Calling a level method without an argument returns a reference to the
  // logging stream with header already print.
  //
  //  ```
  //  logger.sdebug() << "Hello, world!\n";
  //  // -> [D][path] Hello, world!
  //  // ->
  //  ```
  //
  // Calling a level method with a string argument outputs a log line.
  //
  // ```
  // logger.debug("Hello, world!");
  // // -> [D][path] Hello, world!
  // // ->
  // ```
  //

#define DEF_LOGGING(LEVEL)                                                     \
  std::ostream &s##LEVEL();                                                    \
  void LEVEL(std::string);

  DEF_LOGGING(fatal)
  DEF_LOGGING(error)
  DEF_LOGGING(warn)
  DEF_LOGGING(info)
  DEF_LOGGING(debug)

#undef DEF_LOGGING

  std::ostream &strace(
      int line = __builtin_LINE(),
      const char *file = __builtin_FILE(),
      const char *function = __builtin_FUNCTION());

  void trace(
      std::string = "",
      int line = __builtin_LINE(),
      const char *file = __builtin_FILE(),
      const char *function = __builtin_FUNCTION());

private:
  const std::shared_ptr<const Logger> _parent;
  std::ostream &_output;
  Util::NullStream _null_stream;

  void _output_header(Verbosity) const;
  void _output_time() const;

  /// Return `true` if output anything.
  bool _output_path(std::ostream &, bool first = true) const;
};

} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft
