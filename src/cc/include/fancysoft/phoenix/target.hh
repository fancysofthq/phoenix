#pragma once

namespace Fancysoft {
namespace Phoenix {

/// A compilation target configuration.
struct Target {
  enum class ObjectFileFormat {
    COFF,
  };

  ObjectFileFormat object_file_format;
};

} // namespace Phoenix
} // namespace Fancysoft
