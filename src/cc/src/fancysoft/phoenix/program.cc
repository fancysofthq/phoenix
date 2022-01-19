#include <memory>

#include "fancysoft/phoenix/c/file.hh"
#include "fancysoft/phoenix/onyx/file.hh"
#include "fancysoft/phoenix/program.hh"

#include <lld/Common/Driver.h>
#include <llvm/Pass.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

namespace Fancysoft::Phoenix {

void Program::compile_mlir() {
  // TODO: Implement re-compilation.
  //

  if (!_entry) {
    // May throw `File::OpenError`.
    _entry = std::make_shared<Onyx::File>(
        Onyx::File(_ctx.entry_path, weak_from_this()));
    _onyx_files[_ctx.entry_path] = _entry;
  }

  // May throw a syntax panic.
  _entry->parse();

  // 3. * Load pre-built AST & CST from memory
  //    * In development, `import * from "builtin.nx"` in each file implicitly
  //    Effectively, change how `"builtin.nx"` is resolved.

  _entry->compile(_logger->fork("file"));
}

void Program::compile_llir() {
  // TODO: Implement re-compilation.
  //

  compile_mlir();

  // TODO: Lower a file in a separate thread.
  //

  for (auto pair : _onyx_files) {
    auto module = _create_llvm_module(pair.first);
    pair.second->lower(move(module));
  }

  for (auto pair : _c_files) {
    auto module = _create_llvm_module(pair.first);
    pair.second->lower(move(module));
  }
}

Program::_LLVMContext::_LLVMContext(
    Program::Context program_context, std::shared_ptr<Util::Logger> logger) :
    raw_context(std::make_unique<llvm::LLVMContext>()) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // this->target_triple = llvm::sys::getProcessTriple();
  this->target_triple = "x86_64-pc-win32-msvc";

  std::string err;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, err);

  if (!target)
    throw "Invalid target " + err;

  auto cpu = "generic";
  auto features = "";

  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>();
  this->target_machine =
      target->createTargetMachine(target_triple, cpu, features, opt, RM);

  logger->sdebug() << "Configured target triple: "
                   << this->target_machine->getTargetTriple().getTriple()
                   << "\n";
}

std::unique_ptr<llvm::Module>
Program::_create_llvm_module(std::filesystem::path path) {
  if (!_llvm_ctx)
    _llvm_ctx.emplace(_LLVMContext(_ctx, _logger));

  auto mod =
      std::make_unique<llvm::Module>(path.string(), *_llvm_ctx->raw_context);

  mod->setTargetTriple(_llvm_ctx->target_triple);
  mod->setDataLayout(_llvm_ctx->target_machine->createDataLayout());

  return mod;
}

void Program::_link(
    std::filesystem::path exe_path,
    std::vector<std::filesystem::path> lib_paths,
    std::vector<std::string> linked_libs) {
  compile_llir();
  // * Compile each file's object file.
  // * Run the linker on these files.
}

} // namespace Fancysoft::Phoenix
