#pragma once

namespace Fancysoft {
namespace NXC {

struct Target {
  enum class ObjectFileFormat {
    COFF,
  };

  ObjectFileFormat object_file_format;
};

} // namespace NXC
} // namespace Fancysoft
