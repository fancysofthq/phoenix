#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "./unit.hh"

namespace Fancysoft {
namespace Phoenix {

/// An abstract source file unit to read the source code from.
struct File : Unit {
  struct OpenError : std::runtime_error {
    const std::filesystem::path path;

    OpenError(std::filesystem::path path) :
        std::runtime_error("Failed to open file " + path.string()),
        path(path) {}
  };

  const std::filesystem::path path;

  File(std::filesystem::path path) : path(path) {
    _file_stream = std::ifstream(path);

    if (_file_stream.bad()) {
      throw OpenError(path);
    }
  }

  ~File() { _file_stream.close(); }

  virtual Position parse() override = 0;
  std::istream &source_stream() override { return *&_file_stream; }
  std::string get_line(int n);

  /// TODO:
  /// Get a file view peek at specific *line* and *column*.
  /// Also returns the resulting offset for the column.
  std::tuple<std::string, unsigned> get_view(
      unsigned line,           // Begins from 0
      unsigned column,         // Begins from 0
      unsigned view_size = 69, // Nice
      unsigned max_left_padding = 20);

protected:
  std::ifstream _file_stream;
};

} // namespace Phoenix
} // namespace Fancysoft
