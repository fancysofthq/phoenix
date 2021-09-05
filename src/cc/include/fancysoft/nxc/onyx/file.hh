#pragma once

#include <fstream>
#include <istream>
#include <memory>
#include <ostream>

#include "../c/ast.hh"
#include "../file.hh"
#include "./ast.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// A physical Onyx file.
struct File : NXC::File<CST::Root> {
  File(std::filesystem::path path) : NXC::File<CST::Root>(path) {
    _stream = std::ifstream(path);

    if (_stream.bad()) {
      throw OpenError(path);
    }
  }

  ~File() { _stream.close(); }

  std::istream &source_stream() override;

private:
  std::ifstream _stream;
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
