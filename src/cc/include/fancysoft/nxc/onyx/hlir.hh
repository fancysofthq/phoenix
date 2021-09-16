#pragma once

#include <assert.h>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../../util/map.hh"
#include "../../util/variant.hh"
#include "../c/hlir.hh"
#include "../safety.hh"
#include "../storage.hh"
#include "./ast.hh"

namespace Fancysoft {
namespace NXC {

struct Program;

namespace Onyx {
struct HLIR {
  struct Scope;

  struct IntLiteral;
  struct StringLiteral;

  struct CStringLiteral;

  using Literal = std::variant<
      std::unique_ptr<IntLiteral>,
      std::unique_ptr<StringLiteral>,
      std::unique_ptr<CStringLiteral>>;

  enum BuiltInType {
    String,
  };

  // struct TypeSpez;
  struct VarDecl;

  using TypeRestriction = std::variant<C::HLIR::TypeRef
                                       // std::shared_ptr<TypeSpez>
                                       >;

  // struct FuncSpez;
  struct Call;
  struct CCall;

  struct Assignment;
  struct PointerOf;

  /// May be used in an (implicit) body function.
  /// Note that `VarDecl` is not an expression.
  /// Also note that a scope itself may be an expression.
  using Expr = std::variant<
      std::shared_ptr<Scope>,
      std::shared_ptr<CCall>,
      std::shared_ptr<Assignment>>;

  /// May be used as an inline expression, e.g. `if cond() then "foo"`.
  ///
  /// NOTE: An `Assignment` shall be wrapped, e.g. `then a = b` is illegal.
  /// Therefore, an assignment is not in the single expression variant.
  using RVal = Util::Variant::flatten_t<std::variant<
      Literal,
      std::shared_ptr<VarDecl>,
      Expr,
      std::unique_ptr<PointerOf>>>;

  struct IntLiteral {
    const int value;
    IntLiteral(int value) : value(value) {}
    void write(std::ostream &) const;
  };

  struct StringLiteral {
    const std::string value;
    StringLiteral(std::string value) : value(value) {}
    void write(std::ostream &) const;
  };

  struct CStringLiteral {
    const std::string value;
    CStringLiteral(std::string value) : value(value) {}
    void write(std::ostream &) const;
  };

  // /// A type skeleton to act as base for a type specialization.
  // struct TypeSkeleton {
  //   const BuiltInType base;
  //   std::map<std::string, Literal> generic_args;

  //   // TypeSkeleton(TypeSkeleton &) = delete;
  //   // TypeSkeleton(TypeSkeleton &&) = delete;

  //   TypeSkeleton(
  //       BuiltInType base, std::map<std::string, Literal> generic_args) :
  //       base(base), generic_args(move(generic_args)) {}
  // };

  // /// A concrete resolved Onyx type specialization. A type specialization
  // /// resides in the module where it has been triggered for the first time.
  // struct TypeSpez {
  //   const HLIR *containing_hlir;
  //   const TypeSkeleton skeleton;

  //   TypeSpez(const HLIR *containing_hlir, TypeSkeleton skeleton) :
  //       containing_hlir(containing_hlir), skeleton(std::move(skeleton)) {}

  //   static std::shared_ptr<TypeSpez> resolve(Program *, StringLiteral);
  // };

  // struct FuncSpez {
  //   const FuncDecl base;
  //   const std::shared_ptr<TypeSpez> resolved_containing_type;
  //   const std::map<std::string, Literal> resolved_generic_args;
  //   const Storage storage;
  // };

  /// An Onyx variable declaration.
  struct VarDecl {
    std::string id;
    TypeRestriction type;

    VarDecl(std::string id, TypeRestriction type) : id(id), type(type) {}

    void write(std::ostream &) const;
  };

  struct PointerOf {
    // PLAN: Can also take pointer of expressions.
    std::shared_ptr<VarDecl> value;
    PointerOf(std::shared_ptr<VarDecl> value) : value(value) {}
    void write(std::ostream &) const;
  };

  struct CCall {
    const std::shared_ptr<C::HLIR::FuncDecl> callee;
    const std::vector<std::unique_ptr<PointerOf>> args;

    CCall(
        std::shared_ptr<C::HLIR::FuncDecl> callee,
        std::vector<std::unique_ptr<PointerOf>> args) :
        callee(callee), args(move(args)) {}

    void write(std::ostream &) const;
  };

  struct Assignment {
    const std::shared_ptr<VarDecl> lvalue;
    const RVal rvalue;

    Assignment(std::shared_ptr<VarDecl> lvalue, RVal rvalue) :
        lvalue(lvalue), rvalue(move(rvalue)) {}

    void write(std::ostream &) const;
  };

  struct Scope : std::enable_shared_from_this<Scope> {
    const std::optional<Safety> safety;
    const std::optional<Storage> storage;

    const std::shared_ptr<Scope> parent;
    HLIR *hlir;

    Scope(
        HLIR *hlir,
        std::optional<Safety> safety,
        std::optional<Storage> storage,
        std::shared_ptr<Scope> parent) :
        hlir(hlir), safety(safety), storage(storage), parent(parent) {}

    std::shared_ptr<Scope>
    create_child(std::optional<Safety> safety, std::optional<Storage> storage) {
      auto ptr = std::make_shared<Scope>(
          Scope(hlir, safety, storage, shared_from_this()));
      _children.push_back(ptr);
      return ptr;
    }

    std::shared_ptr<CCall> compile(std::shared_ptr<Onyx::AST::CCall>);
    std::shared_ptr<VarDecl> compile(std::shared_ptr<Onyx::AST::VarDecl>);
    RVal compile(Onyx::AST::RVal);
    RVal compile(std::shared_ptr<Onyx::AST::ExplicitSafetyStatement>);

    void write(std::ostream &) const;

  private:
    std::vector<std::shared_ptr<Scope>> _children;
    std::vector<Expr> _exprs;
    std::map<std::string, std::shared_ptr<VarDecl>> _var_decls;

    TypeRestriction _infer(RVal *);

    void _add_var_decl(std::shared_ptr<VarDecl> decl) {
      _var_decls[decl->id] = decl;
    }

    std::shared_ptr<VarDecl> _search_var_decl(std::string id) {
      if (auto ptr = *Util::Map::get_if(_var_decls, id))
        return ptr;
      else if (parent)
        return parent->_search_var_decl(id);
      else
        return nullptr;
    }

    void _add_expr(Expr expr) { _exprs.push_back(move(expr)); }
  };

  HLIR(Program *, const Onyx::AST *);

  /// Write the HLIR in human-readable format into an *output*.
  void write(std::ostream &) const;

protected:
  const std::shared_ptr<Scope> _implicit_main_scope = std::make_shared<Scope>(
      this, Safety::Threadsafe, Storage::Static, nullptr);

  /// An Onyx HLIR contains a C HLIR for declarations
  /// created by `extern` directives.
  std::unique_ptr<C::HLIR> _self_c_hlir =
      std::make_unique<C::HLIR>(C::HLIR(nullptr));

  std::shared_ptr<C::HLIR::FuncDecl> _find_func_decl(std::string id) {
    // TODO: Also search within includes.
    return _self_c_hlir->find_func_decl(id);
  }

  /// TODO: C HLIRs created with `#include` directives, shared across the
  /// program to avoid re-compilation.
  // std::vector<std::shared_ptr<C::HLIR>> _included_c_hlirs;

private:
  void _compile_extern_directive(
      Program *, std::shared_ptr<Onyx::AST::ExternDirective>);
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
