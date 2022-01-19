#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../logger.hh"
#include "./cst.hh"
#include "fancysoft/nxc/onyx/ast.hh"
#include "fancysoft/nxc/onyx/lang.hh"
#include "fancysoft/nxc/onyx/token.hh"
#include "fancysoft/nxc/placement.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {
namespace AST {

struct Id;
struct VarDecl;
struct FuncSuperDecl;
struct BuiltinTypeSuperDecl;
struct TraitSuperDecl;

#pragma region Util

/// A template type which may be directly mapped to a CST node.
template <typename NodeT> struct CSTMappable {
  const std::shared_ptr<const NodeT> cst_node;
  CSTMappable(std::shared_ptr<const NodeT> cst_node) : cst_node(cst_node) {}
};

/// A tuple of args passed to a callee, e.g. `(arg: val)` or `<T: U>`.
template <typename T> struct CalleeArgTuple {
  std::vector<T> vargs;
  std::map<std::string, T> kwargs;
};

#pragma endregion

#pragma region Literal

template <typename CSTNodeT, typename ValueT>
struct LiteralBase : CSTMappable<CSTNodeT> {
  ValueT value() const { return this->cst_node->token.value; }
};

struct BoolLiteral : LiteralBase<CST::Bool, bool> {
  using LiteralBase::LiteralBase;
};

struct IntLiteral : LiteralBase<CST::Int, std::uint64_t> {
  using LiteralBase::LiteralBase;
};

using Literal = std::variant<BoolLiteral, IntLiteral>;

#pragma endregion

#pragma region Type

struct FuncSuperDeclRef;
struct BuiltinTypeSuperDeclRef;
struct TraitSuperDeclRef;

using SuperDeclRef = std::variant<
    std::unique_ptr<FuncSuperDeclRef>,
    std::unique_ptr<BuiltinTypeSuperDeclRef>,
    std::unique_ptr<TraitSuperDeclRef>>;

struct SuperDeclRefUnOp;
struct SuperDeclRefBinOp;

/// A superdeclaration reference expression, e.g. `Foo | Bar`.
using SuperDeclRefExpr = std::variant<
    std::unique_ptr<SuperDeclRefUnOp>,
    std::unique_ptr<SuperDeclRefBinOp>,
    std::unique_ptr<SuperDeclRef>>;

/// A superdeclaration reference unary operation, e.g. `~Foo`.
struct SuperDeclRefUnOp {
  enum class Op { Not, SelfAnd };
  Op op;
  SuperDeclRefExpr operand;
};

/// A superdeclaration reference binary operation, e.g. `Foo | Bar`.
struct SuperDeclRefBinOp {
  enum class Op { And, Or, Xor };
  Op op;
  SuperDeclRefExpr left;
  SuperDeclRefExpr right;
};

struct TemplateArgDecl;

/// A (t)ype (val)ue which can be used as a template argument when (ref)erencing
/// a (decl)aration. It can be either a super declaration reference, a declared
/// template argument reference, or a literal.
///
/// ```nx
/// struct Foo<T> {
///   impl Bar<T, 42, Foo<T>>::baz;
/// #          │  │   └─ A superdeclaration reference
/// #          │  └─ A literal
/// #          └─ A declared arg reference
/// }
using DeclRefTVal =
    std::variant<std::shared_ptr<TemplateArgDecl>, SuperDeclRef, Literal>;

#pragma endregion

/// A full-path superdeclaration identifier, e.g. `&(Foo & Bar<T>)::Baz<U>.qux`.
/// Template arguments are passed, not declared; it doesn't trigger
/// specialization. In some sense, the template arguments are ignored, e.g.
/// `T<8>` and `T<16>` would reference the same superdeclaration `T`.
struct SuperDeclID : CSTMappable<CST::Id> {
  struct Element {
    const Lang::Id::Element::Access access;
    const SuperDeclRefExpr expr;

    Element(Lang::Id::Element::Access access, SuperDeclRefExpr expr) :
        access(access), expr(move(expr)) {}
  };

  std::vector<Element> elements;

  SuperDeclID(
      std::shared_ptr<const CST::Id> cst_node, std::vector<Element> elements) :
      CSTMappable(cst_node), elements(elements) {}
};

template <typename DeclT> struct MultiArgDecl {
  std::vector<std::shared_ptr<DeclT>> args;
  std::shared_ptr<DeclT> compare(MultiArgDecl *) const;
};

/// A single runtime variable declaration.
struct VarDecl : CSTMappable<CST::VarDecl> {
  /// Would be null for a freestanding var decl.
  std::weak_ptr<MultiArgDecl<VarDecl>> parent;

  std::string name_or_alias() const { return cst_node->alias_or_name(); }
  std::string name() const { return cst_node->name.value; };
};

/// A literal type restriction, e.g. `\uint` in `<Bitsize: \uint>`.
struct LiteralRestriction : CSTMappable<CST::LiteralRestriction> {
  using CSTMappable::CSTMappable;
  Lang::LiteralRestriction kind() const { return cst_node->kind(); }
};

/// A single template argument declaration.
struct TemplateArgDecl : CSTMappable<CST::VarDecl> {
  std::weak_ptr<MultiArgDecl<TemplateArgDecl>> parent;
  std::variant<std::shared_ptr<Id>, LiteralRestriction> restriction;
  std::optional<DeclRefTVal> default_value;
  std::string name_or_alias() const { return cst_node->alias_or_name(); }
  std::string name() const { return cst_node->name.value; };
};

// SuperDeclRef create_superdecl_ref(
//     std::shared_ptr<const CST::Id::Element>,
//     AnySuperDecl,
//     std::optional<CalleeArgTuple<DeclRefTVal>> = std::nullopt);

struct Root;
struct BuiltinTypeDecl;
struct TraitDecl;
struct FuncDecl;

struct Scope {
  using AnyDecl = std::variant<
      std::shared_ptr<BuiltinTypeDecl>,
      std::shared_ptr<TraitDecl>,
      std::shared_ptr<VarDecl>,
      std::shared_ptr<FuncDecl>>;

  using AnySuperDecl = std::variant<
      std::shared_ptr<FuncSuperDecl>,
      std::shared_ptr<BuiltinTypeSuperDecl>,
      std::shared_ptr<TraitSuperDecl>>;

  /// ```
  /// Panic! Already declared Foo as struct, not builtin (P0001)
  ///
  ///   def builtin Foo
  ///       ^
  /// Previously declared here
  ///
  ///   decl struct Foo
  ///        ^
  /// ```
  struct P0001 : Panic {
    static P0001 construct(
        std::shared_ptr<const Onyx::CST::TypeDecl> type_decl,
        AnySuperDecl superdecl);

  private:
    P0001(
        std::shared_ptr<CST::Id> id,
        Token::Keyword decl_kind_keyword,
        Token::Keyword previous_decl_kind_keyword) :
        Panic(
            fmt::format(
                "Already declared {0} as {1}, not builtin (P0001)",
                id->Node::print(),
                decl_kind_keyword.Token::print()),
            decl_kind_keyword.placement,
            {{"Previously declared here",
              previous_decl_kind_keyword.placement}}) {}
  };

  struct LookupResult {
    enum Status { Success, Undeclared };
    Status status;
    AnySuperDecl value;
  };

  std::weak_ptr<Scope> parent;

  /// Return a pointer to the root node of the containing AST.
  /// Shall be overriden in `Root`.
  virtual std::shared_ptr<Root> root() {
    auto parent = this->parent.lock();

    if (parent)
      return parent->root();
    else
      throw "BUG: Orphan scope";
  }

  virtual bool is_root() const { return false; }

  virtual std::optional<AnySuperDecl>
      lookup_superdecl(std::shared_ptr<const CST::Id>) const;

  /// NOTE: Doesn't save the superdecl yet.
  template <typename SuperDeclT>
  std::shared_ptr<SuperDeclT> find_or_create_superdecl(
      std::shared_ptr<const CST::TypeDecl> cst_node) const {
    auto lookup_result = lookup_superdecl(cst_node->id);
    std::shared_ptr<SuperDeclT> superdecl;

    if (lookup_result.has_value()) {
      superdecl =
          *std::get_if<std::shared_ptr<SuperDeclT>>(&lookup_result.value());

      if (!superdecl)
        // "Already declared Foo as struct, not builtin".
        throw P0001::construct(cst_node, lookup_result.value());
    } else {
      // Create a new superdecl if not exists.
      superdecl = std::make_shared<SuperDeclT>();
    }
  }
};

inline std::string kind(FuncSuperDecl *) { return "function"; }
inline std::string kind(BuiltinTypeSuperDecl *) { return "builtin"; }
inline std::string kind(TraitSuperDecl *) { return "trait"; }

template <typename DeclT, typename ImplT> struct SuperDecl {
  /// NOTE: A superdecl is guaranteed to always parent at least one subdecl.
  std::vector<std::shared_ptr<DeclT>> subdecls;

  /// Concerete implementations of this superdeclaration.
  std::vector<std::shared_ptr<ImplT>> impls;

  /// Return the first subdecl's name.
  std::string name() const;

  /// Return the kind token, e.g. `struct`, taken from the first subdecl.
  virtual Token::Keyword kind_token() const;

  /// Template arguments shall be compatible for all subdecls. The method merely
  /// returns the first subdecl's template args for further compatibility check.
  std::optional<MultiArgDecl<TemplateArgDecl>> targs() const;
};

/// Definition of a function or a type. Is not an AST node, but a comprehensive
/// container for a declaration and an implementation compiled from a single
/// CST `def` node.
template <
    typename CSTNodeT,
    typename SuperDeclT,
    typename DeclT,
    typename ImplT>
struct Def {
  /// Compile a `def` CST node.
  static Def compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<const CSTNodeT> cst_node,
      Logger *logger) {
    auto superdecl =
        current_scope->find_or_create_superdecl<SuperDeclT>(cst_node);

    auto decl = DeclT::compile(current_scope, superdecl, cst_node, logger);
    auto impl = ImplT::compile(current_scope, superdecl, cst_node, logger);

    superdecl->subdecls.push_back(decl);
    superdecl->impls.push_back(impl);

    return TypeDef(decl, impl);
  }

  std::shared_ptr<DeclT> decl;
  std::shared_ptr<ImplT> impl;

  Def(std::shared_ptr<DeclT> decl, std::shared_ptr<ImplT> impl) :
      decl(decl), impl(impl) {}
};

#pragma region Func

struct FuncSuperDecl;
struct FuncSuperDeclRef;
struct FuncImpl;

/// A function superdeclaration parents all the concrete function declarations
/// found in source code. All the subdecls' template arguments must be
/// compatible with the superdecl's. That said, the very first subdecl is
/// considered the "source" one: its name and template args are honored.
struct FuncSuperDecl : SuperDecl<FuncDecl, FuncImpl> {
  /// A function may be declared either in the root scope or within some type.
  using ParentT = std::variant<
      std::weak_ptr<Root>,
      std::weak_ptr<BuiltinTypeSuperDecl>,
      std::weak_ptr<TraitSuperDecl>>;

  const ParentT parent;

  FuncSuperDecl(ParentT parent) : parent(parent) {}

  Token::Keyword kind_token() const override;
};

/// A freestanding function declaration residing in source code. A function
/// declaration allows runtime argument and return type overloading, but the
/// template arguments declaration must be compatible with the superdecl's. A
/// function declaration doesn't have a body.
struct FuncDecl : CSTMappable<CST::FuncDecl> {
  const std::weak_ptr<FuncSuperDecl> super_decl;

  /// Template args declaration for this function declaration. These must be
  /// compatible with the superdecl's.
  std::optional<MultiArgDecl<TemplateArgDecl>> targs;

  /// Storage can be overloaded.
  Lang::Storage storage;

  /// Runtime args declaration can be overloaded.
  MultiArgDecl<VarDecl> ragrs;

  /// NOTE: A missing return type implies an inferred return type.
  std::optional<SuperDeclRefExpr> return_type;

  std::string name() const { return cst_node->name(); }

  /// Compile a function declaration from either a `def` or a `decl` CST node.
  static std::shared_ptr<FuncDecl> compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<FuncSuperDecl> superdecl,
      std::shared_ptr<const CST::FuncDecl> cst_node,
      Logger *logger);
};

/// A function superdeclaration reference with optional template arguments, e.g.
/// `foo<T>`.
struct FuncSuperDeclRef : CSTMappable<CST::Id::Element> {
  std::weak_ptr<FuncSuperDecl> superdecl;

  /// Passed to the superdecl.
  std::optional<CalleeArgTuple<DeclRefTVal>> tvars;

  FuncSuperDeclRef(
      std::shared_ptr<const CST::Id::Element> cst_node,
      std::weak_ptr<FuncSuperDecl> superdecl,
      std::optional<CalleeArgTuple<DeclRefTVal>> tvars) :
      CSTMappable(cst_node), superdecl(superdecl), tvars(tvars) {}
};

/// A function implementation is found in source code; it implements a
/// previously declared declaration.
struct FuncImpl : CSTMappable<CST::FuncDecl> {
  FuncSuperDeclRef decl_ref;

  std::optional<MultiArgDecl<TemplateArgDecl>> forall;

  /// Runtime args declaration for this implementation.
  MultiArgDecl<VarDecl> ragrs;

  /// Return type declaration for this implementation.
  std::optional<SuperDeclRefExpr> return_type;

  /// Compile a function implementation from either a `def` or an `impl` CST
  /// node.
  static std::shared_ptr<FuncImpl> compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<FuncSuperDecl> superdecl,
      std::shared_ptr<const CST::FuncDecl> cst_node,
      Logger *logger);
};

struct FuncDef : Def<CST::FuncDecl, FuncSuperDecl, FuncDecl, FuncImpl> {};

#pragma endregion

#pragma region TypeDecl

template <typename DeclT, typename ImplT>
struct TypeSuperDecl : SuperDecl<DeclT, ImplT> {
  /// A type may only be declared within the root scope.
  using ParentT = std::variant<std::weak_ptr<Root>>;

  const ParentT parent;

  TypeSuperDecl(ParentT parent) : parent(parent) {}
  Token::Keyword kind_token() const override;
};

template <typename SuperDeclT> struct TypeDecl : CSTMappable<CST::TypeDecl> {
  std::weak_ptr<SuperDeclT> super_decl;

  /// Must be compatible with the superdecl.
  MultiArgDecl<TemplateArgDecl> template_args_decl;
};

template <typename SuperDeclT>
struct TypeSuperDeclRef : CSTMappable<CST::Id::Element> {
  std::weak_ptr<SuperDeclT> superdecl;
  std::optional<CalleeArgTuple<DeclRefTVal>> tvars;

  TypeSuperDeclRef(
      std::shared_ptr<const CST::Id::Element> cst_node,
      std::weak_ptr<SuperDeclT> superdecl,
      std::optional<CalleeArgTuple<DeclRefTVal>> tvars) :
      CSTMappable(cst_node), superdecl(superdecl), tvars(tvars) {}
};

struct HasInstanceFuncDecls {
  std::vector<std::shared_ptr<FuncDecl>> instance_func_decls;
};

struct HasInstanceFuncImpls {
  std::vector<std::shared_ptr<FuncImpl>> instance_func_impls;
};

struct HasInstanceVarDecls {
  std::vector<std::shared_ptr<VarDecl>> instance_var_decls;
};

template <typename SuperDeclT> struct TypeImpl : CSTMappable<CST::TypeDecl> {
  TypeSuperDeclRef<SuperDeclT> super_decl_ref;

  /// All the static var declarations accumulated from impls.
  std::vector<std::shared_ptr<VarDecl>> static_var_decls;

  /// Static functions declared in this concrete object declaration. These are
  /// strongly-referenced by the function superdecl contained in *super_decl*.
  std::vector<std::weak_ptr<FuncDecl>> static_func_decls;

  /// Static functions declared in this concrete object declaration. These are
  /// strongly-referenced by the function superdecl contained in *super_decl*.
  std::vector<std::weak_ptr<FuncImpl>> static_func_impls;

  /// A `forall` modifier declares a list of template arguments to match.
  MultiArgDecl<TemplateArgDecl> forall;
};

#pragma endregion

#pragma region BuiltinType

struct BuiltinTypeDecl;
struct BuiltinTypeImpl;

struct BuiltinTypeSuperDecl : TypeSuperDecl<BuiltinTypeDecl, BuiltinTypeImpl> {

};

struct BuiltinTypeDecl : TypeDecl<BuiltinTypeSuperDecl> {
  /// Compile a builtin type declaration from either a `def` or a `decl` CST
  /// node.
  static std::shared_ptr<BuiltinTypeDecl> compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<BuiltinTypeSuperDecl> superdecl,
      std::shared_ptr<const CST::TypeDecl> cst_node,
      Logger *logger);
};

struct BuiltinTypeSuperDeclRef : TypeSuperDeclRef<BuiltinTypeSuperDecl> {
  using TypeSuperDeclRef::TypeSuperDeclRef;
};

struct BuiltinTypeImpl : TypeImpl<BuiltinTypeSuperDecl>,
                         HasInstanceFuncDecls,
                         HasInstanceFuncImpls {
  std::vector<std::variant<
      std::unique_ptr<TraitSuperDeclRef>,
      std::unique_ptr<BuiltinTypeSuperDeclRef>>>
      ancestors;

  /// Compile a builtin type implementation from either a `def` or an `impl` CST
  /// node.
  static std::shared_ptr<BuiltinTypeImpl> compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<BuiltinTypeSuperDecl> superdecl,
      std::shared_ptr<const CST::TypeDecl> cst_node,
      Logger *logger);
};

struct BuiltinTypeDef : Def<CST::TypeDecl,
                            BuiltinTypeSuperDecl,
                            BuiltinTypeDecl,
                            BuiltinTypeImpl> {};

#pragma endregion

#pragma region Struct

// struct StructDecl;
// struct StructImpl;

// struct StructSuperDecl : TypeSuperDecl<StructDecl, StructImpl> {};
// struct StructDecl : TypeDecl<StructSuperDecl> {};

// struct StructImpl : TypeImpl<StructSuperDecl>,
//                     HasInstanceFuncDecls,
//                     HasInstanceFuncImpls,
//                     HasInstanceVarDecls {};

#pragma endregion

#pragma region Class

// struct ClassDecl;
// struct ClassImpl;

// struct ClassSuperDecl : TypeSuperDecl<ClassDecl, ClassImpl> {};
// struct ClassDecl : TypeDecl<ClassSuperDecl> {};

// struct ClassImpl : TypeImpl<ClassSuperDecl>,
//                    HasInstanceFuncDecls,
//                    HasInstanceFuncImpls,
//                    HasInstanceVarDecls {};

#pragma endregion

#pragma region Trait

struct TraitImpl;

struct TraitSuperDecl : TypeSuperDecl<TraitDecl, TraitImpl> {
  /// A trait can be extended by any type.
  std::vector<
      std::variant<std::weak_ptr<BuiltinTypeImpl>, std::weak_ptr<TraitImpl>>>
      descendants;
};

struct TraitDecl : TypeDecl<TraitSuperDecl> {
  /// Compile a trait declaration from either a `def` or a `decl` CST node.
  static std::shared_ptr<TraitDecl> compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<TraitSuperDecl> superdecl,
      std::shared_ptr<const CST::TypeDecl> cst_node,
      Logger *logger);
};

struct TraitSuperDeclRef : TypeSuperDeclRef<TraitSuperDecl> {
  using TypeSuperDeclRef::TypeSuperDeclRef;
};

struct TraitImpl : TypeImpl<TraitSuperDecl>,
                   HasInstanceFuncDecls,
                   HasInstanceFuncImpls {
  /// Compile a trait implementation from either a `def` or an `impl` CST node.
  static std::shared_ptr<TraitImpl> compile(
      std::shared_ptr<Scope> current_scope,
      std::shared_ptr<TraitSuperDecl> superdecl,
      std::shared_ptr<const CST::TypeDecl> cst_node,
      Logger *logger);
};

struct TraitDef : Def<CST::TypeDecl, TraitSuperDecl, TraitDecl, TraitImpl> {};

#pragma endregion

#pragma region Enum

// struct EnumDecl;
// struct EnumImpl;

// struct EnumSuperDecl : TypeSuperDecl<EnumDecl, EnumImpl> {};
// struct EnumDecl : TypeDecl<EnumSuperDecl> {};

// struct EnumVal : CSTMappable<CST::EnumVal> {};

// struct EnumImpl : TypeImpl<EnumSuperDecl>,
//                   HasInstanceFuncDecls,
//                   HasInstanceFuncImpls {
//   std::vector<EnumVal> enum_vals;
// };

#pragma endregion

#pragma region Unit

// struct UnitDecl;
// struct UnitImpl;

// struct UnitSuperDecl : TypeSuperDecl<UnitDecl, UnitImpl> {};
// struct UnitDecl : TypeDecl<UnitSuperDecl> {};
// struct UnitImpl : TypeImpl<UnitSuperDecl> {};

#pragma endregion

struct Root : Scope, std::enable_shared_from_this<Root> {
  using Export = std::variant<
      std::weak_ptr<BuiltinTypeSuperDecl>,
      std::weak_ptr<TraitSuperDecl>,
      // std::weak_ptr<StructSuperDecl>,
      // std::weak_ptr<ClassSuperDecl>,
      // std::weak_ptr<EnumSuperDecl>,
      // std::weak_ptr<UnitSuperDecl>,
      std::weak_ptr<VarDecl>,
      std::weak_ptr<FuncSuperDecl>>;

  /// NOTE: An imported feature may also be exported, e.g. `export import Foo
  /// from "./foo.nx"`.
  ///
  /// NOTE: A declaration export implies its super declaration export:
  ///
  /// ```nx
  /// decl foo(a : T)
  /// export decl foo(a : U) # Would export both overloads
  /// export foo             # Ditto
  /// ```
  ///
  std::unordered_map<std::string, Export> exports;

  std::optional<Export> default_export;

  using Import = std::variant<
      std::weak_ptr<BuiltinTypeSuperDecl>,
      std::weak_ptr<TraitSuperDecl>,
      // std::weak_ptr<StructSuperDecl>,
      // std::weak_ptr<ClassSuperDecl>,
      // std::weak_ptr<EnumSuperDecl>,
      // std::weak_ptr<UnitSuperDecl>,
      std::weak_ptr<VarDecl>,
      std::weak_ptr<FuncSuperDecl>,
      std::weak_ptr<Root> // E.g. `import * as Foo from "path"`
      >;

  std::unordered_map<std::string, Import> imports;

  std::vector<AnyDecl> decls;
  std::unordered_map<std::string, AnySuperDecl> super_decls;

  /// Pointers to the `decls` vector elements.
  std::unordered_map<std::string, std::weak_ptr<VarDecl>> var_decl_index;

  std::shared_ptr<Root> root() override { return shared_from_this(); }
  bool is_root() const override { return true; }

  void compile(std::shared_ptr<const CST::TypeDecl>, Logger *);

  /// Would panic if failed to lookup mid-way.
  std::optional<AnySuperDecl>
      lookup_superdecl(std::shared_ptr<const CST::Id>) const override;
};

} // namespace AST
} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
