#pragma once

#include <map>
#include <memory>

#include "./onyx/file.hh"

namespace Fancysoft {
namespace NXC {

struct Program {
  /// Create a program, starting with the *entry* source file.
  Program(std::filesystem::path entry);

  bool compiled() const { return _compiled; }

  /// Compile the program, but do not emit it just yet.
  void compile();

  /// Emit compiled Onyx HLIRs into the output directory.
  void emit_hlir(std::filesystem::path output_dir);

  // /// Emit a single compiled executable file.
  // void emit_exe(std::filesystem::path output_path);
private:
  std::shared_ptr<Onyx::File> _entry_module;

  /// TODO: Would add C physical file modules here.
  std::map<std::filesystem::path, std::shared_ptr<Onyx::File>> _modules;

  /// The program-wide Onyx type specialization map.
  // std::map<Onyx::HLIR::NXTypeSkeleton,
  // std::shared_ptr<Onyx::HLIR::NXTypeSpez>>
  //     _onyx_type_spezs;

  bool _compiled = false;
};

} // namespace NXC
} // namespace Fancysoft
