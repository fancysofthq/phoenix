#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <thread>

#include "fancysoft/util/logger.hh"

namespace Fancysoft::Util {

Logger::Logger(Verbosity verbosity, std::ostream &output) :
    verbosity(verbosity), _output(output) {}

void Logger::_output_header(Verbosity level, const char *context) {
  _output << "[";

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
  }

  if (enable_thread_id_output) {
    _output << "][@" << std::hex << std::this_thread::get_id() << std::dec;
  }

  if (enable_time_output) {
    _output << "][";
    _output_time();
  }

  _output << "]";

  if (context)
    _output << "[" << context << "]";

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
  }

IMPL_LOG(fatal, Fatal)
IMPL_LOG(error, Error)
IMPL_LOG(warn, Warn)
IMPL_LOG(info, Info)
IMPL_LOG(debug, Debug)
IMPL_LOG(trace, Trace)

#undef IMPL_LOG

} // namespace Fancysoft::Util
