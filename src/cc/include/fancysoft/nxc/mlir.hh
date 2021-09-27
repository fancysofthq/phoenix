#pragma once

#include <cstddef>
#include <memory>
#include <ostream>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "./c/ast.hh"
#include "./onyx/ast.hh"
#include "./program.hh"

#include "./safety.hh"
#include "./storage.hh"

#include "../util/map.hh"

namespace Fancysoft {
namespace NXC {

/// The Middle-Level Intermediate Representation.
/// Represents both C and Onyx code. It is then lowered to LLIR.
struct MLIR {
  MLIR(const Onyx::AST *, Program *);

  /// Output the MLIR.
  void write(std::ostream &) const;

  /// Lower the MLIR to an LLVM module.
  void lower(llvm::Module *) const;

private:
  enum class _CBuiltInType {
    Void,
    Char,
  };

  struct _CStringLiteral;

  using _Literal = std::variant<std::unique_ptr<_CStringLiteral>>;

  struct _CTypeRef;
  using _TypeRestriction = std::variant<_CTypeRef>;

  struct _CFuncDecl;
  struct _CCall;

  struct _VarDecl;
  struct _VarRef;
  struct _Assignment;
  struct _PointerOf;

  struct _Scope;
  struct _TopLevelScope;
  struct _Block;

  /// A scope is comprised of expressions.
  /// A scope itself may be an expression as well.
  using _Expr = std::variant<
      std::shared_ptr<_Block>,
      std::shared_ptr<_VarDecl>,
      std::shared_ptr<_CCall>,
      std::shared_ptr<_Assignment>>;

  /// An rval may be assigned to something. It may also be used as an inline
  /// expression, e.g. in `if cond() then "foo"`, where `"foo"` is an rval.
  using _RVal = Util::Variant::flatten_t<std::variant<
      _Literal,
      std::shared_ptr<_Block>,
      std::shared_ptr<_CCall>,
      std::unique_ptr<_VarRef>,
      std::unique_ptr<_PointerOf>>>;

  /// A C string literal.
  struct _CStringLiteral {
    const std::string value;
    _CStringLiteral(std::string value) : value(value) {}
    void write(std::ostream &) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> * = nullptr);
  };

  /// A C type reference, e.g. `void` or `struct foo`.
  struct _CTypeRef {
    const _CBuiltInType type;
    const unsigned short pointer_depth;

    static _CTypeRef compile(std::shared_ptr<C::AST::TypeRef> ast);

    _CTypeRef(_CBuiltInType type, unsigned short pointer_depth) :
        type(type), pointer_depth(pointer_depth) {}

    void write(std::ostream &) const;
    llvm::Type *lower(llvm::Module *) const;
  };

  /// A C function declaration, i.e. a prototype.
  struct _CFuncDecl {
    struct ArgDecl {
      const _CTypeRef type;
      const std::optional<std::string> id;

      static ArgDecl
      compile(std::shared_ptr<const C::AST::FuncDecl::ArgDecl> ast);

      ArgDecl(_CTypeRef type, std::optional<std::string> id) :
          type(type), id(id) {}

      llvm::Type *lower(llvm::Module *) const;
    };

    const std::shared_ptr<const C::AST::FuncDecl> ast;
    const _CTypeRef return_type;
    const std::string id;
    const std::vector<ArgDecl> args;

    static _CFuncDecl compile(std::shared_ptr<const C::AST::FuncDecl>);

    _CFuncDecl(
        std::shared_ptr<const C::AST::FuncDecl> ast,
        _CTypeRef return_type,
        std::string id,
        std::vector<ArgDecl> args) :
        ast(ast), return_type(return_type), id(id), args(args) {}

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Function *lower(llvm::Module *) const;
  };

  /// A C call.
  struct _CCall {
    const std::shared_ptr<_CFuncDecl> callee;
    const std::vector<_RVal> args;

    _CCall(std::shared_ptr<_CFuncDecl> callee, std::vector<_RVal> args) :
        callee(callee), args(move(args)) {}

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  /// A variable reference (by id).
  struct _VarRef {
    const std::shared_ptr<_VarDecl> decl;
    _VarRef(std::shared_ptr<_VarDecl> decl) : decl(decl) {}

    void write(std::ostream &) const;

    /// Return the LLVM reference to the declaration.
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  /// An assignment to a variable.
  struct _Assignment {
    const _VarRef lvalue;
    const _RVal rvalue;

    _Assignment(_VarRef lvalue, _RVal rvalue) :
        lvalue(lvalue), rvalue(move(rvalue)) {}

    void write(std::ostream &, unsigned indent = 0) const;
    void write_rvalue(std::ostream &) const;

    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  /// A variable declaration (or definition).
  struct _VarDecl {
    friend _VarRef;

    const std::shared_ptr<const Onyx::AST::VarDecl> ast;

    /// The variable identifier.
    const std::string id;

    /// NOTE: Must always be set.
    const _TypeRestriction type;

    /// If not set, the variable is explicitly undefined.
    std::optional<_RVal> value;

    _VarDecl(
        std::shared_ptr<const Onyx::AST::VarDecl> ast,
        std::string id,
        _TypeRestriction type,
        std::optional<_RVal> value) :
        ast(ast), id(id), type(type), value(move(value)) {}

    /// Different specializations for different scopes.
    template <typename Scope>
    void write(std::ostream &, unsigned indent = 0) const;

    /// Subsequent calls do not generate LLIR, but return the reference.
    template <typename Scope>
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *);

  private:
    llvm::Value *_llvm_ref = nullptr;
    llvm::Value *_lower_to_local(llvm::Module *, llvm::IRBuilder<> *);
    void _write_local(std::ostream &, unsigned indent = 0) const;
  };

  /// A take-pointer-of expression.
  struct _PointerOf {
    const _VarRef value;
    _PointerOf(_VarRef value) : value(value) {}
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
    void write(std::ostream &) const;
  };

  /// An abstract scope with its own safety and storage.
  struct _Scope : std::enable_shared_from_this<_Scope> {
    const Safety safety;
    const Storage storage;
    const std::shared_ptr<_Scope> parent;

    _Scope(Safety safety, Storage storage, std::shared_ptr<_Scope> parent) :
        safety(safety), storage(storage), parent(parent) {}

    std::shared_ptr<_VarDecl>
        compile_var_decl(std::shared_ptr<Onyx::AST::VarDecl>);

    std::shared_ptr<_CCall> compile_c_call(std::shared_ptr<Onyx::AST::CCall>);
    _RVal compile_rval(Onyx::AST::RVal);
    _RVal compile_rval(std::shared_ptr<Onyx::AST::ExplicitSafetyStatement>);
    void compile_extern_directive(std::shared_ptr<Onyx::AST::ExternDirective>);
    void compile_c_ast(const C::AST *);

  protected:
    std::vector<std::shared_ptr<_Scope>> _children;
    std::vector<_Expr> _exprs;

    /// Variable declaration index for the current scope.
    /// NOTE: It is automatically updated upon an `_add_expr` call.
    std::map<std::string, std::shared_ptr<_VarDecl>> _var_decl_index;

    /// The map of C functions declared in this scope.
    std::map<std::string, std::shared_ptr<_CFuncDecl>> _c_func_decls;

    /// Infer a type restriction from an rval.
    _TypeRestriction _infer(_RVal *);

    /// Search for a variable declaration within self and parent scope(s).
    std::shared_ptr<_VarDecl> _search_var_decl(std::string id);

    /// Search for an existing C function declaration, or return nullptr.
    std::shared_ptr<_CFuncDecl> _search_c_func_decl(std::string id);

    /// Add a C function declaration. Would panic if already declared.
    void _add_c_func_decl(std::shared_ptr<const C::AST::FuncDecl>);

    /// Add an expression to the scope. Implicitly updates `_var_decl_index`.
    void _add_expr(_Expr);

    /// Add a child scope.
    template <typename T>
    std::shared_ptr<T> _create_child(Safety safety, Storage storage) {
      auto ptr = std::make_shared<T>(T(safety, storage, shared_from_this()));
      _children.push_back(ptr);
      return ptr;
    }
  };

  /// The top-level scope.
  struct _TopLevelScope : _Scope {
    _TopLevelScope() : _Scope(Safety::Fragile, Storage::Static, nullptr) {}

    void write(std::ostream &) const;
    void lower(llvm::Module *) const;
  };

  /// A simple block of code, e.g. a condition branch.
  struct _Block : _Scope {
    using _Scope::_Scope;

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  Program *_program;

  const std::shared_ptr<_TopLevelScope> _top_level_scope =
      std::make_shared<_TopLevelScope>();

  static std::optional<_CBuiltInType> _search_c_built_in_type(std::string id);
  static void _write(_CBuiltInType, std::ostream &);

  /// Return `true` if *id* is a reserved C keyword or built-in type.
  static bool _is_c_reserved(std::string id);
};

} // namespace NXC
} // namespace Fancysoft
