#pragma once

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <variant>

#include "../c/ast.hh"
#include "../c/block.hh"
#include "../c/mlir.hh"
#include "../util/logger.hh"
#include "../util/map.hh"
#include "../util/node.hh"
#include "../util/variant.hh"
#include "./cst.hh"
#include "./file.hh"
#include "./lang.hh"
#include "./mlir.hh"
#include "./token.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Onyx {

/// An Onyx Abstract Syntax Tree.
struct AST {
  const std::filesystem::path path;

  AST(Program *program,
      std::filesystem::path path,
      MLIR *mlir,
      C::AST *c_ast,
      C::MLIR *c_mlir,
      std::shared_ptr<Util::Logger> logger) :
      path(path),
      _program(program),
      _mlir(mlir),
      _c_ast(c_ast),
      _c_mlir(c_mlir),
      _logger(logger) {}

  /// Compile a CST into this AST.
  /// NOTE: It's not threadsafe.
  void compile(CST *);

protected:
  struct Node : Util::Node {};

  struct Entity;
  struct Scope;
  struct SyntaxScope;
  struct SemanticScope;
  struct Block;
  struct Root;

  struct Namespace;
  struct Alias;

  struct Literal;
  struct NumericLiteral;
  struct StringLiteral;

  struct Restriction;
  struct TemplateArgSuperion;
  struct TemplateArgDecl;

  struct VarSuperion;
  struct VarDef;

  struct FunctionSuperion;
  struct FunctionDecl;
  struct FunctionImpl;
  struct FunctionDef;

  using AnyFuncIon = std::variant<
      std::shared_ptr<FunctionDecl>,
      std::shared_ptr<FunctionImpl>,
      std::shared_ptr<FunctionDef>>;

  struct TraitSuperion;
  struct TraitSubion;
  struct TraitDef;
  struct TraitExt;

  struct StructSuperion;
  struct StructSubion;
  struct StructDef;
  struct StructExt;

  using AnySuperion = std::variant<
      std::shared_ptr<FunctionSuperion>,
      std::shared_ptr<TraitSuperion>,
      std::shared_ptr<StructSuperion>>;

  using AnySuperionWeak = std::variant<
      std::weak_ptr<FunctionSuperion>,
      std::weak_ptr<TraitSuperion>,
      std::weak_ptr<StructSuperion>>;

  using AnyTypeSuperion =
      std::variant<std::shared_ptr<TraitSuperion>, std::shared_ptr<StructSuperion>>;

  using AnyTypeSuperionWeak =
      std::variant<std::weak_ptr<TraitSuperion>, std::weak_ptr<StructSuperion>>;

  // ONYX: In a class header, a specialization ID is stored (i.e. w/ targs).
  using AnySubion = std::variant<
      std::shared_ptr<FunctionDecl>,
      std::shared_ptr<FunctionImpl>,
      std::shared_ptr<FunctionDef>,
      std::shared_ptr<TraitSubion>,
      std::shared_ptr<StructSubion>>;

  using AnySubionWeak = std::variant<
      std::weak_ptr<FunctionDecl>,
      std::weak_ptr<FunctionImpl>,
      std::weak_ptr<FunctionDef>,
      std::weak_ptr<TraitSubion>,
      std::weak_ptr<StructSubion>>;

  /// A entity which may be exported.
  using Exportable = Util::Variant::Flatten<
      std::variant<AnySuperion, std::shared_ptr<Alias>, std::shared_ptr<VarDef>>>;

  struct ID;
  // struct UnOp;
  // struct BinOp;

  using TVal = std::variant<
      std::shared_ptr<ID>,
      std::shared_ptr<NumericLiteral>,
      std::shared_ptr<StringLiteral>>;

#pragma region Util

  template <typename CSTNodeT> struct CSTMappable {
    CSTMappable(std::shared_ptr<CSTNodeT> cst_node) : cst_node(cst_node) {}

  protected:
    const std::shared_ptr<CSTNodeT> cst_node;
  };

  struct Identifiable {
    /// The corresponding token identifier.
    virtual std::shared_ptr<CST::ID> id_node() const = 0;

    /// The string identifier.
    virtual std::string id_string() const { return id_node()->string(); }
  };

  struct HasInstanceFields {
  protected:
    /// Instance field definitions.
    std::unordered_map<std::string, std::shared_ptr<VarDef>> instance_field_defs;
  };

  struct HasInstanceMethods {
  protected:
    /// Instance method declarations.
    std::unordered_map<std::string, AnyFuncIon> instance_method_ion;
  };

  struct HasTArgs {
    std::shared_ptr<TemplateArgDecl> find_targ(std::string id) const {
      if (_targs.contains(id))
        return _targs.at(id);
      else
        return nullptr;
    }

    virtual std::shared_ptr<TemplateArgDecl>
        compile_targ(std::shared_ptr<CST::VarDef>) = 0;

  protected:
    void _add_targ(std::shared_ptr<TemplateArgDecl> decl) {
      auto id = decl->id_string();

      if (auto prev = Util::Map::get_if(_targs, id))
        throw "TODO: Panic! Already declared template arg with id {}";
      else
        _targs[id] = decl;
    }

    std::unordered_map<std::string, std::shared_ptr<TemplateArgDecl>> _targs;
  };

  template <typename... AncestorT> struct HasAncestors {
  protected:
    std::variant<std::shared_ptr<AncestorT>...> ancestors;
  };

#pragma endregion

#pragma region Scope

  /// A generic nested scope with direct access to the AST.
  struct Scope {
    virtual AST *const ast() = 0;
  };

  /// A generic syntax scope is roughly bounded by curly braces and contains ions or code.
  /// A syntax scope is searched with a simple ID node. When prefixed with `::`, searches
  /// the top level scope. `.` and `:` prefixes are currently illegal.
  struct SyntaxScope : Scope {
    const std::optional<std::weak_ptr<SyntaxScope>> parent_syntax_scope;
    std::vector<std::shared_ptr<SyntaxScope>> child_syntax_scopes;

    SyntaxScope(
        std::optional<std::weak_ptr<SyntaxScope>> parent_syntax_scope = std::nullopt) :
        parent_syntax_scope(parent_syntax_scope) {}

    /// A syntax lookup is performed in the following steps:
    ///
    /// 1. Call `_search_in_this`
    /// 2. Search the special scope (see `Token::ID::Kind`)
    /// 3. Search the builtin scope
    /// 4. Search parent syntax scope (recursive with `lookup` call)
    ///
    /// If another entity is found, panic.
    virtual std::shared_ptr<Entity> lookup(std::shared_ptr<CST::ID> id);

  protected:
    /// Lookup exclusively in _this_.
    virtual std::shared_ptr<Entity> _search_in_this(std::shared_ptr<CST::ID>) const = 0;

    /// NOTE: Shall be overriden in `Root`.
    virtual std::shared_ptr<Entity> root_ptr() const {
      return parent_syntax_scope->lock()->root_ptr();
    }
  };

  /// An abstract semantic scope maintains multiple entities, e.g. a type definition.
  struct SemanticScope : Scope {
    const std::optional<std::weak_ptr<SemanticScope>> parent_semantic_scope;
    std::vector<std::shared_ptr<SemanticScope>> child_semantic_scopes;

    SemanticScope(
        std::optional<std::weak_ptr<SemanticScope>> parent_semantic_scope =
            std::nullopt) :
        parent_semantic_scope(parent_semantic_scope) {}

    /// Resolve an ID query to an AST entity node.
    std::shared_ptr<Entity> resolve(std::shared_ptr<CST::IDQuery>) const;

    /// Resolve a simple ID to an AST entity node.
    virtual std::shared_ptr<Entity> resolve(std::shared_ptr<CST::ID>) const = 0;
  };

  /// A block of executable code (AST proxies code specialization to MLIR).
  ///
  /// NOTE: A freestanding code block is legal in source code (root, function body or a
  /// default variable value).
  struct Block : SyntaxScope {
    using ArgsT = std::unordered_map<std::string, std::shared_ptr<VarDef>>;

    /// Runtime arguments for this block, if any (e.g. `(a, b) -> { a + b }`).
    const std::unordered_map<std::string, std::shared_ptr<VarDef>> args;

    // TODO: std::unique_ptr<MLIR::Block> mlir;

    /// A code block in the root.
    Block(std::shared_ptr<Root> parent, ArgsT args = {}) :
        SyntaxScope(parent), args(args) {}

    /// A function definition body.
    Block(std::shared_ptr<FunctionDef> parent, ArgsT args = {}) :
        SyntaxScope(parent), args(args) {}

    /// A function implementation body.
    Block(std::shared_ptr<FunctionImpl> parent, ArgsT args = {}) :
        SyntaxScope(parent), args(args) {}

    /// A variable default value.
    Block(std::shared_ptr<VarDef> parent, ArgsT args = {}) :
        SyntaxScope(parent), args(args) {}

    /// Get the containing AST pointer.
    AST *ast() const { return parent_syntax_scope.value().lock()->ast(); }

    void compile(std::shared_ptr<CST::VarDef>);
    void compile(std::shared_ptr<CST::Call>);
    void compile(std::shared_ptr<CST::UnOp>);
    void compile(std::shared_ptr<CST::BinOp>);
    void compile(std::shared_ptr<CST::If>);
    void compile(std::shared_ptr<CST::Switch>);
    void compile(std::shared_ptr<CST::While>);
    void compile(std::shared_ptr<CST::Control>);

  protected:
    Block() : SyntaxScope() {}
  };

  /// An AST root, i.e. its top-level scope.
  struct Root : Block, SemanticScope, std::enable_shared_from_this<Root> {
    Root(AST *ast) : _ast(ast), Block() {}

    AST *const ast() override { return _ast; }

    std::shared_ptr<Entity> _search_in_this(std::shared_ptr<CST::ID>) const override;

    std::shared_ptr<Entity> resolve(std::shared_ptr<CST::ID> id) const override {
      return _search_in_this(id);
    }

    void compile(std::shared_ptr<CST::Import>);
    void compile(std::shared_ptr<CST::Export>);
    void compile(std::shared_ptr<CST::Alias>);
    void compile(std::shared_ptr<CST::FuncDecl>);
    void compile(std::shared_ptr<CST::FuncDef>);

    virtual void compile(std::shared_ptr<CST::TypeDef>);

    template <typename SupT, typename DefT>
    std::shared_ptr<DefT> compile(std::shared_ptr<CST::TypeDef>);

    void compile(std::shared_ptr<CST::Node>) {
      throw "Panic! Can not have this CST node in root";
    }

    // const char *node_name() const override { return "Root"; }
    // void print(std::ostream &stream, unsigned indent = 0) const override;

  private:
    AST *const _ast;

    std::unordered_map<std::string, std::shared_ptr<VarDef>> _var_defs;
    std::unordered_map<std::string, AnySuperion> _superions;
    std::vector<std::shared_ptr<Block>> _code_blocks;

    /// TODO: Map to a CST node.
    std::unordered_map<std::string, Exportable> _imports;

    /// TODO: Map to a CST node, include an `export { foo: bar }` directive.
    std::unordered_map<std::string, Exportable> _exports;

    void _add_superion(AnySuperion super) {
      auto id = std::visit([](auto &sup) { return sup->id_string(); }, super);

      if (_superions.contains(id))
        throw "Panic! Already declared {} {} as {}";
      else
        _superions[id] = super;
    }
  };

#pragma endregion

#pragma region Entity

  // Anonymous entity.
  /// TODO: In `[1, 2].size()`, `[1, 2]` is also a entity; literal entity
  ///

  /// An abstract entity node. See `Lang::EntityCategory`.
  struct Entity {
    /// The category of this entity.
    virtual Lang::EntityCategory entity_category() const = 0;
  };

  /// A `this` instance can be used within an instance field or function argument
  /// default value expression, e.g. `let x = this.y * 2`.
  struct This : Entity {
    using Container = std::variant<
        std::shared_ptr<FunctionDef>,
        std::shared_ptr<FunctionImpl>,
        std::shared_ptr<VarDef>>;

    Container container;

    This(Container container) : container(container) {
      if (auto def = *std::get_if<std::shared_ptr<VarDef>>(&container)) {
        if (def->is_static())
          throw "TODO: Panic! Can't use `this` in this context";
      }
    }

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::IDLiteral;
    }
  };

  /// A literal type node, e.g. `\Bool`.
  struct LiteralKind : Entity, CSTMappable<CST::LiteralKind> {
    using CSTMappable::CSTMappable;
    Lang::BeLiteralType kind() const { return cst_node->token.kind; }
  };

  /// A numeric literal, e.g. `42`.
  struct NumericLiteral : Entity, CSTMappable<CST::NumericLiteral> {
    using CSTMappable::CSTMappable;
    template <typename T> T to();
  };

  /// A scope-independent ID literal, e.g. `nil`. For a scope-dependent, see `This`.
  struct IDLiteral : Entity, CSTMappable<CST::ID> {
    using CSTMappable::CSTMappable;

    Lang::IDLiteral literal() const { return cst_node->literal().value(); }

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::IDLiteral;
    }
  };

  /// A string literal, e.g. `"foo"`.
  struct StringLiteral : Entity, CSTMappable<CST::StringLiteral> {
    StringLiteral(std::shared_ptr<CST::StringLiteral> cst_node) : CSTMappable(cst_node) {}
    std::string value() const { return cst_node->token.value; }
  };

  /// A pre-specialization node, e.g. `List<T>` and `Enumerable<T>` in
  /// `class List<T> : Enumerable<T>`. Upon specialization, code is generated.
  struct ID : CSTMappable<CST::ID> {
    AnySuperion superion;
    std::vector<TVal> targs;
  };

  struct Impl {};

  struct FunctionSpec {
    std::shared_ptr<MLIR::Function> code;
    std::shared_ptr<FunctionImpl> source_implementation;
  };

  // /// A resolved `Identifiable` reference with template arguments passed, if any.
  // /// TODO: If no targs, direct reference type.
  // struct Specialization : Entity {
  //   const std::variant<
  //       std::shared_ptr<CST::IDRef>,
  //       std::shared_ptr<CST::SpecRef>,
  //       std::shared_ptr<CST::IDQuery>>
  //       cst_node;

  //   /// The resolved reference target.
  //   const std::shared_ptr<Identifiable> target;

  //   Specialization(
  //       std::variant<
  //           std::shared_ptr<CST::IDRef>,
  //           std::shared_ptr<CST::SpecRef>,
  //           std::shared_ptr<CST::IDQuery>> cst_node,
  //       std::shared_ptr<Identifiable> target,
  //       std::vector<TVal> targs) :
  //       cst_node(cst_node), target(target), _targs(targs) {}

  //   void add_targ(TVal targ) { _targs.push_back(targ); }

  //   Lang::EntityCategory entity_category() const override {
  //     return Lang::EntityCategory::Specialization;
  //   }

  // protected:
  //   /// Template arguments passed, if any.
  //   std::vector<TVal> _targs;
  // };

  /// For example, a namespace `T` may be created with `import * as T`.
  /// It is then accessed with `::`, e.g. `T::foo`.
  struct Namespace : SemanticScope, Entity, Identifiable {
    Namespace(std::shared_ptr<SemanticScope> parent_semantic_scope) :
        SemanticScope(parent_semantic_scope) {}

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::Namespace;
    }
  };

#pragma endregion

#pragma region Ion

  /// An abstract entity superion.
  struct Superion : Entity {};

  /// A template superion may declare template argument superions.
  struct TemplateSuperion : Superion, HasTArgs {};

  /// A generic entity ion residing in source code. Note that it isn't
  /// neccessarily linked to a superion, e.g. a freestanding variable. Also it's
  /// not guaranteed to have a name or entity category, e.g. an expression.
  template <typename CSTNodeT> struct Ion : CSTMappable<CSTNodeT>, Entity {
    const std::shared_ptr<CSTNodeT> cst_node;
    const std::shared_ptr<CST::Comment> doc_cst_node;

    Ion(std::shared_ptr<CSTNodeT> cst_node, std::shared_ptr<CST::Comment> doc_cst_node) :
        CSTMappable<CSTNodeT>(cst_node), doc_cst_node(doc_cst_node) {}

    /// The ion kind, e.g. an `Expression`.
    virtual Lang::IonKind ion_kind() const = 0;

    /// Return either a compiled documentation or `nullptr`.
    std::string doc() const;
  };

  /// A subion contains a superion reference, thus shall be identifiable.
  template <typename SupT, typename CSTNodeT>
  struct Subion : Ion<CSTNodeT>, Identifiable {
    const std::weak_ptr<SupT> superion;

    Subion(
        std::weak_ptr<SupT> superion,
        std::shared_ptr<CSTNodeT> cst_node,
        std::shared_ptr<CST::Comment> doc_cst_node) :
        superion(superion), Ion<CSTNodeT>(cst_node, doc_cst_node) {}
  };

  /// An abstract template argument superion residing in a type or function
  /// superion object. Actual declarations must be public-id-compatible with
  /// this.
  struct TemplateArgSuperion : Superion, Identifiable {
    /// The containing type or function superion.
    const AnySuperionWeak parent;

    /// The list of actual declarations, shall not be empty.
    std::vector<std::shared_ptr<TemplateArgDecl>> decls;

    TemplateArgSuperion(AnySuperionWeak parent) : parent(parent) {}

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::TemplateArgument;
    }

    /// Return the public ID token for this superion.
    // std::shared_ptr<CST::ID> id_node() const override {
    //   assert(
    //       decls.size() &&
    //       "There shall be at least one actual template argument
    //       declaration");

    //   return decls.front()->public_id_token();
    // }
  };

  /// An actual template argument declaration node.
  struct TemplateArgDecl : Ion<CST::VarDef>, Identifiable {
    // ONYX: I'd like to be able to pass an uninitialized yet ion pointer here,
    // which would become initialized at some point later. That'd be unsafe,
    // though; unsafe to use the `uninitialized` operator.
    // Therefore, an `unitialized` class instance should be legal, e.g. `unsafe!
    // uninitialized Klass`.
    //

    // A entity containing the template argument declaration.
    using Container = std::variant<
        std::shared_ptr<FunctionDecl>,
        std::shared_ptr<FunctionImpl>,
        std::shared_ptr<FunctionDef>,
        std::shared_ptr<TraitSubion>,
        std::shared_ptr<StructSubion>>;

    static std::shared_ptr<TemplateArgDecl>
        compile_targ(Container, std::shared_ptr<CST::VarDef>);

    /// The container.
    const Container container;

    /// A type restriction, if any. For example, `V` in `T: U : V`.
    const std::shared_ptr<Restriction> restriction;

    TemplateArgDecl(
        std::shared_ptr<CST::VarDef> cst_node,
        std::shared_ptr<CST::Comment> doc_cst_node,
        Container container,
        std::shared_ptr<Restriction> restriction = nullptr) :
        Ion(cst_node, doc_cst_node), container(container), restriction(restriction) {}

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::TemplateArgument;
    }

    Lang::IonKind ion_kind() const override { return Lang::IonKind::Declaration; }

    /// Return the actual argument ID node, e.g. `U` in `T: U : V`.
    std::shared_ptr<CST::ID> id_node() const override { return cst_node->id; }

    /// Return the argument alias token, e.g. `T` in `T: U : V`.
    std::optional<Token::ID> alias_token() const { return cst_node->alias_token; }

    // /// Return the argument alias string, e.g. `"T"` in `T: U : V`.
    // std::string alias_string() const {
    //   if (auto token = alias_token())
    //     return token->value;
    //   else
    //     return nullptr;
    // }

    // /// Return the argument alias or ID token, e.g. `T` in `T: U : V`.
    // Token::Id public_id_token() const {
    //   if (cst_node->alias.has_value())
    //     return cst_node->alias.value();
    //   else
    //     return cst_node->id;
    // }

    // /// Return the argument alias or ID string, e.g. `"T"` in `T: U : V`.
    // std::string public_id_string() const { return public_id_token().value; }
  };

#pragma endregion

#pragma region Expression

  // /// A resolved unary type operation.
  // struct UnOp : Ion<CST::UnOp> {
  //   /// The resolved operator.
  //   std::variant<Lang::TypeUnOp, std::shared_ptr<FuncSuperion>> _operator;

  //   /// The resolved operand.
  //   std::shared_ptr<Entity> operand;

  //   Lang::EntityCategory entity_category() const override {
  //     return Lang::EntityCategory::Expression;
  //   }
  // };

  // /// A resolved binary type operation.
  // struct BinOp : Ion<CST::BinOp> {
  //   /// A built-in binary logic operator.
  //   enum LogicOp { And, Or };

  //   /// The resolved left operand.
  //   std::variant<TVal, std::shared_ptr<Specialization>> left_operand;

  //   /// The resolved operator.
  //   std::variant<LogicOp, std::shared_ptr<FuncSuperion>> _operator;

  //   /// The resolved right operand.
  //   std::variant<TVal, std::shared_ptr<Specialization>> right_operand;

  //   Lang::EntityCategory entity_category() const override {
  //     return Lang::EntityCategory::Expression;
  //   }
  // };

#pragma endregion

#pragma region Type

  /// A single alias directive entry.
  struct Alias : Entity, CSTMappable<CST::Alias::Entry>, Identifiable {
    /// Template argument declarations, if any.
    const std::vector<std::shared_ptr<TemplateArgDecl>> targ_decls;

    /// The resolved target.
    const std::shared_ptr<Entity> target;

    Alias(
        std::shared_ptr<CST::Alias::Entry> cst_node,
        std::vector<std::shared_ptr<TemplateArgDecl>> targ_decls,
        std::shared_ptr<Entity> target) :
        CSTMappable(cst_node), targ_decls(targ_decls), target(target) {}

    /// An alias entry proxies its category to the target's.
    Lang::EntityCategory entity_category() const override {
      return target->entity_category();
    }

    /// The entry identifier CST node.
    std::shared_ptr<CST::ID> id_node() const override { return cst_node->id; }
  };

  /// A resolved type restriction.
  struct Restriction : CSTMappable<CST::Restriction> {
    static std::shared_ptr<Restriction>
        compile(std::shared_ptr<Scope>, std::shared_ptr<CST::Restriction>);

    std::optional<TVal> real_part;
    std::optional<TVal> virtual_part;

    Restriction(
        std::shared_ptr<CST::Restriction> cst_node,
        std::optional<TVal> real_part,
        std::optional<TVal> virtual_part) :
        CSTMappable(cst_node), real_part(real_part), virtual_part(virtual_part) {}
  };

  /// A generic type superion may only have a single definition, but many
  /// extensions.
  template <typename DefT, typename ExtT>
  struct TypeSuperion : TemplateSuperion, Identifiable, SemanticScope {
    std::shared_ptr<SemanticScope> instance_scope;

    /// A type must always have a single definition.
    std::shared_ptr<DefT> def;

    /// A type may have one or more extensions.
    std::vector<std::shared_ptr<ExtT>> exts;

    TypeSuperion(std::shared_ptr<SemanticScope> parent_semantic_scope) :
        SemanticScope(parent_semantic_scope) {
      child_semantic_scopes.push_back(instance_scope);
    }

    std::shared_ptr<CST::ID> id_node() const override {
      assert(def && "A type must have a definition");
      auto id = def->cst_node->id_query->simple_id();
      assert(id && "A type superion must be a simple ID");
      return id;
    }

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::Type;
    }

    virtual Lang::TypeCategory type_category() const = 0;

    std::shared_ptr<Entity> resolve(std::shared_ptr<CST::ID>) const override;
  };

  /// A generic type subion contains template arguments and static members, and
  /// may inherit from a number of ancestors.
  /// TODO: Wrap AncestorT in shared_ptr.
  template <typename SupT>
  struct TypeSubion : Subion<SupT, CST::TypeDef>, SyntaxScope, HasTArgs {
    TypeSubion(
        std::shared_ptr<SupT> superion,
        std::shared_ptr<SyntaxScope> parent_syntax_scope,
        std::shared_ptr<CST::TypeDef> cst_node,
        std::shared_ptr<CST::Comment> doc_cst_node) :
        SyntaxScope(parent_syntax_scope),
        Subion<SupT, CST::TypeDef>(superion, cst_node, doc_cst_node) {}

    std::shared_ptr<CST::ID> id_node() const override {
      auto ref = this->cst_node->id_query->simple_id();
      assert(ref && "A type {ion_category} ID can not be a complex ID query");
      return ref;
    }

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::Type;
    }

    virtual Lang::TypeCategory type_category() const = 0;

    void compile(std::shared_ptr<CST::Alias>);
    void compile(std::shared_ptr<CST::VarDef>);
    void compile(std::shared_ptr<CST::FuncDecl>);
    void compile(std::shared_ptr<CST::FuncDef>);

    AST *const ast() override { return this->parent_scope.value().lock()->ast(); }
    std::shared_ptr<Entity> _search_in_this(std::shared_ptr<CST::ID>) const override;
  };

  template <typename SupT> struct TypeDef : TypeSubion<SupT> {
    std::shared_ptr<TemplateArgDecl> compile_targ(std::shared_ptr<CST::VarDef>) override;
  };

  template <typename SupT> struct TypeExt : TypeSubion<SupT> {
    std::shared_ptr<TemplateArgDecl> compile_targ(std::shared_ptr<CST::VarDef>) override;
  };

#pragma endregion

#pragma region Var

  /// A runtime variable or argument superion.
  struct VarSuperion : Superion, Identifiable {
    using Parent = std::variant<
        std::weak_ptr<Root>,
        std::weak_ptr<FunctionDecl>,
        std::weak_ptr<FunctionDef>,
        std::weak_ptr<FunctionImpl>,
        std::weak_ptr<TraitDef>,
        std::weak_ptr<TraitExt>,
        std::weak_ptr<StructDef>,
        std::weak_ptr<StructExt>>;

    const std::weak_ptr<Parent> parent;
    std::shared_ptr<VarDef> child;

    bool is_static() const { return child->is_static(); }
  };

  /// A runtime variable or argument definition.
  struct VarDef : Subion<VarSuperion, CST::VarDef>, SyntaxScope {
    VarDef(
        std::weak_ptr<VarSuperion> superion,
        std::shared_ptr<SyntaxScope> parent_syntax_scope,
        std::shared_ptr<CST::VarDef> cst_node,
        std::shared_ptr<CST::Comment> doc_cst_node) :
        Subion(superion, cst_node, doc_cst_node), SyntaxScope(parent_syntax_scope) {}

    bool is_static() const {
      return cst_node->modifiers.includes(Token::Keyword::Static);
    }
  };

#pragma endregion

#pragma region Function

  /// A function superion node.
  struct FunctionSuperion : TemplateSuperion, Identifiable {
    using Parent = std::variant<
        std::weak_ptr<Root>,
        std::weak_ptr<TraitDef>,
        std::weak_ptr<TraitExt>,
        std::weak_ptr<StructDef>,
        std::weak_ptr<StructExt>>;

    const std::weak_ptr<Parent> parent;

    std::vector<std::shared_ptr<FunctionDecl>> decls;
    std::vector<std::shared_ptr<FunctionImpl>> impls;
    std::vector<std::shared_ptr<FunctionDef>> defs;

    std::shared_ptr<CST::ID> id_node() const override {
      assert(
          (decls.size() || defs.size()) &&
          "A function superion must have at least one declaration or "
          "definition");

      // BUG: Implement non-simple queries.
      //

      if (decls.size())
        return decls.front()->cst_node->id_query->simple_id();
      else if (defs.size())
        return defs.front()->cst_node->id_query->simple_id();
      else
        throw "BUG: A function superion must have at least one declaration or "
              "definition";
    }

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::Function;
    };

  private:
    friend FunctionDecl;
    friend FunctionDef;

    /// This constructor may only be called by a friend to ensure at least one
    /// declaration or definition exists.
    FunctionSuperion(std::weak_ptr<Parent> parent) : parent(parent), TemplateSuperion() {}
  };

  /// A function subion containing a reference to the parent superion.
  template <typename CSTNodeT>
  struct FunctionSubion : Subion<FunctionSuperion, CSTNodeT>, HasTArgs {
    using ParentScope = std::variant<
        std::weak_ptr<Root>,
        std::weak_ptr<TraitDef>,
        std::weak_ptr<TraitExt>,
        std::weak_ptr<StructDef>,
        std::weak_ptr<StructExt>>;

    const std::weak_ptr<ParentScope> parent_scope;

    FunctionSubion(
        std::shared_ptr<ParentScope> parent_scope,
        std::shared_ptr<FunctionSuperion> superion,
        std::shared_ptr<CSTNodeT> cst_node,
        std::shared_ptr<CST::Comment> doc_cst_node) :
        parent_scope(parent_scope),
        Subion<FunctionSuperion, CSTNodeT>(superion, cst_node, doc_cst_node) {}

    Lang::EntityCategory entity_category() const override {
      return Lang::EntityCategory::Function;
    };
  };

  /// A function declaration node doesn't contain a body.
  /// The template arguments are contained in the type query.
  struct FunctionDecl : FunctionSubion<CST::FuncDecl> {
    /// Would automatically create a function superion in *parent_scope*.
    static std::shared_ptr<FunctionDecl>
    compile(std::weak_ptr<ParentScope> parent_scope, std::shared_ptr<CST::FuncDecl>);

    FunctionDecl(
        std::shared_ptr<ParentScope> parent_scope,
        std::shared_ptr<FunctionSuperion> parent_superion,
        std::shared_ptr<CST::FuncDecl> decl_cst_node,
        std::shared_ptr<CST::Comment> doc_cst_node) :
        FunctionSubion(parent_scope, parent_superion, decl_cst_node, doc_cst_node) {}

    Lang::IonKind ion_kind() const override { return Lang::IonKind::Declaration; };
  };

  /// A function (re)implementation node contains a body.
  /// The template arguments are contained in the `forall` modifier.
  struct FunctionImpl : FunctionSubion<CST::FuncDef>, SyntaxScope {
    /// Would implicitly create the function superion in the scope.
    static std::shared_ptr<FunctionImpl>
    compile(std::weak_ptr<ParentScope> parent_scope, std::shared_ptr<CST::FuncDef>);

    /// The function body block.
    std::shared_ptr<Block> body;

    FunctionImpl(
        std::shared_ptr<ParentScope> parent_scope,
        std::shared_ptr<CST::FuncDef> decl_cst_node,
        std::shared_ptr<FunctionSuperion> parent_superion,
        std::shared_ptr<CST::Comment> doc_cst_node,
        std::shared_ptr<Block> body) :
        FunctionSubion(parent_scope, parent_superion, decl_cst_node, doc_cst_node),
        body(body) {}

    /// Is it a `reimpl` node?
    bool is_re() const {
      return cst_node->action_keyword->kind == Token::Keyword::Reimpl;
    }

    Lang::IonKind ion_kind() const override { return Lang::IonKind::Implementation; };
  };

  /// A function definition node acts both as declaration and implementation.
  /// The template arguments are contained in the type query.
  struct FunctionDef : FunctionSubion<CST::FuncDef>, SyntaxScope {
    /// Would automatically create a function superion in *parent_scope*.
    static std::shared_ptr<FunctionDef>
    compile(std::weak_ptr<ParentScope> parent_scope, std::shared_ptr<CST::FuncDef>);

    /// The function body block.
    std::shared_ptr<Block> body;

    FunctionDef(
        std::shared_ptr<ParentScope> parent_scope,
        std::shared_ptr<CST::FuncDef> decl_cst_node,
        std::shared_ptr<FunctionSuperion> parent_superion,
        std::shared_ptr<CST::Comment> doc_cst_node,
        std::shared_ptr<Block> body) :
        FunctionSubion(parent_scope, parent_superion, decl_cst_node, doc_cst_node),
        body(body) {}

    Lang::IonKind ion_kind() const override { return Lang::IonKind::Definition; };
  };

#pragma endregion

#pragma region Trait

  /// A trait superion node.
  struct TraitSuperion : TypeSuperion<TraitDef, TraitExt> {
    using TypeSuperion::TypeSuperion;

    Lang::TypeCategory type_category() const override {
      return Lang::TypeCategory::Trait;
    };
  };

  /// A generic trait subion contains a superion reference and member ions.
  struct TraitSubion : TypeSubion<TraitSuperion>,
                       HasInstanceMethods,
                       HasAncestors<TraitSuperion> {
    using TypeSubion::TypeSubion;

    Lang::TypeCategory type_category() const override {
      return Lang::TypeCategory::Trait;
    };
  };

  /// A trait definition node.
  struct TraitDef : TraitSubion, TypeDef<TraitSuperion> {
    using TraitSubion::TraitSubion;
    Lang::IonKind ion_kind() const override { return Lang::IonKind::Definition; }
  };

  /// A trait extension node.
  /// Its template arguments are contained in a `forall` modifier.
  struct TraitExt : TraitSubion, TypeExt<TraitSuperion> {
    using TraitSubion::TraitSubion;
    Lang::IonKind ion_kind() const override { return Lang::IonKind::Extension; }
  };

#pragma endregion

#pragma region Struct

  /// A struct superion node.
  struct StructSuperion : TypeSuperion<StructDef, StructExt> {
    using TypeSuperion::TypeSuperion;

    Lang::TypeCategory type_category() const override {
      return Lang::TypeCategory::Struct;
    };
  };

  /// A generic struct subion contains a superion reference and member ions.
  struct StructSubion : TypeSubion<StructSuperion>,
                        HasInstanceFields,
                        HasInstanceMethods,
                        HasAncestors<TraitSuperion, StructSuperion> {
    using TypeSubion::TypeSubion;

    Lang::TypeCategory type_category() const override {
      return Lang::TypeCategory::Struct;
    };
  };

  /// A struct definition node.
  struct StructDef : StructSubion,
                     TypeDef<StructSuperion>,
                     std::enable_shared_from_this<StructDef> {
    using StructSubion::StructSubion;
    Lang::IonKind ion_kind() const override { return Lang::IonKind::Definition; }
  };

  /// A struct extension node.
  struct StructExt : StructSubion, TypeExt<StructSuperion> {
    using StructSubion::StructSubion;
    Lang::IonKind ion_kind() const override { return Lang::IonKind::Extension; }
  };

#pragma endregion

#pragma region Panic

  // Semantic panic definitions.
  //

  /// `P002`.ID
  struct UndeclaredReference : Panic {
    UndeclaredReference(Token::ID id) :
        Panic(
            Panic::UndeclaredReference,
            fmt::format("Undeclared reference `{}`", id.Token::print()),
            id.placement) {}
  };

  /// `P003`.
  struct DeclarationCategoryMismatch : Panic {
    DeclarationCategoryMismatch(
        Token::ID id, Token::ID previous_id, Lang::EntityCategory previous_category) :
        Panic(
            Panic::DeclarationCategoryMismatch,
            fmt::format(
                "Already declared `{}` as {}",
                id.value,
                Lang::entity_category_string(previous_category)),
            id.placement,
            {{"Previously declared here", previous_id.placement}}) {}
  };

  struct AlreadyDeclared : Panic {
    AlreadyDeclared(
        std::string id, std::shared_ptr<CST::ID> prev, std::shared_ptr<CST::ID> current) :
        Panic(
            Panic::AlreadyDeclared,
            "Already declared " + id,
            current->id.placement,
            {{"Previously declared here", prev->id.placement}}) {}
  };

#pragma endregion

  /// A prespecialization.
  struct Prespec {};

  /// The map of exported entities for this AST.
  /// TODO: Map to `export` keyword(s), allow freestanding `export`
  /// directives.
  std::unordered_map<std::string, Exportable> exports() const;

  /// The default export for this AST, if any.
  /// TODO: Map to the `default` keyword, allow freestanding `export`
  /// directive.
  std::optional<Exportable> default_export() const;

  std::shared_ptr<CST::Comment> adjacent_comment() const { return _adjacent_comment; }

private:
  /// The latest comment node reference unless disrupted by another node.
  std::shared_ptr<CST::Comment> _adjacent_comment;

  /// The program containing the AST.
  const Program *_program;

  /// The linked Onyx MLIR.
  MLIR *const _mlir;

  /// The linked static C AST.
  C::AST *const _c_ast;

  /// The linked static C MLIR.
  C::MLIR *const _c_mlir;

  /// A logger instance for this AST.
  const std::shared_ptr<Util::Logger> _logger;

  /// The root node.
  std::shared_ptr<Root> _root;
};

} // namespace Onyx
} // namespace Phoenix
} // namespace Fancysoft
