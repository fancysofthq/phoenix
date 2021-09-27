#pragma once

#include <filesystem>
#include <memory>
#include <optional>

namespace Fancysoft {
namespace NXC {

struct Program;

/// A workspace hosts a number of programs to be built. All of its programs
/// share the same cache and reference index. Direct paths (e.g. `"foo/bar"`)
/// within the workspace programs are resolved relative to *root*.
struct Workspace {
  std::filesystem::path root;
  std::optional<std::filesystem::path> cache_dir;

  std::optional<std::filesystem::path> lto_cache_dir() {
    if (!cache_dir)
      return std::nullopt;
    else {
      auto dir = cache_dir.value() / "./lto_cache/";
      std::filesystem::create_directories(dir);
      return dir;
    }
  }
  // std::vector<std::shared_ptr<Program>> programs;
};

} // namespace NXC
} // namespace Fancysoft
