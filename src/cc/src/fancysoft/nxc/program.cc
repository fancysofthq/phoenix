#include <fmt/ostream.h>

#include <fstream>
#include <iostream>
#include <llvm/Pass.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>

#include <lld/Common/Driver.h>
#include <memory>
#include <ostream>
#include <sstream>

#include "fancysoft/nxc/onyx/file.hh"
#include "fancysoft/nxc/program.hh"
#include "fancysoft/util/logger.hh"

namespace Fancysoft::NXC {

Program::Program(CompilationContext ctx, std::shared_ptr<Workspace> workspace) :
    _compilation_ctx(ctx), workspace(workspace) {
  auto module = std::make_shared<Onyx::File>(ctx.entry_path, this);
  _modules[ctx.entry_path] = module;
  _entry_module = module;
}

void Program::compile_mlir() {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";

  if (!_entry_module->compiled())
    _entry_module->compile();

  Util::logger.trace("Program") << __builtin_FUNCTION() << "() exit\n";
}

void Program::emit_mlir(
    std::variant<std::filesystem::path, std::ostream *> out,
    IROutputFormat format) {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";
  compile_mlir();

  switch (format) {
  case IROutputFormat::Raw: {
    Util::logger.debug("Program") << "Emitting MLIR, raw\n";

    std::unique_ptr<std::ofstream> file;
    std::ostream *output;

    if (auto path = std::get_if<std::filesystem::path>(&out)) {
      file = std::make_unique<std::ofstream>(*path, std::ios::trunc);
      output = file.get();
    } else {
      output = std::get<std::ostream *>(out);
    }

    bool first = true;
    for (auto &module : _modules) {
      if (first)
        first = false;
      else
        *output << '\x1C';

      module.second->mlir()->write(*output);
    }

    Util::logger.trace("Program") << "Successfully emitted MLIR\n";
  }
  }

  Util::logger.trace("Program") << __builtin_FUNCTION() << "() exit\n";
}

void Program::compile_llir() {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";
  compile_mlir();

  if (!_llvm_ctx)
    _llvm_ctx = std::make_unique<_LLVMContext>();

  for (auto &mlir_module : _modules) {
    if (mlir_module.second->lowered())
      continue;

    auto llvm_module = std::make_unique<llvm::Module>(
        mlir_module.first.string(), *_llvm_ctx->raw_context);

    llvm_module->setTargetTriple(_llvm_ctx->target_triple);
    llvm_module->setDataLayout(_llvm_ctx->target_machine->createDataLayout());

    mlir_module.second->lower(move(llvm_module));
  }

  Util::logger.trace("Program") << __builtin_FUNCTION() << "() exit\n";
}

void Program::emit_llir(
    std::variant<std::filesystem::path, std::ostream *> out,
    IROutputFormat format) {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";
  compile_llir();

  switch (format) {
  case IROutputFormat::Raw: {
    Util::logger.debug("Program") << "Emitting LLIR, raw\n";

    std::unique_ptr<std::ofstream> file;
    std::ostream *output;

    if (auto path = std::get_if<std::filesystem::path>(&out)) {
      file = std::make_unique<std::ofstream>(*path, std::ios::trunc);
      output = file.get();
    } else {
      output = std::get<std::ostream *>(out);
    }

    auto llvm_ostream = llvm::raw_os_ostream(*output);

    bool first = true;
    for (auto &module : _modules) {
      if (first)
        first = false;
      else
        *output << '\x1C';

      module.second->llir()->print(llvm_ostream, nullptr, true, true);
    }

    Util::logger.trace("Program") << "Successfully emitted LLIR\n";
  }
  }

  Util::logger.trace("Program") << __builtin_FUNCTION() << "() exit\n";
}

void Program::emit_exe(
    std::filesystem::path exe_path,
    std::vector<std::filesystem::path> lib_paths,
    std::vector<std::string> linked_libs) {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";
  _compile_obj();
  _link(exe_path, lib_paths, linked_libs);
  Util::logger.trace("Program") << __builtin_FUNCTION() << "() exit\n";
}

Program::_LLVMContext::_LLVMContext() :
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

  Util::logger.debug("Program")
      << "Configured target triple: "
      << this->target_machine->getTargetTriple().getTriple() << "\n";
}

void Program::_compile_obj() {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";
  compile_llir();

  llvm::legacy::PassManager pass;
  auto file_type = llvm::CGFT_ObjectFile;

  for (auto &module : _modules) {
    auto obj_path = _obj_path(module.first);

    std::error_code err;
    auto file =
        llvm::raw_fd_ostream(obj_path.string(), err, llvm::sys::fs::OF_None);

    if (err)
      throw "Failed to open file at " + obj_path.string() + ": " +
          err.message();

    if (_llvm_ctx->target_machine->addPassesToEmitFile(
            pass, file, nullptr, file_type))
      throw "The target machine can't emit a file of this type";

    Util::logger.trace("Program")
        << "Compiling object file at " << obj_path << "\n";

    pass.run(*module.second->llir());
    file.flush();

    Util::logger.debug("Program")
        << "Compiled object file at " << obj_path << "\n";
  }

  Util::logger.trace("Program") << __builtin_FUNCTION() << "() exit\n";
}

void Program::_link(
    std::filesystem::path exe_path,
    std::vector<std::filesystem::path> lib_paths,
    std::vector<std::string> linked_libs) {
  Util::logger.trace("Program") << __builtin_FUNCTION() << "()\n";

  lib_paths.push_back("C:\\Program Files (x86)\\Windows "
                      "Kits\\10\\Lib\\10.0.18362.0\\ucrt\\x64");
  lib_paths.push_back(
      "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.18362.0\\um\\x64");
  lib_paths.push_back(
      "C:\\Program Files (x86)\\Microsoft Visual "
      "Studio\\2019\\BuildTools\\VC\\Tools\\MSVC\\14.27.29110\\lib\\x64");

  linked_libs.push_back("ucrt");
  linked_libs.push_back("cmt");

  std::vector<std::string> args;

  if (auto lto_cache_dir = workspace->lto_cache_dir())
    args.push_back("/lldltocache:\"" + lto_cache_dir->string() + "\"");

  // args.push_back("/entry:__nx_implicit_main");
  args.push_back("/subsystem:console");
  args.push_back("/out:" + exe_path.string());

  for (auto &module : _modules) {
    auto obj_path = _obj_path(module.first);
    args.push_back(obj_path.string());
  }

  for (auto path : lib_paths) {
    args.push_back("/libpath:" + path.string());
  }

  for (auto lib : linked_libs) {
    args.push_back("/defaultlib:lib" + lib);
  }

  // args.push_back("C:\\Program Files (x86)\\Windows "
  //                "Kits\\10\\Lib\\10.0.18362.0\\ucrt\\x64\\libucrt.lib");

  fmt::print(
      Util::logger.debug("Program"), "Linker args: {}\n", fmt::join(args, " "));

  std::vector<const char *> char_args;

  for (auto &arg : args) {
    char_args.push_back(arg.c_str());
  }

  std::stringstream ssout;
  std::stringstream sserr;

  llvm::raw_os_ostream sout(Util::logger.debug("Linker") << "Output:\n");
  llvm::raw_os_ostream serr(std::cerr); // TODO: Capture the output

  bool success = !!lld::coff::link(char_args, false, sout, serr);

  if (!success) {
    throw LinkerFailure(sserr.str());
  }
}

std::filesystem::path
Program::_obj_path(std::filesystem::path module_path) const {
  auto dir =
      workspace->cache_dir.value() / "./obj/" / module_path.parent_path();
  std::filesystem::create_directories(dir);
  auto path = dir / module_path.stem();
  path.replace_extension(".o");
  return path;
}

} // namespace Fancysoft::NXC
