#pragma once

#include <filesystem>
#include <memory>
#include <optional>

namespace Fancysoft {
namespace Phoenix {

struct Program;

/// A workspace hosts multiple of programs to build. All of the programs
/// share the same cache and reference index. Meta paths (e.g. `"foo/bar"`)
/// within the workspace fallback to relative to *root*.
struct Workspace {
  std::filesystem::path root;
  std::optional<std::filesystem::path> cache_dir;

  /// Get the LTO cache directory within *cache_dir* if set.
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

} // namespace Phoenix
} // namespace Fancysoft
