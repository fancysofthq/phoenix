#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"

#include "../logger.hh"
#include "./block.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

struct AST {
  enum BuiltInType {
    Void,
    Char,
  };

  struct TypeRef {
    const std::shared_ptr<const CST::TypeRef> cst;

    BuiltInType type;
    unsigned pointer_depth;

    TypeRef(
        std::shared_ptr<const CST::TypeRef> cst,
        BuiltInType type,
        unsigned pointer_depth) :
        cst(cst), type(type), pointer_depth(pointer_depth) {}

    llvm::Type *codegen(llvm::Module *, Logger *) const;
  };

  struct FuncDecl {
    struct Arg {
      const std::shared_ptr<const CST::FuncDecl::Arg> cst;

      TypeRef type;
      std::optional<std::string> id;

      Arg(std::shared_ptr<const CST::FuncDecl::Arg> cst,
          TypeRef type,
          std::optional<std::string> id) :
          cst(cst), type(type), id(id) {}
    };

    struct VArg {
      const std::shared_ptr<const CST::FuncDecl::VArg> cst;
      VArg(std::shared_ptr<const CST::FuncDecl::VArg> cst) : cst(cst) {}
    };

    const std::shared_ptr<const CST::FuncDecl> cst;

    std::string id;
    TypeRef return_type;
    std::vector<Arg> args;
    std::optional<VArg> varg;

    FuncDecl(
        std::shared_ptr<const CST::FuncDecl> cst,
        std::string id,
        TypeRef return_type,
        std::vector<Arg> args,
        std::optional<VArg> varg) :
        cst(cst), id(id), return_type(return_type), args(args), varg(varg) {}

    llvm::Function *codegen(llvm::Module *, Logger *) const;
  };

  AST(std::shared_ptr<Logger> logger) : _logger(logger) {}

  /// Compile a C CST into this AST.
  void compile(const CST *);

  std::shared_ptr<FuncDecl> search_func_decl(std::string id);
  void codegen(llvm::Module *module) const;

private:
  static std::optional<BuiltInType> _search_c_builtin_type(std::string id);

  std::shared_ptr<Logger> _logger;
  std::unordered_map<std::string, std::shared_ptr<FuncDecl>> _func_decls;

  void _compile(std::shared_ptr<const CST::FuncDecl>);
  TypeRef _compile(std::shared_ptr<const CST::TypeRef>);
  FuncDecl::Arg _compile(std::shared_ptr<const CST::FuncDecl::Arg>);
  FuncDecl::VArg _compile(std::shared_ptr<const CST::FuncDecl::VArg>);
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
