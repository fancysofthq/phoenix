#pragma once

#include <cstddef>
#include <memory>
#include <ostream>
#include <sstream>
#include <vcruntime.h>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include <fancysoft/util/map.hh>

#include "./c/ast.hh"
#include "./onyx/ast.hh"
#include "./program.hh"

#include "./safety.hh"
#include "./storage.hh"

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
  struct _IntPrimitive;
  using _ConcreteType = std::variant<_CTypeRef, _IntPrimitive>;

  struct _CFuncDecl;
  struct _CCall;

  struct _VarDecl;
  // struct _VarRef;

  struct _FuncDecl;

  struct _Call;
  struct _Assignment;
  // struct _PointerOf;

  struct _Scope;
  struct _TopLevelScope;
  struct _Block;

  /// A scope is comprised of expressions.
  /// A scope itself may be an expression as well.
  using _Expr = std::variant<
      std::shared_ptr<_Block>,
      std::shared_ptr<_VarDecl>,
      std::shared_ptr<_FuncDecl>,
      std::shared_ptr<_CCall>,
      std::shared_ptr<_Call>,
      std::shared_ptr<_Assignment>>;

  /// An rval may be assigned to something. It may also be used as an inline
  /// expression, e.g. in `if cond() then "foo"`, where `"foo"` is an rval.
  using _RVal = Util::Variant::Flatten<std::variant<
      _Literal,
      std::shared_ptr<_Block>,
      std::shared_ptr<_CCall>,
      std::shared_ptr<_Call>,
      std::shared_ptr<_VarDecl>
      // std::unique_ptr<_VarRef>
      >>;

  /// A C string literal.
  struct _CStringLiteral {
    const std::string value;
    _CStringLiteral(std::string value) : value(value) {}
    void write(std::ostream &) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> * = nullptr);
  };

  /// A C type reference, e.g. `void` or `struct foo*`.
  struct _CTypeRef {
    /// TODO: Allow non-built-in types.
    const _CBuiltInType type;

    const unsigned short pointer_depth;

    static _CTypeRef compile(std::shared_ptr<C::AST::TypeRef> ast);

    _CTypeRef(_CBuiltInType type, unsigned short pointer_depth) :
        type(type), pointer_depth(pointer_depth) {}

    void write(std::ostream &) const;
    llvm::Type *lower(llvm::Module *) const;
  };

  /// An integer primitive type, lowered to `iN`.
  struct _IntPrimitive {
    const bool is_signed;
    const unsigned bitsize;

    _IntPrimitive(bool is_signed, unsigned bitsize) :
        is_signed(is_signed), bitsize(bitsize) {}

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

    struct VArgDecl {
      const std::shared_ptr<const C::AST::FuncDecl::VArg> ast;
      VArgDecl(std::shared_ptr<const C::AST::FuncDecl::VArg> ast) : ast(ast) {}
    };

    const std::shared_ptr<const C::AST::FuncDecl> ast;
    const _CTypeRef return_type;
    const std::string id;
    const std::vector<std::variant<ArgDecl, VArgDecl>> args;

    static _CFuncDecl compile(std::shared_ptr<const C::AST::FuncDecl>);

    _CFuncDecl(
        std::shared_ptr<const C::AST::FuncDecl> ast,
        _CTypeRef return_type,
        std::string id,
        std::vector<std::variant<ArgDecl, VArgDecl>> args) :
        ast(ast), return_type(return_type), id(id), args(args) {}

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Function *lower(llvm::Module *) const;
  };

  /// A C call.
  struct _CCall {
    /// A resolved reference to a C function prototype.
    const std::shared_ptr<_CFuncDecl> callee;

    const std::vector<_RVal> args;

    _CCall(std::shared_ptr<_CFuncDecl> callee, std::vector<_RVal> args) :
        callee(callee), args(move(args)) {}

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  // /// A variable reference (by id).
  // struct _VarRef {
  //   /// A resolved reference to a variable declaration.
  //   const std::shared_ptr<_VarDecl> decl;

  //   _VarRef(std::shared_ptr<_VarDecl> decl) : decl(decl) {}

  //   void write(std::ostream &) const;

  //   /// Return the LLVM reference to the declaration.
  //   llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  // };

  /// An assignment to a variable.
  struct _Assignment {
    const std::shared_ptr<_VarDecl> lvalue;
    const _RVal rvalue;

    _Assignment(std::shared_ptr<_VarDecl> lvalue, _RVal rvalue) :
        lvalue(lvalue), rvalue(move(rvalue)) {}

    void write(std::ostream &, unsigned indent = 0) const;
    void write_rvalue(std::ostream &) const;

    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  /// A variable declaration (or definition).
  struct _VarDecl {
    // friend _VarRef;

    const std::shared_ptr<const Onyx::AST::Var> ast;

    /// The variable identifier.
    const std::string id;

    const _ConcreteType type;

    /// If not set, the variable is explicitly `undefined`.
    std::optional<_RVal> value;

    _VarDecl(
        std::shared_ptr<const Onyx::AST::Var> ast,
        std::string id,
        _ConcreteType type,
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

  /// A function declaration (or definition).
  struct _FuncDecl {
    const std::shared_ptr<const Onyx::AST::Func> ast;
    const std::string id;
    const _ConcreteType return_type;
    const std::vector<std::shared_ptr<_VarDecl>> args;
    const std::shared_ptr<_Block> body;

    _FuncDecl(
        std::shared_ptr<const Onyx::AST::Func> ast,
        std::string id,
        _ConcreteType return_type,
        std::vector<std::shared_ptr<_VarDecl>> args,
        std::shared_ptr<_Block> body = nullptr) :
        ast(ast), id(id), return_type(return_type), args(args), body(body) {}

    void write(std::ostream &, unsigned indent = 0) const;

    /// Subsequent calls do not generate LLIR,
    /// but return the function reference.
    llvm::Value *lower(llvm::Module *, std::string name_prefix);
  };

  struct _Call {
    const std::shared_ptr<const Onyx::AST::Call> ast;
    const std::shared_ptr<_FuncDecl> callee;
    const std::vector<_RVal> args;

    _Call(
        std::shared_ptr<const Onyx::AST::Call> ast,
        std::shared_ptr<_FuncDecl> callee,
        std::vector<_RVal> args) :
        ast(ast), callee(callee), args(move(args)) {}

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *);
  };

  // /// A take-pointer-of expression.
  // struct _PointerOf {
  //   const _VarRef value;
  //   _PointerOf(_VarRef value) : value(value) {}
  //   llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  //   void write(std::ostream &) const;
  // };

  /// An abstract scope with its own safety and storage.
  struct _Scope : std::enable_shared_from_this<_Scope> {
    /// A parent-scope-local unique id of the scope.
    const std::string id;

    const Safety safety;
    const Storage storage;
    const std::shared_ptr<_Scope> parent;

    _Scope(
        std::string id,
        Safety safety,
        Storage storage,
        std::shared_ptr<_Scope> parent) :
        id(id), safety(safety), storage(storage), parent(parent) {}

    _ConcreteType compile_type(std::shared_ptr<Onyx::AST::TypeExpr>);

    std::shared_ptr<_VarDecl> compile_var_decl(std::shared_ptr<Onyx::AST::Var>);

    std::shared_ptr<_FuncDecl>
        compile_func_decl(std::shared_ptr<Onyx::AST::Func>);

    std::shared_ptr<_Call> compile_call(std::shared_ptr<Onyx::AST::Call>);

    _RVal compile_rval(Onyx::AST::RVal);
    _RVal compile_rval(std::shared_ptr<Onyx::AST::ExplSafety>);

    /// TODO: May also return a value?
    void compile_expr(Onyx::AST::Expr);

    void compile_extern(std::shared_ptr<Onyx::AST::Extern>);
    void compile_c_ast(const C::AST *);

    /// Return a prefix to prepend to the name of a function contained in the
    /// scope.
    std::string function_prefix() const;

  protected:
    std::vector<std::shared_ptr<_Scope>> _children;
    std::vector<_Expr> _exprs;

    /// Variable declaration index for the current scope.
    /// NOTE: It is automatically updated upon an `_add_expr` call.
    std::map<std::string, std::shared_ptr<_VarDecl>> _var_decls;

    /// Function declaration index for the current scope.
    /// NOTE: It is automatically updated upon an `_add_expr` call.
    std::map<std::string, std::shared_ptr<_FuncDecl>> _func_decls;

    /// The map of C functions declared in this scope.
    std::map<std::string, std::shared_ptr<_CFuncDecl>> _c_func_decls;

    /// Infer a concrete type from an rval.
    /// TODO: May also infer a wider type for later narrowing.
    _ConcreteType _infer(_RVal *);

    /// Search for a variable declaration within self and parent scope(s).
    std::shared_ptr<_VarDecl> _search_var_decl(std::string id);

    /// Search for a function declaration within self and parent scope(s).
    std::shared_ptr<_FuncDecl> _search_func_decl(std::string id);

    /// Search for an existing C function declaration, or return nullptr.
    std::shared_ptr<_CFuncDecl> _search_c_func_decl(std::string id);

    /// Add a C function declaration. Would panic if already declared.
    void _add_c_func_decl(std::shared_ptr<const C::AST::FuncDecl>);

    /// Add an expression to the scope.
    /// Implicitly updates `_var_decls` and `_func_decls`.
    void _add_expr(_Expr);

    /// Add a child scope.
    template <typename T, typename... Args>
    std::shared_ptr<T> _create_child(Args... args) {
      auto ptr = std::make_shared<T>(T(args..., shared_from_this()));
      _children.push_back(ptr);
      return ptr;
    }
  };

  /// The top-level scope.
  struct _TopLevelScope : _Scope {
    _TopLevelScope() : _Scope("", Safety::Fragile, Storage::Static, nullptr) {}

    void write(std::ostream &) const;
    void lower(llvm::Module *) const;
  };

  /// A block of code, e.g. a function body or a condition branch.
  struct _Block : _Scope {
    using _Scope::_Scope;

    void write(std::ostream &, unsigned indent = 0) const;
    llvm::Value *lower(llvm::Module *, llvm::IRBuilder<> *) const;
  };

  Program *_program;

  const std::shared_ptr<_TopLevelScope> _top_level_scope =
      std::make_shared<_TopLevelScope>(this);

  static std::optional<_CBuiltInType> _search_c_built_in_type(std::string id);
  static void _write(_CBuiltInType, std::ostream &);

  /// Return `true` if *id* is a reserved C keyword or built-in type.
  static bool _is_c_reserved(std::string id);
};

} // namespace NXC
} // namespace Fancysoft
