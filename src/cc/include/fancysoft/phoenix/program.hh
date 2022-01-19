#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <variant>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

#include "./target.hh"
#include "./util/logger.hh"

namespace Fancysoft {
namespace Phoenix {

struct Workspace;

namespace Onyx {
struct File;
}

namespace C {

struct AST;
struct File;

} // namespace C

/// A program represents an entire Onyx project source code base. Once created,
/// it may be used to continiously re-compile the project (useful for LSP).
struct Program : std::enable_shared_from_this<Program> {
  /// The Intermediate Representation (i.e. non-executable) output format.
  enum class IROutputFormat {
    Raw, ///< An amalgamation comprised of files contents separated with `0x1C`.
    // Tar, ///< A tarball archive.
  };

  /// The program's context.
  struct Context {
    /// The program's entry path.
    std::filesystem::path entry_path;

    /// The target machine configuration.
    Phoenix::Target target;

    /// The map of environment variables.
    std::map<std::string, std::string> environment_variables;

    /// The map of C preprocessor definitions (`-D`).
    std::map<std::string, std::string> c_preprocessor_definitions;

    /// The list of C include paths (`-I`).
    std::vector<std::filesystem::path> c_include_paths;

    /// The list of Onyx import paths (`-E`).
    std::vector<std::filesystem::path> onyx_import_paths;

    /// The list of Onyx macro require paths (`-M`).
    std::vector<std::filesystem::path> onyx_macro_require_paths;

    Context(
        std::filesystem::path entry_path,
        Phoenix::Target target,
        std::map<std::string, std::string> environment_variables,
        std::map<std::string, std::string> c_preprocessor_definitions,
        std::vector<std::filesystem::path> c_include_paths,
        std::vector<std::filesystem::path> onyx_import_paths,
        std::vector<std::filesystem::path> onyx_macro_require_paths) :
        entry_path(entry_path),
        target(target),
        environment_variables(environment_variables),
        c_preprocessor_definitions(c_preprocessor_definitions),
        c_include_paths(c_include_paths),
        onyx_import_paths(onyx_import_paths),
        onyx_macro_require_paths(onyx_macro_require_paths) {}
  };

  /// Create a program. It is not compiled yet.
  Program(
      std::weak_ptr<Workspace> workspace,
      Context ctx,
      std::shared_ptr<Util::Logger> logger) :
      _workspace(workspace), _ctx(ctx), _logger(logger) {}

  /// Get a pointer to the global mutable C AST.
  C::AST *c_ast() { return _global_c_ast.get(); }

  /// Compile the program MLIR without lowering it.
  void compile_mlir();

  /// Emit MLIR of the program.
  void emit_mlir(
      std::variant<std::filesystem::path, std::ostream *> output,
      IROutputFormat);

  /// Lower the program to LLIR without emitting anything.
  void compile_llir();

  /// Emit LLIR of the program.
  void emit_llir(
      std::variant<std::filesystem::path, std::ostream *> output,
      IROutputFormat);

  /// Emit the program as a single executable binary file.
  void emit_exe(
      std::filesystem::path exe_path,
      std::vector<std::filesystem::path> lib_paths,
      std::vector<std::string> linked_libs);

private:
  /// An LLVM context struct.
  struct _LLVMContext {
    const std::unique_ptr<llvm::LLVMContext> raw_context;
    std::string target_triple;
    llvm::TargetMachine *target_machine;
    llvm::legacy::PassManager pass_manager;

    /// Create an LLVM context based on program context.
    _LLVMContext(Program::Context, std::shared_ptr<Util::Logger>);
  };

  const std::shared_ptr<Util::Logger> _logger;

  /// The workspace containing the program.
  const std::weak_ptr<Workspace> _workspace;

  /// The program's context.
  Context _ctx;

  /// The entry module of the program derived from `_ctx`.
  std::shared_ptr<Onyx::File> _entry;

  /// The LLVM context of the program derived from `_ctx`.
  std::optional<_LLVMContext> _llvm_ctx;

  /// Map of Onyx files.
  std::map<std::filesystem::path, std::shared_ptr<Onyx::File>> _onyx_files;

  /// Map of C files.
  std::map<std::filesystem::path, std::shared_ptr<C::File>> _c_files;

  /// The global C AST.
  std::unique_ptr<C::AST> _global_c_ast;

  /// Create an LLVM module for file path. The pointer should then be assigned
  /// to the according source file.
  std::unique_ptr<llvm::Module> _create_llvm_module(std::filesystem::path);

  /// Get an object file path for *module_path* w.r.t. the workspace settings.
  /// Would create missing directories implicitly.
  std::filesystem::path _obj_path(std::filesystem::path module_path) const;

  /// Link the entire program into a single executable.
  void _link(
      std::filesystem::path exe_path,
      std::vector<std::filesystem::path> lib_paths,
      std::vector<std::string> linked_libs);
};

} // namespace Phoenix
} // namespace Fancysoft
