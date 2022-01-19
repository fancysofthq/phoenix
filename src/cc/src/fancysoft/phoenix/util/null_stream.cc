#include "fancysoft/phoenix/util/null_stream.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Util {

NullStream::NullStream() : std::ostream(nullptr) {}
NullStream::NullStream(const NullStream &) : std::ostream(nullptr) {}

template <class T>
const NullStream &operator<<(NullStream &&os, const T &value) {
  return os;
}

} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft
