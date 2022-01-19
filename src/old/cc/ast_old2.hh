#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "fancysoft/nxc/onyx/ast.hh"
#include "fancysoft/nxc/onyx/lang.hh"
#include "fancysoft/nxc/placement.hh"
#include "fancysoft/util/variant.hh"

#include "../c/ast.hh"
#include "./cst.hh"
#include "./mlir/builtin.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

namespace MLIR {

struct IntType;
struct Function;

} // namespace MLIR

namespace AST {

#pragma region Forward

struct Scope;

struct TraitSuperDecl;
struct TraitDecl;
struct TraitImpl;

struct BuiltinTypeSuperDecl;
struct BuiltinTypeDecl;
struct BuiltinTypeImpl;
struct BuiltinTypeSpec;

struct FunctionSuperDecl;
struct FunctionDecl;
struct FunctionImpl;

using AnySuperDecl = std::variant<
    std::shared_ptr<FunctionSuperDecl>,
    std::shared_ptr<BuiltinTypeSuperDecl>,
    std::shared_ptr<TraitSuperDecl>>;

using AnySuperDeclWeak = std::variant<
    std::weak_ptr<FunctionSuperDecl>,
    std::weak_ptr<BuiltinTypeSuperDecl>,
    std::weak_ptr<TraitSuperDecl>>;

using AnyTypeDeclWeak =
    std::variant<std::weak_ptr<BuiltinTypeDecl>, std::weak_ptr<TraitDecl>>;

using AnyImpl = std::variant<
    std::shared_ptr<BuiltinTypeImpl>,
    std::shared_ptr<FunctionImpl>,
    std::shared_ptr<TraitImpl>>;

using AnyTypeSpec = std::variant<std::shared_ptr<BuiltinTypeSpec>>;

struct VarDecl;
struct TemplateArgDecl;

using AnyDecl = std::variant<
    std::shared_ptr<FunctionDecl>,
    std::shared_ptr<BuiltinTypeDecl>,
    std::shared_ptr<TraitDecl>,
    std::shared_ptr<VarDecl>>;

using AnyDeclWeak = std::variant<
    std::weak_ptr<FunctionDecl>,
    std::weak_ptr<BuiltinTypeDecl>,
    std::weak_ptr<TraitDecl>,
    std::weak_ptr<VarDecl>>;

struct Id;
struct LiteralRestriction;

struct BoolLiteral;
struct IntLiteral;
struct CStringLiteral;

/// A basic literal may be used as a template argument value, e.g. `T<42>`.
using BasicLiteral =
    std::variant<std::unique_ptr<BoolLiteral>, std::unique_ptr<IntLiteral>>;

/// A (t)ype (val)ue can be passed as a template argument.
using TVal =
    Util::Variant::Flatten<std::variant<BasicLiteral, std::unique_ptr<Id>>>;

struct CStringLiteral;

/// A runtime literal may be used as a runtime value.
using RuntimeLiteral = Util::Variant::Flatten<
    std::variant<BasicLiteral, std::unique_ptr<CStringLiteral>>>;

struct Call;
struct If;
struct While;
struct RuntimeAssign;

using Expr = std::variant<
    std::unique_ptr<Block>,
    std::unique_ptr<Call>,
    std::unique_ptr<If>,
    std::unique_ptr<While>,
    std::unique_ptr<RuntimeAssign>>;

/// A (r)untime (val)ue may be used as a runtime argument or be assigned.
using RVal = std::variant<Id, RuntimeLiteral, Expr>;

struct Return;
using Statement = std::variant<std::unique_ptr<Return>>;

#pragma endregion

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

struct HasDecls {
  struct LookupResult {
    /// Would be `true` iff `Baz` is found in `Foo::Bar::Baz`.
    bool success;
    unsigned latest_found_index;
    std::optional<AnySuperDecl> decl;
  };

  LookupResult lookup(Lang::Id);

protected:
  std::vector<AnySuperDecl> _any_super_decls;
  std::vector<AnyDecl> _any_decls;
  std::unordered_map<std::string, AnySuperDeclWeak> _any_super_decls_index;
};

struct HasDocs {
  virtual std::string docs() const = 0;
};

template <typename ImplT> struct HasImpls {
  void add_impl(std::shared_ptr<ImplT>);

protected:
  std::vector<std::shared_ptr<ImplT>> _impls;
};

/// A full-path ID node.
struct Id : CSTMappable<CST::Id> {
  /// Compile an ID (e.g. `foo::bar.baz`), performing the declarations lookup.
  static std::shared_ptr<Id>
  compile(std::weak_ptr<Scope> parent_scope, std::shared_ptr<CST::Id> cst_node);

  struct Element {
    using DeclType = std::variant<
        std::shared_ptr<FunctionSuperDecl>,
        std::shared_ptr<BuiltinTypeSuperDecl>,
        std::shared_ptr<TraitSuperDecl>,
        std::shared_ptr<VarDecl>,
        std::shared_ptr<TemplateArgDecl>>;

    DeclType decl;
    CalleeArgTuple<TVal> template_args;
  };

  std::vector<Element> path;
};

/// A literal type restriction, e.g. `<Bitsize: \uint>`.
struct LiteralRestriction : CSTMappable<CST::LiteralRestriction> {
  using CSTMappable::CSTMappable;
  Lang::LiteralRestriction kind() const { return cst_node->kind(); }
};

/// Compiled from a single template argument declaration, e.g. `T: U = V`.
struct TemplateArgDecl : CSTMappable<CST::VarDecl>, HasDocs {
  /// Compile a template argument declaration node (e.g. `Bitsize: 32`).
  static std::shared_ptr<TemplateArgDecl> compile(
      std::weak_ptr<Scope> parent_scope,
      std::shared_ptr<CST::VarDecl> cst_node);

  std::variant<std::weak_ptr<FunctionDecl>, AnyTypeDeclWeak> parent;
  std::optional<std::variant<Id, LiteralRestriction>> type_restriction;

  TemplateArgDecl(
      std::shared_ptr<CST::VarDecl> cst_node,
      std::variant<std::weak_ptr<FunctionDecl>, AnyTypeDeclWeak> parent,
      std::optional<std::variant<Id, LiteralRestriction>> type_restriction) :
      CSTMappable(cst_node),
      parent(parent),
      type_restriction(type_restriction) {}

  std::string name() const { return cst_node->name.value; }
  std::string docs() const override { return cst_node->docs(); }
};

/// A declaration of zero or more template args, e.g. `<T, U>`.
struct MultiTemplateArgDecl {
  /// Check if this is compatible to other template args decl. Compatibility
  /// implies equal aliases, equal restrictions and non-conflicting default
  /// values. For example, `<T: U : V = W>` is compatible to `<T: X : V>`
  /// and also `<T : V = W>`.
  ///
  /// Returns the first pair of conflicting argument CST nodes, if any. The
  /// first node is in self, the second is the compared's. For example:
  ///
  /// Self     | Other    | Result
  /// ---      | ---      | ---
  /// `<T, U>` | `<T, V>` | `{U, V}`
  /// `<T>`    | `<>`     | `{T, null}`
  /// `<>`     | `<T>`    | `{null, T}`
  ///
  /// If both multidecls are empty, woul return `std::nullopt`, considering them
  /// equal.
  ///
  std::optional<std::pair<
      std::shared_ptr<const CST::VarDecl>,
      std::shared_ptr<const CST::VarDecl>>>
  compare(MultiTemplateArgDecl &) const;

  Placement placement() const;

protected:
  std::vector<TemplateArgDecl> _args;
  std::unordered_map<std::string, TemplateArgDecl *> _args_index;
};

#pragma region Scope

struct Scope : std::enable_shared_from_this<Scope>, HasDecls {
  /// The parent scope containing the current scope.
  ///
  /// It may be different for superdecls, decls and impls:
  ///
  /// ```nx
  /// namespace Foo {
  ///   decl type Bar;
  ///   decl bar(arg : Bar) # Here, `Bar` is `Foo::Bar`
  /// }
  ///
  /// decl type Bar;
  /// decl Foo::bar(arg : Bar) # Here, `Bar` is `::Bar`
  /// ```
  std::weak_ptr<Scope> parent_scope;

  Scope(std::weak_ptr<Scope> parent_scope) : parent_scope(parent_scope) {}

  /// Return the first scope in hierarchy which doesn't have a parent scope (may
  /// be self).
  std::weak_ptr<Scope> root() {
    auto parent = this->parent_scope.lock();

    if (parent)
      return parent->root();
    else
      return shared_from_this();
  }

  bool is_root() const { return !!this->parent_scope.lock(); }

  void compile(std::shared_ptr<const CST::FuncDecl>);
  void compile(std::shared_ptr<const CST::TypeDecl>, Logger *);
  void compile(std::shared_ptr<const CST::VarDecl>);
  void compile(std::shared_ptr<const CST::Alias>);
};

/// A scope with a name, such as a function declaration.
struct NamedScope : Scope {
  virtual std::string name() const;
  NamedScope(std::weak_ptr<Scope> parent_scope) : Scope(parent_scope) {}
};

/// An anonymous scope, e.g. a function body or the top level scope.
struct AnonScope : Scope {
  using Scope::Scope;
};

/// An anonymous code block.
struct Block : AnonScope {
  using AnonScope::AnonScope;

  void compile(std::shared_ptr<const CST::If>);
  void compile(std::shared_ptr<const CST::While>);
  void compile(std::shared_ptr<const CST::Call>);
  void compile(std::shared_ptr<const CST::Return>);
  void compile(std::shared_ptr<const CST::RuntimeAssign>);
  void compile(std::shared_ptr<const CST::ExplSafety>);
  void compile(std::shared_ptr<const CST::Block>);

protected:
  std::vector<Expr> _body;
};

/// The AST root, which is also the top level scope of the containing file.
struct Root : Block {
  Root(std::shared_ptr<Logger> logger) :
      _logger(logger),
      _c_ast(std::make_unique<C::AST>(_logger->fork("c-ast"))),
      Block(std::weak_ptr<Scope>()) {}

  void compile(std::shared_ptr<const CST::Extern>);
  void compile(std::shared_ptr<const CST::Import>);

private:
  std::weak_ptr<AnyDecl> _default_export;
  std::unordered_map<std::string, AnySuperDeclWeak> _exports;
  const std::shared_ptr<Logger> _logger;
  const std::unique_ptr<C::AST> _c_ast;
};

#pragma endregion

#pragma region SuperDecl

/// A super declaration doesn't exist in source code; it's updated continiously
/// during the compilation; it's the single source of truth for all of its
/// child declarations. A super declaration is guaranteed to always contain at
/// least one concrete declaration.
template <typename DeclT> struct SuperDecl : HasDecls {
  const std::weak_ptr<Scope> parent_scope;
  MultiTemplateArgDecl template_args;

  SuperDecl(std::weak_ptr<Scope> parent_scope, std::shared_ptr<DeclT> decl) :
      parent_scope(parent_scope) {
    _any_decls.push_back(decl);
  }

  std::weak_ptr<Scope> root() { return parent_scope.lock()->root(); }

  /// Return the name of the very first declaration (e.g. `String`).
  virtual std::string name() const {
    return std::visit(
        [](auto &decl) { return decl->name(); }, _any_decls.front());
  }

  /// Return the declaration kind, e.g. `"function"`.
  virtual std::string kind() const;

  /// Return the placement of the very first declaration.
  Placement initial_placement() const {
    return _any_decls.front().cst_node.placement();
  }

  void add_subdecl(std::shared_ptr<DeclT>);

protected:
  std::vector<std::shared_ptr<DeclT>> _subdecls;
};

/// A template struct containing an object type super declaration. Objects types
/// are some of the `builtin`, also `struct`, `class`, `enum` and `unit` types.
/// An object type superdecl contains implementations.
template <typename DeclT, typename ImplT>
struct ObjectTypeSuperDecl : SuperDecl<DeclT>, HasImpls<ImplT> {
  using SuperDecl<DeclT>::SuperDecl;
};

/// A trait type super declaration.
struct TraitSuperDecl : SuperDecl<TraitDecl> {
  using SuperDecl::SuperDecl;
  std::string kind() const override { return "trait"; }
};

/// A builtin type super declaration.
struct BuiltinTypeSuperDecl
    : ObjectTypeSuperDecl<BuiltinTypeDecl, BuiltinTypeImpl> {
  enum Type { Bool, Int };

  BuiltinTypeSuperDecl(
      std::weak_ptr<Scope> parent_scope,
      std::shared_ptr<BuiltinTypeDecl> decl,
      Type type) :
      ObjectTypeSuperDecl(parent_scope, decl) {}

  std::string kind() const override { return "builtin type"; }
};

/// A struct containing a function super declaration. A function super
/// declaration doesn't exist in source code; it's updated continiously
/// during the compilation; it contains all the function overload
/// declarations, and all the implementations.
struct FunctionSuperDecl : SuperDecl<FunctionDecl>, HasImpls<FunctionImpl> {
  using SuperDecl<FunctionDecl>::SuperDecl;
  std::string kind() const override { return "function"; }
};

#pragma endregion

#pragma region Decl

/// An abstract declaration applicable both to type and function declarations.
template <typename SuperDeclT, typename CSTNodeT>
struct Decl : NamedScope, CSTMappable<CSTNodeT>, HasDocs {
  std::weak_ptr<SuperDeclT> super_decl;
  MultiTemplateArgDecl template_args;

  Decl(
      std::shared_ptr<CSTNodeT> cst_node,
      std::weak_ptr<Scope> parent_scope,
      std::weak_ptr<SuperDeclT> super_decl) :
      CSTMappable<CSTNodeT>(cst_node), NamedScope(parent_scope) {}

  std::string name() const override { return this->cst_node->name(); }
  std::string docs() const override { return this->cst_node->docs(); }
};

/// A type declaration found in source code, applicable both to object and
/// trait types.
template <typename SuperDeclT>
struct TypeDecl : Decl<SuperDeclT, CST::TypeDecl> {};

/// Compiled from an trait type declaration contained in source code.
struct TraitDecl : TypeDecl<TraitSuperDecl> {
  /// Compile a `decl trait` node.
  static std::shared_ptr<BuiltinTypeDecl>
  compile(std::shared_ptr<CST::TypeDecl> cst_node);

  using TypeDecl::TypeDecl;
};

/// Compiled from a function declaration contained in source code.
struct FunctionDecl : Decl<FunctionSuperDecl, CST::FuncDecl> {
  /// Compile a `decl function` node.
  static std::shared_ptr<FunctionDecl> compile(
      std::weak_ptr<Scope> parent_scope,
      std::shared_ptr<CST::FuncDecl> cst_node);

  std::unordered_map<std::string, std::shared_ptr<VarDecl>> args;
};

/// Compiled from a builtin type declaration contained in source code.
struct BuiltinTypeDecl : TypeDecl<BuiltinTypeSuperDecl> {
  /// Compile a `def builtin` node. Effectively ignores variable declarations
  /// and function implementations.
  static std::shared_ptr<BuiltinTypeDecl>
  from_def(std::shared_ptr<const CST::TypeDecl> cst_node);

  using TypeDecl::TypeDecl;
};

/// Compiled from a single runtime variable declaration contained in source
/// code.
struct VarDecl : CSTMappable<CST::VarDecl>, HasDocs {
  /// Compile a runtime variable declaration node (`let` or `final` or arg).
  static std::shared_ptr<VarDecl> compile(
      std::weak_ptr<Scope> parent_scope,
      std::shared_ptr<CST::VarDecl> cst_node);

  std::weak_ptr<Scope> parent_scope;
  std::optional<Id> type_restriction;

  VarDecl(
      std::shared_ptr<CST::VarDecl> cst_node,
      std::weak_ptr<Scope> parent_scope,
      std::optional<Id> type_restriction) :
      CSTMappable(cst_node),
      parent_scope(parent_scope),
      type_restriction(type_restriction) {}

  std::string name() const { return cst_node->name.value; }
  std::string docs() const override { return cst_node->docs(); }
};

#pragma endregion

#pragma region Impl

/// An abstract implementation applicable both to type and function impls.
template <typename SuperDeclT, typename CSTNodeT>
struct Impl : NamedScope, CSTMappable<CSTNodeT>, HasDocs {
  std::weak_ptr<SuperDeclT> super_decl;

  Impl(
      std::shared_ptr<CSTNodeT> cst_node,
      std::weak_ptr<Scope> parent_scope,
      std::weak_ptr<SuperDeclT> super_decl) :
      CSTMappable<CSTNodeT>(cst_node), NamedScope(parent_scope) {}

  std::string name() const override { return this->cst_node->name(); }
  std::string docs() const override { return this->cst_node->docs(); }
};

/// A type (object or trait) implementation found in source code.
template <typename SuperDeclT>
struct TypeImpl : Impl<SuperDeclT, CST::TypeDecl> {};

/// Compiled from a trait type implementation contained in source code.
struct TraitImpl : TypeImpl<TraitSuperDecl> {
  /// Compile an `impl builtin (type)` node.
  static std::shared_ptr<TraitDecl>
  compile(std::shared_ptr<CST::TypeDecl> cst_node);

  using TypeImpl::TypeImpl;
};

/// An object type implementaiton containing specializations.
template <typename SuperDeclT, typename SpecT>
struct ObjectTypeImpl : TypeImpl<SuperDeclT> {
  using TypeImpl<SuperDeclT>::TypeImpl;

protected:
  std::vector<std::shared_ptr<SpecT>> _specs;
};

/// Compiled from a builtin type implementation contained in source code.
struct BuiltinTypeImpl : ObjectTypeImpl<BuiltinTypeSuperDecl, BuiltinTypeSpec> {
  /// Compile an impl from a `def builtin` node. Only includes variable
  /// declarations and function implementations. Implies `forall` for all
  /// template arguments declared in the `def` node.
  ///
  /// ```
  /// def Foo<T>;
  ///
  /// # Turns into:
  /// #
  ///
  /// decl Foo<T>;
  /// forall <T> impl Foo<T>;
  /// ```
  static std::shared_ptr<BuiltinTypeImpl>
      from_def(std::shared_ptr<const CST::TypeDecl>);

  using ObjectTypeImpl::ObjectTypeImpl;
};

struct FunctionImpl {};

#pragma endregion

#pragma region Spec

/// A builtin type specialization.
struct BuiltinTypeSpec {
  std::weak_ptr<BuiltinTypeImpl> impl;
  CalleeArgTuple<TVal> targs;
  MLIR::Builtin::Type::Any mlir_type;
};

/// A concrete variable specialization with specialized type.
struct VarSpec {
  std::weak_ptr<VarDecl> decl;
  AnyTypeSpec type;
  std::string name() const { return decl.lock()->name(); }
};

/// A function specializations originates from a function implementation. It
/// maps to either an MLIR user-defined or MLIR builtin function.
struct FunctionSpec {
  std::weak_ptr<FunctionImpl> impl;
  CalleeArgTuple<TVal> targs;
  CalleeArgTuple<VarSpec> args;
  std::variant<std::shared_ptr<MLIR::Function>, MLIR::Builtin::Function>
      mlir_spec;
};

#pragma endregion

#pragma region Literal

struct BoolLiteral : CSTMappable<CST::Bool> {
  using CSTMappable::CSTMappable;
  bool value() const { return cst_node->token.value; }
};

struct IntLiteral : CSTMappable<CST::Int> {
  using CSTMappable::CSTMappable;
  std::uint64_t value() const { return cst_node->token.value; }
};

struct CStringLiteral : CSTMappable<CST::CStringLiteral> {
  using CSTMappable::CSTMappable;
  std::string value() const { return cst_node->token.value; }
};

#pragma endregion

#pragma region Exprs

struct Call {
  /// The narrow-est matching declaration.
  std::weak_ptr<FunctionDecl> callee;

  CalleeArgTuple<TVal> targs;
  CalleeArgTuple<RVal> args;

  /// Resolve the callee specialization.
  std::shared_ptr<FunctionSpec> resolve_callee() {
    if (_callee_spec)
      return _callee_spec;

    // Resolve code
  }

private:
  std::shared_ptr<FunctionSpec> _callee_spec;
};

struct Case : CSTMappable<CST::Case> {
  RVal cond;
  Block branch;
};

struct If : CSTMappable<CST::If> {
  Case main;
  std::vector<Case> elifs;
  std::optional<Block> else_branch;
};

struct While : CSTMappable<CST::While> {
  RVal cond;
  Block body;
};

struct Return : CSTMappable<CST::Return> {
  std::optional<RVal> value;
};

struct RuntimeAssign : CSTMappable<CST::RuntimeAssign> {
  std::shared_ptr<VarDecl> variable;
  RVal value;
};

#pragma endregion

} // namespace AST

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
