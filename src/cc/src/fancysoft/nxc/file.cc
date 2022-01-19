#include "fancysoft/nxc/file.hh"

namespace Fancysoft::NXC {

std::string File::get_line(int n) {
  auto prev = _file_stream.tellg();
  _file_stream.seekg(0, std::ios::beg);

  for (int i = 0; i < n; i++)
    _file_stream.ignore(std::numeric_limits<unsigned short>::max(), '\n');

  std::stringbuf buf;
  _file_stream.get(buf, '\n');
  _file_stream.seekg(prev);

  return buf.str();
}

} // namespace Fancysoft::NXC
