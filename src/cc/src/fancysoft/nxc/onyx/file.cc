#include <memory>

#include "fancysoft/nxc/onyx/file.hh"

namespace Fancysoft::NXC::Onyx {

std::istream &File::source_stream() { return *&_stream; }

} // namespace Fancysoft::NXC::Onyx
