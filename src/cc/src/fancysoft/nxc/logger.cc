#include <assert.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ostream>
#include <thread>

#include "fancysoft/nxc/logger.hh"
#include "fancysoft/util/variant.hh"

namespace Fancysoft::NXC {

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

std::shared_ptr<Logger> Logger::fork(std::vector<const char *> sub_path) const {
  auto child = std::make_shared<Logger>(
      this->verbosity, this->_output, sub_path, shared_from_this());
  child->enable_thread_id_output = this->enable_thread_id_output;
  child->enable_time_output = this->enable_time_output;
  return child;
}

std::shared_ptr<Logger> Logger::fork(const char *sub_path) const {
  return fork(std::vector<const char *>{sub_path});
}

void Logger::_output_header(Verbosity level) const {
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

  _output << "][";
  _output_path(_output);
  _output << "] ";
}

void Logger::_output_time() const {
  using namespace std::chrono;

  auto now = system_clock::now();
  auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
  auto a = system_clock::to_time_t(now);
  std::tm b = *std::localtime(&a);

  _output << std::put_time(&b, "%H:%M:%S");
  _output << '.' << std::setfill('0') << std::setw(3) << ms.count();
}

bool Logger::_output_path(std::ostream &o, bool first) const {
  if (_parent) {
    first = first && !_parent->_output_path(o, first);
  }

  for (auto &p : path) {
    if (first)
      first = false;
    else
      o << '/';

    o << p;
  }

  return !first;
}

#define IMPL_LOG(LEVEL, VERBOSITY)                                             \
  std::ostream &Logger::s##LEVEL() {                                           \
    if (verbosity < Logger::Verbosity::None &&                                 \
        verbosity <= Logger::Verbosity::VERBOSITY) {                           \
      _output_header(Logger::Verbosity::VERBOSITY);                            \
      return _output;                                                          \
    } else {                                                                   \
      return _null_stream;                                                     \
    }                                                                          \
  }                                                                            \
                                                                               \
  void Logger::LEVEL(std::string message) { s##LEVEL() << message << "\n"; }

IMPL_LOG(fatal, Fatal)
IMPL_LOG(error, Error)
IMPL_LOG(warn, Warn)
IMPL_LOG(info, Info)
IMPL_LOG(debug, Debug)
#undef IMPL_LOG

std::ostream &Logger::strace(int line, const char *file, const char *function) {
  if (verbosity <= Logger::Verbosity::Trace) {
    _output_header(Logger::Verbosity::Trace);
    _output << "(In `" << function << "` at " << file << ":" << line << ") ";
    return _output;
  } else {
    return _null_stream;
  }
}

void Logger::trace(
    std::string message, int line, const char *file, const char *function) {
  strace(line, file, function) << message << "\n";
}

} // namespace Fancysoft::NXC
