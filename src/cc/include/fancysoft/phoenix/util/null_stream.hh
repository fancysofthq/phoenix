#include <iostream>

namespace Fancysoft {
namespace Phoenix {
namespace Util {

// An output stream inspired by dev/null.
class NullStream : public std::ostream {
public:
  NullStream();
  NullStream(const NullStream &);
};

template <class T>
const NullStream &operator<<(NullStream &&os, const T &value);

} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft
