#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>

#include <fmt/core.h>

#include "./unit.hh"

namespace Fancysoft {
namespace NXC {

template <class CSTT> struct File : Unit {
  struct OpenError : std::runtime_error {
    const std::filesystem::path path;

    OpenError(std::filesystem::path path) :
        std::runtime_error(
            fmt::format("Failed to open file at {}", path.string())),
        path(path) {}
  };

  const std::filesystem::path path;

  /// The CST root representing the exact unit structure.
  /// The CST itself is compiled after the `parse()` call.
  const std::shared_ptr<CSTT> cst = std::make_shared<CSTT>();

  File(std::filesystem::path path) : path(path) {}

  virtual std::istream &source_stream() override = 0;
  // virtual std::string source() const override;
};

} // namespace NXC
} // namespace Fancysoft
