#pragma once

#include <compare>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <fancysoft/util/map.hh>
#include <fancysoft/util/variant.hh>

#include "../c/ast.hh"
#include "../exception.hh"
#include "../logger.hh"
#include "./cst.hh"
#include "./lang.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

struct AST {
  /// A scope provides the `lookup` method.
  struct Scope;

  /// A polymorphic C++ struct containing lookup-able super declarations.
  struct Namespace;

  /// A code block implementation existing in source code. It doesn't require
  /// underlying entities to be specialized.
  struct Block;

  /// The implicit top level code block implementation.
  struct TopLevel;

  /// Anything with implementations.
  template <typename ImplT> struct HasImpls;

  /// Anything with specializations.
  template <typename SpecT> struct HasSpecs;

  /// A template C++ struct acting as a base for all Onyx super declarations.
  ///
  /// A super declaration is abstract, i.e. it doesn't exist in source code.
  /// Instead, it acts as a parent for concrete, source-code declarations. A
  /// declaration itself may reside in a namespace other than the superdecl's.
  ///
  ///
  /// ```nx
  /// namespace Foo
  ///   decl struct Bar;
  ///   decl struct Baz;
  /// end
  ///
  /// struct Qux;
  ///
  /// # A concrete declaration residing in the top level, not in `Foo`.
  /// # However, its superdecl is still `Foo::Bar`.
  /// decl Foo::Bar {
  ///   # Baz    # Panic! Undeclared in current scope
  ///   Foo::Baz # OK
  ///   Qux      # OK
  /// }
  /// ```
  template <typename DeclT> struct SuperDecl;

  /// Mapped to a concrete declaration existing in source code.
  template <typename SuperDeclT> struct Decl;

  /// An implementation existing in source code.
  template <typename SuperDeclT> struct Impl;

  /// A native Onyx type super declaration.
  struct NativeTypeSuperDecl;

  /// A native Onyx type declaration.
  struct NativeTypeDecl;

  /// A native Onyx type implementation, i.e. the list of functions etc.
  struct NativeTypeImpl;

  /// An `interface` super declaration.
  struct InterfaceSuperDecl;

  /// An `interface` declaration.
  struct InterfaceDecl;

  /// A function super declaration, which only shares the name.
  struct FuncSuperDecl;

  /// A function declaration.
  struct FuncDecl;

  /// A function implementation.
  struct FuncImpl;

  /// A single runtime variable declaration found in declarations. Note that
  /// there are no "superdecls" of variables.
  struct VarDecl;

  /// A declared runtime variable implementation found in implementations.
  struct VarImpl;

  /// A single template argument declaration.
  struct TemplateArgDecl;

  /// The possible result of a `Scope:lookup()` call.
  using AnySuperDecl = std::variant<
      std::shared_ptr<NativeTypeSuperDecl>,
      std::shared_ptr<InterfaceSuperDecl>,
      std::shared_ptr<FuncSuperDecl>,
      std::shared_ptr<VarDecl>>;

  using AnySuperDeclWeak = std::variant<
      std::weak_ptr<NativeTypeSuperDecl>,
      std::weak_ptr<InterfaceSuperDecl>,
      std::weak_ptr<FuncSuperDecl>,
      std::weak_ptr<VarDecl>>;

  struct BoolLiteral;
  struct IntLiteral;

  /// A basic literal may be used as a template argument value, e.g. `T<42>`.
  using BasicLiteral = std::variant<BoolLiteral, IntLiteral>;

  /// A tuple of args passed to a callee, e.g. `(arg: val)` or `<T: U>`.
  template <typename T> struct ArgTuple {
    std::vector<T> vargs;
    std::map<std::string, T> kwargs;
  };

  /// A reference of a declaration, e.g. `T::U`. All template arguments in the
  /// path matter, e.g. `T<A>::U` and `T<B>::U` may be different declarations.
  struct DeclRef {
    struct Element {
      AnySuperDeclWeak decl;
      ArgTuple<TValDecl> args;
    };

    std::vector<Element> elements;
  };

  /// A type expression, e.g. `T & U` or `~T`.
  struct TypeExpr {
    /// TODO: Make it complex.
    DeclRef type_ref;
  };

  /// A (t)ype (val)ue declaration, e.g. `8` in `Bitsize : \uint = 8`.
  using TValDecl = std::variant<BasicLiteral, TypeExpr>;

  struct CStringLiteral;

  /// A runtime literal.
  using RuntimeLiteral =
      Util::Variant::Flatten<std::variant<BasicLiteral, CStringLiteral>>;

  struct Call;

  /// A (r)untime (val)ue (impl)ementation found in implementations.
  using RValImpl = std::
      variant<RuntimeLiteral, std::unique_ptr<Call>, std::shared_ptr<VarImpl>>;

  struct Call {
    std::variant<std::shared_ptr<C::AST::FuncDecl>, std::shared_ptr<FuncImpl>>
        calllee;
    ArgTuple<RValImpl> args;
  };

  struct If;
  struct Return;

  using StatementImpl =
      std::variant<std::unique_ptr<If>, std::unique_ptr<Return>>;

  /// A template type which may be directly mapped to a CST node.
  template <typename NodeT> struct CSTMappable {
    const std::shared_ptr<const NodeT> cst_node;
    CSTMappable(std::shared_ptr<const NodeT> cst_node) : cst_node(cst_node) {}
  };

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

  /// A literal type restriction, e.g. `<Bitsize: \uint>`.
  struct LiteralRestriction : CSTMappable<CST::LiteralRestriction> {
    using CSTMappable::CSTMappable;
    Lang::LiteralRestriction kind() const { return cst_node->kind(); }
  };

  struct TemplateArgDecl : CSTMappable<CST::VarDecl> {
    /// TODO:
    // const std::variant<
    //     std::shared_ptr<TypeDecl>,
    //     std::shared_ptr<FuncDecl>,
    //     std::shared_ptr<TemplateArgDecl>,
    //     std::shared_ptr<FuncArgDecl>,
    //     std::shared_ptr<VarDecl>,
    //     // std::weak_ptr<AnyTypeDecl>, // `struct Foo<T>`
    //     // std::weak_ptr<AnyTypeImpl>, // `forall U struct Foo<U>`
    //     std::weak_ptr<FuncDecl>, // `decl foo<T>(arg : T)`
    //     std::weak_ptr<FuncImpl>> // `forall U impl foo(arg : U)`
    //     parent;

    const std::string name;
    const std::optional<std::string> alias;
    const std::optional<std::unique_ptr<TypeExpr>> restriction;
    const std::optional<TValDecl> default_value;
  };

  struct Scope {
    struct LookupResult {
      /// Has the lookup completely succeeded?
      bool success;

      /// If successfull, contains the actual lookup result. Otherwise would
      /// contain the latest found value, if any.
      std::optional<AnySuperDecl> value;
    };

    virtual LookupResult lookup(Lang::Id) const = 0;
  };

  struct Namespace : Scope, std::enable_shared_from_this<Namespace> {
    const std::weak_ptr<Namespace> parent;

    Namespace(std::weak_ptr<Namespace> parent) : parent(parent) {}

    /// Get the top level scope pointer (may be self).
    virtual std::weak_ptr<TopLevel> top_level() {
      return parent.lock()->top_level();
    }

    LookupResult lookup(Lang::Id) const override;
    void compile(std::shared_ptr<const CST::TypeDecl>, Logger *);
    void compile(std::shared_ptr<const CST::FuncDecl>, Logger *);
    void compile(std::shared_ptr<const CST::VarDecl>, Logger *);
    virtual ~Namespace() = 0;

  protected:
    std::vector<std::shared_ptr<Namespace>> _children;
    std::vector<AnySuperDecl> _super_decls;
  };

  struct Block : Namespace {
    using Node = Util::Variant::Flatten<std::variant<
        RValImpl, // A freestanding rvalue is legal: a block's latest executed
                  // "expression" is deemed to be its returned value.
        StatementImpl>>;

    const Lang::Storage storage;
    const Lang::Safety safety;
    std::vector<Node> nodes;

    /// Only if explicitly set, e.g. `{ 42 } : Int32`.
    std::shared_ptr<DeclTypeRef> explicit_return_type;

    Block(
        std::weak_ptr<Namespace> parent,
        Lang::Storage storage,
        Lang::Safety safety) :
        Namespace(parent), storage(storage), safety(safety) {}

    void compile(std::shared_ptr<const Onyx::CST::Call>, Logger *);
    void compile(std::shared_ptr<const Onyx::CST::ExplSafety>, Logger *);
    void compile(std::shared_ptr<const Onyx::CST::UnOp>, Logger *);
    void compile(std::shared_ptr<const Onyx::CST::BinOp>, Logger *);
    void compile(std::shared_ptr<const Onyx::CST::If>, Logger *);
    void compile(std::shared_ptr<const Onyx::CST::While>, Logger *);
    void compile(std::shared_ptr<const Onyx::CST::Block>, Logger *);
  };

  struct CodeBlockSpec {
    using Node = Util::Variant::Flatten<std::variant<RValSpec, StatementSpec>>;
    /// The originating implementation.
    const std::shared_ptr<Block> impl;
  };

  struct TopLevel : Block {
    /// The containing AST pointer.
    const std::weak_ptr<AST> ast;

    /// Return weak pointer to self.
    std::weak_ptr<TopLevel> top_level() override {
      return dynamic_pointer_cast<TopLevel>(shared_from_this());
    }

    TopLevel(std::weak_ptr<AST> ast) :
        Block(
            std::weak_ptr<Namespace>(),
            Lang::Storage::Static,
            Lang::Safety::Fragile),
        ast(ast) {}
  };

  template <typename DeclT> struct TypeSuperDecl : Namespace {
    const std::string name;

    /// The superdecl template args define all the childrens'.
    const std::map<std::string, TemplateArgDecl> template_args;

    /// The vector of concrete declarations.
    std::vector<std::shared_ptr<DeclT>> decls;

    TypeSuperDecl(std::weak_ptr<Namespace> parent, std::string name) :
        Namespace(parent), name(name) {}
  };

  /// ```nx
  /// forall T decl Parser<Lexer: L : Lexer<T>> # TODO:
  /// ```
  template <typename SuperDeclT>
  struct TypeDecl : Namespace, CSTMappable<CST::TypeDecl> {
    const std::weak_ptr<SuperDeclT> super_decl;
    std::map<std::string, std::shared_ptr<TemplateArgDecl>> template_args;
    std::map<std::string, std::shared_ptr<TemplateArgDecl>> forall;

    TypeDecl(
        std::shared_ptr<const CST::TypeDecl> cst_node,
        std::weak_ptr<Namespace> parent,
        std::weak_ptr<SuperDeclT> super_decl) :
        Namespace(parent), CSTMappable(cst_node), super_decl(super_decl) {}

    std::string name() const { return super_decl.lock()->name; }
  };

  template <typename SuperDeclT>
  struct TypeImpl : Namespace, CSTMappable<CST::TypeDecl> {
    const std::weak_ptr<SuperDeclT> super_decl;
    std::map<std::string, TemplateArgDecl> template_args;
    std::map<std::string, std::shared_ptr<TemplateArgDecl>> forall;

    TypeImpl(
        std::shared_ptr<const CST::TypeDecl> cst_node,
        std::weak_ptr<Namespace> parent,
        std::weak_ptr<SuperDeclT> super_decl) :
        Namespace(parent), CSTMappable(cst_node), super_decl(super_decl) {}

    std::string name() const { return super_decl.lock()->name; }
  };

  template <typename ImplT> struct HasImpls {
    std::vector<std::shared_ptr<ImplT>> impls;
  };

  struct NativeTypeSuperDecl : TypeSuperDecl<NativeTypeDecl>,
                               HasImpls<NativeTypeImpl> {
    enum Kind {
      Int,
    };

    static std::optional<Kind> parse_kind(std::string string) {
      if (!string.compare("Int"))
        return Int;
      else
        return std::nullopt;
    }

    static std::string kind_to_str(Kind kind) {
      switch (kind) {
      case Int:
        return "Int";
      };
    }

    NativeTypeSuperDecl(std::weak_ptr<Namespace> parent, Kind kind) :
        TypeSuperDecl(parent, kind_to_str(kind)) {}
  };

  struct NativeTypeDecl : TypeDecl<NativeTypeSuperDecl> {
    static std::shared_ptr<NativeTypeDecl> compile(
        std::shared_ptr<const CST::TypeDecl>,
        std::shared_ptr<Namespace>,
        Logger *logger);

    NativeTypeDecl(
        std::shared_ptr<const CST::TypeDecl> cst_node,
        std::weak_ptr<Namespace> parent,
        std::weak_ptr<NativeTypeSuperDecl> super_decl) :
        TypeDecl(cst_node, parent, super_decl) {}
  };

  struct NativeTypeImpl : TypeImpl<NativeTypeSuperDecl> {
    using TypeImpl::TypeImpl;
  };

  struct InterfaceSuperDecl : TypeSuperDecl<InterfaceDecl> {};
  struct InterfaceDecl : TypeDecl<InterfaceSuperDecl> {};

  /// A multi-part type declaration reference is used to reference a
  /// not-yet-specialized type declaration.
  ///
  /// For example, in `impl foo(arg : T::U<V>)`, `T::U<V>` is a `DeclTypeRef`
  /// implying that the type specialization must be declared at the moment
  /// of reference (semantic error otherwise), but it doesn't have to be
  /// concretely specialized just yet. In other words, a referenced type
  /// is guarenteed to be specialized at some point later.
  ///
  struct DeclTypeRef : CSTMappable<CST::Id> {
    struct Part : CSTMappable<CST::Id::Part> {
      /// The containing multi-part type reference (the *parts* variable).
      const std::weak_ptr<DeclTypeRef> parent;

      /// The resolved super declaration of the type which is being referenced,
      /// or the according template arg declaration.
      std::variant<std::weak_ptr<TypeSuperDecl>, std::weak_ptr<TemplateArgDecl>>
          decl;

      /// The reference template arguments, if any.
      ArgTuple<TypeRefExpr> args;
    };

    std::vector<std::shared_ptr<Part>> parts;

    static std::shared_ptr<DeclTypeRef> compile(
        std::shared_ptr<const Onyx::CST::Id>,
        std::weak_ptr<Block> parent,
        Logger *);
  };

  /// A complex type reference expression, e.g. `T & U` in `x : T & U`. All
  /// underlying types are referenced, implying that they must be implemented,
  /// but they don't have to be specialized just yet (see `DeclTypeRef`).
  struct TypeRefExpr {
    // TODO: Add complex expressions, such as `T & U` etc.
    //

    using Value = std::
        variant<LiteralRestriction, BasicLiteral, std::shared_ptr<DeclTypeRef>>;

    static TypeRefExpr compile(
        std::shared_ptr<const CST::TypeExpr>,
        std::weak_ptr<Block> parent,
        Logger *);

    Value value;
    TypeRefExpr(Value value) : value(value) {}
  };

  /// A type specialization is triggered by specializing executable code
  /// referencing the type. A specialization may be partial, i.e. incomplete. A
  /// partial specialization may not be used as a runtime value. A
  /// specialization triggers delayed macros evaluation, which may result in new
  /// declarations and implementations, thus the `Namespace` inheritance.
  ///
  /// TODO: Virtual specs, e.g. `x~Trait`.
  /// TODO: A type spec defines memory layout.
  struct TypeSpec : Namespace {
    /// Trigger a type specialization.
    static std::unique_ptr<TypeSpec>
    trigger(std::shared_ptr<DeclTypeRef>, Logger *);

    /// The implementation containing instructions for the specialization.
    std::weak_ptr<TypeImpl> impl;

    /// The type reference which has triggered the specialization.
    std::weak_ptr<DeclTypeRef> ref;

    /// Concrete generic argument specializations, if any.
    /// NOTE: A partial specialization may have fewer arguments specialized.
    /// NOTE: An argument is referenced either by its alias or by its name.
    std::map<std::string, std::variant<BasicLiteral, std::shared_ptr<TypeSpec>>>
        args;

    TypeSpec(std::shared_ptr<TypeImpl> impl) :
        Namespace(impl->super_decl.lock()->parent) {}

    /// Check if the specialization is concrete, i.e. may be used in
    /// runtime. A "partial" specialization is the opposite of "concrete".
    bool is_concrete() const;

    std::string name() const { return impl.lock()->super_decl.lock()->name; }

    /// Compare two type specializations. "Less" and "greater" mean
    /// "narrower" and "broader" type specializations accordingly, e.g.
    /// `Int32 < Integer`. Incompatible types return
    /// `std::partial_ordering::unordered`.
    std::partial_ordering operator<=>(TypeSpec const &other) const;

    /// Make the C++ struct polymorphic.
    virtual ~TypeSpec() = 0;
  };

  /// TODO:
  struct IntPrimitiveSpec : TypeSpec {
    const bool is_signed;
    const std::uint32_t bitsize;

    IntPrimitiveSpec(bool is_signed, std::uint32_t bitsize) :
        TypeSpec(std::weak_ptr<Namespace>()),
        is_signed(is_signed),
        bitsize(bitsize) {}
  };

  /// A struct type specialization.
  struct StructSpec : TypeSpec {
    std::vector<std::pair<std::string, std::shared_ptr<TypeSpec>>> attributes;
  };

  /// A runtime variable declaration, e.g. `let x : Int32 = 42`.
  struct VarDecl : Scope, CSTMappable<CST::VarDecl> {
    using Parent = std::variant<
        std::weak_ptr<FuncDecl>,
        std::weak_ptr<FuncImpl>,
        std::weak_ptr<TypeSuperDecl>,
        std::weak_ptr<TypeDecl>,
        std::weak_ptr<TypeImpl>>;

    const Parent parent;

    std::unique_ptr<TypeRefExpr> type_restriction;
    std::optional<RValImpl> default_value;

    static std::shared_ptr<VarDecl>
    compile(std::shared_ptr<const CST::VarDecl>, Parent, Logger *);

    VarDecl(std::shared_ptr<const CST::VarDecl> cst_node, Parent parent) :
        CSTMappable(cst_node), parent(parent) {}

    std::string name() const { return cst_node->name.value; }

    std::optional<std::string> alias() const {
      if (cst_node->alias)
        return cst_node->alias->value;
      else
        return std::nullopt;
    }

    std::string alias_or_name() const {
      if (alias())
        return alias().value();
      else
        return name();
    }

    /// A variable declaration lookup is delegated to its type restriction.
    /// For example, `assert(x::Bitsize == 32)`.
    LookupResult lookup(Lang::Id) const override;
  };

  /// A variable definition with a specialized type, referencing a `VarDecl`.
  /// TODO: Type masquerade.
  struct VarDef {
    /// The original variable declaration.
    const std::weak_ptr<VarDecl> decl;

    /// The concrete specialized variable type.
    const std::weak_ptr<TypeSpec> type;

    VarDef(std::weak_ptr<VarDecl> decl, std::weak_ptr<TypeSpec> type) :
        decl(decl), type(type) {}
  };

  /// A function super declaration containing function declarations and
  /// implementations. The only thing that children share is the
  /// superdeclaration's name.
  struct FuncSuperDecl {
    const std::weak_ptr<Block> parent;
    const std::string name;

  protected:
    std::vector<std::shared_ptr<FuncDecl>> _decls;
    std::vector<std::shared_ptr<FuncImpl>> _impls;
  };

  /// A function declaration may have nested type and function declarations.
  struct FuncDecl : CSTMappable<CST::FuncDecl> {
    /// The namespace the declaration is physically contained in.
    const std::weak_ptr<Namespace> parent;

    /// The super declaration for this declaration.
    const std::weak_ptr<FuncSuperDecl> super_decl;

    std::map<std::string, TemplateArgDecl> template_args;
    std::map<std::string, std::shared_ptr<VarDecl>> runtime_args;
    std::optional<TypeRefExpr> return_type;

    /// Compile a function declaration, but do not store it anywhere yet.
    static std::shared_ptr<FuncDecl> compile(
        std::shared_ptr<const CST::FuncDecl>, std::weak_ptr<Block>, Logger *);

    FuncDecl(
        std::shared_ptr<const CST::FuncDecl> cst_node,
        std::weak_ptr<Namespace> parent,
        std::weak_ptr<FuncSuperDecl> super_decl) :
        CSTMappable(cst_node), parent, super_decl(super_decl) {}

    std::partial_ordering operator<=>(FuncDecl const &other) const;
    std::string name() const { return super_decl.lock()->name; }
  };

  /// A function implementation doesn't imply a concrete function specialization
  /// just yet. Instead, it "describes" how the function would be implemented.
  /// The same function declaration may be overload-implemented multiple times:
  ///
  /// ```nx
  /// decl foo(a : Number)
  /// impl foo(a : Int) {}
  /// impl foo(a : Int64) {}
  /// ```
  ///
  struct FuncImpl : Scope, CSTMappable<CST::FuncDecl> {
    static std::shared_ptr<FuncImpl> compile(
        std::shared_ptr<const CST::FuncDecl>,
        std::weak_ptr<Namespace> parent,
        Logger *);

    const std::weak_ptr<FuncSuperDecl> super_decl;
    ArgTuple<std::shared_ptr<DeclTypeRef>> template_args;

    /// A `forall` modifier leaves the implementation generic.
    ///
    /// ```
    /// decl convert<Return: R, Val: V>to(arg : V)
    /// forall <T : Float, U : Int> impl convert(arg : U) : T;
    ///
    /// # Ditto-def.
    /// def convert<Return: R, Val: V>(arg : V) : R;
    ///
    /// # NOTE: Also possible, but would still
    /// # prohibit incomplete types in runtime.
    /// impl convert(arg : Int) : Float;
    /// ```
    std::unordered_map<std::string, std::shared_ptr<TemplateArgDecl>> forall;

    std::unordered_map<std::string, std::shared_ptr<VarDecl>> runtime_args;
    std::optional<DeclTypeRef> return_type;
    std::unique_ptr<Block> body;

    /// Return all the matching declarations.
    std::vector<std::shared_ptr<FuncDecl>> matching_decls() const;

  private:
    FuncImpl(
        std::shared_ptr<const CST::FuncDecl> cst_node,
        std::weak_ptr<FuncSuperDecl> super_decl) :
        CSTMappable(cst_node), super_decl(super_decl) {}
  };

  /// A concrete function specialization is triggered by a reference from an
  /// executable code being specialized. A function specialization requires
  /// the return and all the argument types to be completely specialized.
  /// Delayed macros may be evaluated during a specialization, thus a
  /// specialization has its own body.
  struct FuncSpec {
    const std::shared_ptr<FuncImpl> impl;
    const std::vector<std::shared_ptr<VarDef>> runtime_args;
    const std::shared_ptr<TypeSpec> return_type;
    const std::unique_ptr<Block> body;
  };

  struct Return {
    std::optional<RValImpl> value;
  };

  struct Case {
    RValImpl cond;
    Block branch;
  };

  struct If {
    Case self;
    std::vector<Case> elifs;
    std::optional<Block> else_branch;
  };

  AST(std::filesystem::path, const CST *, std::shared_ptr<Logger>);

private:
  const std::shared_ptr<Logger> _logger;
  const std::shared_ptr<TopLevel> _top_level;
  const std::unique_ptr<C::AST> _c_ast;
  void _compile(std::shared_ptr<const Onyx::CST::Extern>);
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
