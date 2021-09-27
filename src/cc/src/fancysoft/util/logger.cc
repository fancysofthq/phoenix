#include <assert.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <thread>

#include "fancysoft/util/logger.hh"
#include "fancysoft/util/variant.hh"

namespace Fancysoft::Util {

const char *Logger::verbosity_to_string(Verbosity verbosity) {
  switch (verbosity) {
  case Verbosity::Trace:
    return "TRACE";
  case Verbosity::Debug:
    return "DEBUG";
  case Verbosity::Info:
    return "INFO";
  case Verbosity::Warn:
    return "WARN";
  case Verbosity::Error:
    return "ERROR";
  case Verbosity::Fatal:
    return "FATAL";
  case Verbosity::None:
    return "NONE";
  }
}

Logger::Logger(Verbosity verbosity, std::ostream &output) :
    verbosity(verbosity), _output(output) {}

void Logger::_output_header(
    Verbosity level,
    std::variant<const char *, std::vector<const char *>> context) {
  _output << '[';

  switch (level) {
  case Verbosity::Trace:
    _output << "T";
    break;
  case Verbosity::Debug:
    _output << "D";
    break;
  case Verbosity::Info:
    _output << "I";
    break;
  case Verbosity::Warn:
    _output << "W";
    break;
  case Verbosity::Error:
    _output << "E";
    break;
  case Verbosity::Fatal:
    _output << "F";
    break;
  case Verbosity::None:
    assert(false);
    break;
  }

  if (enable_thread_id_output) {
    _output << "][@" << std::hex << std::this_thread::get_id() << std::dec;
  }

  if (enable_time_output) {
    _output << "][";
    _output_time();
  }

  _output << ']';

  std::visit(
      [this](auto &ctx) {
        using T = std::decay_t<decltype(ctx)>;

        if constexpr (std::is_same_v<T, const char *>) {
          if (ctx)
            _output << '[' << ctx << ']';
        } else if constexpr (std::is_same_v<T, std::vector<const char *>>) {
          _output << '[';

          bool first = true;
          for (auto &c : ctx) {
            if (first)
              first = false;
            else
              _output << '/';

            _output << c;
          }
          _output << ']';
        } else
          static_assert(
              Util::Variant::always_false_v<T>, "non-exhaustive visitor!");
      },
      context);

  _output << " ";
}

void Logger::_output_time() {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
  auto a = system_clock::to_time_t(now);
  std::tm b = *std::localtime(&a);

  _output << std::put_time(&b, "%H:%M:%S");
  _output << '.' << std::setfill('0') << std::setw(3) << ms.count();
}

#define IMPL_LOG(LEVEL, VERBOSITY)                                             \
  std::ostream &Logger::LEVEL(const char *context) {                           \
    if (verbosity <= Logger::Verbosity::VERBOSITY) {                           \
      _output_header(Logger::Verbosity::VERBOSITY, context);                   \
      return _output;                                                          \
    } else                                                                     \
      return _null_stream;                                                     \
  }                                                                            \
                                                                               \
  std::ostream &Logger::LEVEL(const std::vector<const char *> context) {       \
    if (verbosity <= Logger::Verbosity::VERBOSITY) {                           \
      _output_header(Logger::Verbosity::VERBOSITY, context);                   \
      return _output;                                                          \
    } else                                                                     \
      return _null_stream;                                                     \
  }

IMPL_LOG(fatal, Fatal)
IMPL_LOG(error, Error)
IMPL_LOG(warn, Warn)
IMPL_LOG(info, Info)
IMPL_LOG(debug, Debug)
IMPL_LOG(trace, Trace)

#undef IMPL_LOG

} // namespace Fancysoft::Util
