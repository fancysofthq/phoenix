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

namespace C {
struct File;
}

/// A program represents an entire Onyx project source code base. Once created,
/// it may be used to continiously re-compile the project (useful for LSP).
struct Program {
  /// The Intermediate Representation (i.e. non-executable) output format.
  enum class IROutputFormat {
    Raw, ///< Amalgamation comprised of files separated with `0x1C`.
    // Tar, ///< A tarball archive.
  };

  /// The program's compilation context, which may change over time.
  struct CompilationContext {
    /// The target machine configuration.
    NXC::Target target;

    /// The map of environment variables.
    std::map<std::string, std::string> environment_variables;

    /// The map of C preprocessor definitions (`-D`).
    std::map<std::string, std::string> c_preprocessor_definitions;

    /// The list of C include paths (`-I`).
    std::vector<std::filesystem::path> c_include_paths;

    /// The list of Onyx import paths (`-M`).
    std::vector<std::filesystem::path> onyx_import_paths;

    /// The list of Onyx macro require paths (`-R`).
    std::vector<std::filesystem::path> onyx_macro_require_paths;
  };

  /// The workspace containing the program.
  const std::shared_ptr<Workspace> workspace;

  /// Create a program. It is not compiled yet.
  Program(
      std::shared_ptr<Workspace>,
      std::filesystem::path entry_path,
      CompilationContext);

  /// Compile the program into MLIR without lowering it yet.
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
  struct _LLVMContext {
    const std::unique_ptr<llvm::LLVMContext> raw_context;
    std::string target_triple;
    llvm::TargetMachine *target_machine;
    llvm::legacy::PassManager pass_manager;
    _LLVMContext();
  };

  CompilationContext _compilation_ctx;
  std::unique_ptr<_LLVMContext> _llvm_ctx;
  std::shared_ptr<Onyx::File> _entry_file;

  std::map<std::filesystem::path, std::shared_ptr<Onyx::File>> _nx_source_files;
  std::map<std::filesystem::path, std::shared_ptr<C::File>> _c_source_files;

  /// The program-wide Onyx type specialization map.
  // std::map<Onyx::HLIR::NXTypeSkeleton,
  // std::shared_ptr<Onyx::HLIR::NXTypeSpec>>
  //     _onyx_type_specs;

  /// Compile an object file.
  /// TODO: What?
  void _compile_obj();

  /// Get object file path for *module_path*.
  /// Would create missing directories implicitly.
  std::filesystem::path _obj_path(std::filesystem::path module_path) const;

  /// Link the entire program into a single executable.
  void _link(
      std::filesystem::path exe_path,
      std::vector<std::filesystem::path> lib_paths,
      std::vector<std::string> linked_libs);
};

} // namespace NXC
} // namespace Fancysoft
