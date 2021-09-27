#pragma once

#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"

#include "./target.hh"
#include "./workspace.hh"

namespace Fancysoft {
namespace NXC {

namespace Onyx {
struct File;
}

/// A program represents an entire Onyx project source code base. Once created,
/// it may be used to continiously re-compile the project (useful for the LSP).
/// An Onyx program may have multiple entry points.
struct Program {
  /// The Intermediate Representation (i.e. non-executable) output format.
  enum class IROutputFormat {
    Raw, ///< Raw, files separated with `0x1C`.
    // Tar, ///< A tarball archive.
  };

  struct CompilationContext {
    /// The target machine settings, e.g. CPU.
    NXC::Target target;

    /// The entry source file path.
    std::filesystem::path entry_path;

    /// The map of environment variables.
    std::map<std::string, std::string> environment_variables;

    /// The map of C preprocessor definitions.
    std::map<std::string, std::string> c_preprocessor_macros;

    /// The list of C include paths.
    std::vector<std::filesystem::path> c_include_paths;

    /// The list of Onyx import paths.
    std::vector<std::filesystem::path> onyx_import_paths;

    /// The list of Onyx macro require paths.
    std::vector<std::filesystem::path> onyx_macro_require_paths;
  };

  const std::shared_ptr<Workspace> workspace;

  /// Create a program. It is not compiled just yet.
  Program(CompilationContext, std::shared_ptr<Workspace>);

  /// Compile the program into MLIR without lowering it just yet.
  void compile_mlir();

  /// Emit the MLIR of the program.
  void emit_mlir(
      std::variant<std::filesystem::path, std::ostream *> output,
      IROutputFormat);

  /// Lower the program to LLIR without emitting anything.
  void compile_llir();

  /// Emit the LLIR of the program.
  void emit_llir(
      std::variant<std::filesystem::path, std::ostream *> output,
      IROutputFormat);

  /// Link the program, and emit it as a single executable file.
  void emit_exe(
      std::filesystem::path exe_path,
      std::vector<std::filesystem::path> lib_paths,
      std::vector<std::string> linked_libs);

private:
  CompilationContext _compilation_ctx;

  struct _LLVMContext {
    const std::unique_ptr<llvm::LLVMContext> raw_context;
    std::string target_triple;
    llvm::TargetMachine *target_machine;
    llvm::legacy::PassManager pass_manager;
    _LLVMContext();
  };

  std::shared_ptr<Onyx::File> _entry_module;

  /// TODO: Would add C physical file modules here.
  std::map<std::filesystem::path, std::shared_ptr<Onyx::File>> _modules;

  /// The program-wide Onyx type specialization map.
  // std::map<Onyx::HLIR::NXTypeSkeleton,
  // std::shared_ptr<Onyx::HLIR::NXTypeSpez>>
  //     _onyx_type_spezs;

  std::unique_ptr<_LLVMContext> _llvm_ctx;

  void _compile_obj();

  void _link(
      std::filesystem::path exe_path,
      std::vector<std::filesystem::path> lib_paths,
      std::vector<std::string> linked_libs);

  /// Get object file path for *module_path*.
  /// Would create missing directories implicitly.
  std::filesystem::path _obj_path(std::filesystem::path module_path) const;
};

} // namespace NXC
} // namespace Fancysoft
