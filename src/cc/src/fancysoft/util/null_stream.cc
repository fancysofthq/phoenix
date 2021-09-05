#include "fancysoft/util/null_stream.hh"

namespace Fancysoft {
namespace Util {

NullStream::NullStream() : std::ostream(nullptr) {}
NullStream::NullStream(const NullStream &) : std::ostream(nullptr) {}

template <class T>
const NullStream &operator<<(NullStream &&os, const T &value) {
  return os;
}

} // namespace Util
} // namespace Fancysoft
