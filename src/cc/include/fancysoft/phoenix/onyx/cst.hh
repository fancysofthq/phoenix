#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>

#include "../c/block.hh"
#include "../exception.hh"
#include "../panic.hh"
#include "../util/node.hh"
#include "./token.hh"
#include "fancysoft/phoenix/onyx/lang.hh"
#include "fancysoft/phoenix/placement.hh"
#include "fancysoft/phoenix/util/variant.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Onyx {

/// An Onyx Concrete Syntax Tree.
///
/// TODO: Preserve newline or semicolon.
///
/// DESIGN: CST is designed so that it preserves only significant syntax information,
/// whereas the number of empty lines is insignificant, along with a semicolon followed by
/// a newline.
struct CST {
  struct Node : Util::Node {};

  struct EmptyLine;
  struct Comment;

  template <typename TokenT> struct Literal;
  using LiteralKind = Literal<Token::LiteralKind>;
  using NumericLiteral = Literal<Token::NumericLiteral>;
  using StringLiteral = Literal<Token::StringLiteral>;
  struct Container;

  struct Extern;
  struct Import;
  // struct Export;

  struct VarDef;
  struct FuncDecl;
  struct FuncDef;
  struct TypeDef;

  struct Alias;

  struct ID;
  struct IDQuery;
  struct Call;

  struct UnOp;
  struct BinOp;

  struct If;
  struct Switch;
  struct While;
  struct Control;

  using Expression =
      std::variant<std::shared_ptr<Call>, std::shared_ptr<UnOp>, std::shared_ptr<BinOp>>;

  /// An rval may be directly assigned.
  using RVal = Util::Variant::Flatten<std::variant<
      std::shared_ptr<LiteralKind>,
      std::shared_ptr<ID>,
      std::shared_ptr<IDQuery>,
      Expression>>;

  /// A `TVal` may be used as a template argument.
  using TVal = Util::Variant::Flatten<std::variant<
      std::shared_ptr<NumericLiteral>,
      std::shared_ptr<StringLiteral>,
      std::shared_ptr<IDQuery>
      // std::shared_ptr<UnOp>,
      // std::shared_ptr<BinOp>,
      >>;

  using Statement = std::variant<
      std::shared_ptr<If>,
      std::shared_ptr<Switch>,
      std::shared_ptr<While>,
      std::shared_ptr<Control>>;

#pragma region Util

  /// A entity which may have `export` or `export default` modifier.
  struct Exportable {
    const std::optional<Token::Keyword> export_keyword;
    const std::optional<Token::Keyword> default_keyword;

    Exportable(
        std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword) :
        export_keyword(export_keyword), default_keyword(default_keyword) {
      if (default_keyword.has_value() && !export_keyword.has_value())
        throw "BUG: Can not be `default` without `exported`";
    }

    bool is_exported() const { return export_keyword.has_value(); }
    bool is_exported_by_default() const { return default_keyword.has_value(); }
  };

  /// An utility type containing a list of keywords.
  struct Keywords {
    const std::vector<Token::Keyword> tokens;

    /// Find the first keyword token with given kind.
    std::optional<Token::Keyword> find(Token::Keyword::Kind) const;

    /// Return true if contains the keyword kind.
    bool includes(Token::Keyword::Kind) const;

    /// Return the first keyword which is not in the set of *allowed*.
    std::optional<Token::Keyword> disjoint(std::set<Token::Keyword::Kind> allowed) const;
  };

  /// Multiple variables declarations, e.g. `let x, y = 1, final z = 2`.
  struct MultiVarDecl {
    const std::vector<std::shared_ptr<VarDef>> decls;
    MultiVarDecl(std::vector<std::shared_ptr<VarDef>> decls) : decls(decls) {}
  };

#pragma endregion

#pragma region Node

  /// A empty line node.
  struct EmptyLine : Node {
    const Token::Punct token;
    EmptyLine(Token::Punct token) : token(token) {}
    const char *node_name() const override { return "EmptyLine"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A comment node comprised of multiple adjacent comment tokens.
  struct Comment : Node {
    const std::vector<Token::Comment> tokens;
    Comment(std::vector<Token::Comment> tokens) : tokens(tokens) {}
    const char *node_name() const override { return "Comment"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// An `extern` directive imprints raw C code.
  struct Extern : Node {
    /// The `extern` keyword token.
    const Token::Keyword keyword;

    /// The virtual C code block.
    std::shared_ptr<C::Block> block;

    Extern(Token::Keyword keyword, std::shared_ptr<C::Block> block) :
        keyword(keyword), block(block) {}

    const char *node_name() const override { return "Extern"; }
    void print(std::ostream &, unsigned indent = 0) const override;
  };

  /// An `alias` directive.
  struct Alias : Node {
    struct Entry {
      std::shared_ptr<ID> id;
      std::optional<MultiVarDecl> targs;

      Entry(std::shared_ptr<ID> id, std::optional<MultiVarDecl> targs = std::nullopt) :
          id(id), targs(targs) {
        assert(id && "An `alias` entry must have an identifier");
      }
    };

    Token::Keyword alias_keyword;
    std::vector<std::shared_ptr<Entry>> entries;
    Token::Keyword to_keyword;
    std::shared_ptr<IDQuery> target;

    Alias(
        Token::Keyword alias_keyword,
        std::vector<std::shared_ptr<Entry>> entries,
        Token::Keyword to_keyword,
        std::shared_ptr<IDQuery> target) :
        alias_keyword(alias_keyword),
        entries(entries),
        to_keyword(to_keyword),
        target(target) {
      assert(target && "An `alias` target must be set");
    }
  };

  template <typename TokenT> struct Literal {
    TokenT token;
    Literal(TokenT token) : token(token) {}
  };

  /// A single ID reference node with optional template arguments, e.g. `foo<T>`.
  struct ID : Node {
    Token::ID id;

    /// Template arguments being passed, e.g. `<T, U>`.
    std::vector<TVal> args;

    std::string string() const { return id.string(); }
    std::optional<Lang::IDLiteral> literal() const { return id.literal(); }
  };

  /// A complex identifier query node, e.g. `(Foo && Bar<T>)::Baz<U>`, or simply `foo`.
  ///
  /// TODO: Implement complex expressions; currently, an element may only be
  /// either `ID` or `TemplateID`.
  struct IDQuery : Node {
    struct Element : Node {
      using Value = std::variant<std::shared_ptr<ID>

                                 // /// TIP: `((A, B), C)::[0, 1]` returns `B`.
                                 // std::shared_ptr<Container>,

                                 // std::shared_ptr<UnOp>,
                                 // std::shared_ptr<BinOp>
                                 >;

      Token::Punct scope_access_token;
      Value value;

      Element(Token::Punct scope_access_token, Value value) :
          scope_access_token(scope_access_token), value(value) {}

      Lang::AccessScope scope_access_kind() const {
        switch (scope_access_token.kind) {
        case Token::Punct::ScopeStatic:
          return Lang::AccessScope::Static;
        case Token::Punct::ScopeInstance:
          return Lang::AccessScope::Instance;
        case Token::Punct::ScopeUFCS:
          return Lang::AccessScope::UFCS;
        default:
          throw "Unexpected keyword";
        }
      }

      Placement placement() const;
    };

    using Path = std::vector<std::shared_ptr<Element>>;

    const Path path;
    IDQuery(Path path) : path(path) {}

    inline std::shared_ptr<ID> simple_id() const { return _simple<ID>(); }

    /// The total node placement.
    Placement placement() const;
    // {
    //   return Placement(
    //       path.front()->payload.placement.unit,
    //       {path.front()->payload.placement.location.start,
    //        path.back()->payload.placement.location.end});
    // }

    void print(std::ostream &, unsigned indent = 0) const override;

  private:
    /// Return a simple `ID` node if it is a simple query, otherwise `nullptr`.
    template <typename NodeT> std::shared_ptr<NodeT> _simple() const {
      if (path.size() != 1)
        return nullptr;

      if (auto id = *std::get_if<std::shared_ptr<NodeT>>(&path.back()->value))
        return id;
      else
        return nullptr;
    }
  };

  /// An `import` node.
  struct Import : Node {
    // Implemented variants:
    //
    //   * [x] `import T`
    //   * [x] `import * as T`
    //   * [x] `import { T }`
    //   * [x] `import { T as U }`
    //   * [ ] `import { T, U as V } as W`
    //

    /// An optional `as` clause, e.g. `import { Foo as Bar }`.
    struct Alias {
      const Token::Punct as_keyword;
      const Token::ID alias_id;

      Alias(Token::Punct as_keyword, Token::ID alias_id) :
          as_keyword(as_keyword), alias_id(alias_id) {}
    };

    /// The imported element, e.g. `Foo` in `import Foo from "path"`.
    struct Element {
      /// An explicitly destructed element is non-default,
      /// e.g. `import { Foo } from "path"`.
      const bool destructed;

      /// The punct token `*` is only allowed when not *destructed*,
      /// i.e. `import * from` is legal, but `import { * } from` is not.
      const std::variant<Token::ID, Token::Punct> id;

      /// The optional `as` clause, e.g. `import { Foo as Bar }`.
      const std::optional<Alias> alias;

      Element(
          bool destructed,
          std::variant<Token::ID, Token::Punct> id,
          std::optional<Alias> alias) :
          destructed(destructed), id(id), alias(alias) {}
    };

    /// The `import` keyword token.
    const Token::Keyword import_keyword;

    /// The elements being imported.
    const std::vector<Element> elements;

    /// The `from` keyword token.
    const Token::Keyword from_keyword;

    /// The `from` clause value, i.e. the path.
    const Token::StringLiteral from_value;

    Import(
        Token::Keyword import_keyword,
        bool wrapped,
        std::vector<Element> elements,
        Token::Keyword from_keyword,
        Token::StringLiteral from_value) :
        import_keyword(import_keyword),
        elements(elements),
        from_keyword(from_keyword),
        from_value(from_value) {}

    const char *node_name() const override { return "Import"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A freestanding `export` node, e.g. `export foo;`.
  struct Export : Node {
    // TODO:
  };

  /// An unary operation node.
  struct UnOp : Node {
    const Token::Op _operator;
    const RVal operand;

    UnOp(Token::Op _operator, RVal operand) : _operator(_operator), operand(operand) {}

    // OPTIMIZE: Memoize.
    std::optional<Lang::WellKnownUnOp> well_known_op() const {
      return Lang::parse_well_known_unop(_operator.name.c_str());
    }

    const char *node_name() const override { return "UnOp"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A binary operation node.
  /// TODO: `~` doesn't require spaces unless wrapped in parens? `(x ~ T).foo`,
  /// but `x~T.foo`
  struct BinOp : Node {
    const RVal left_operand;
    const Token::Op _operator;
    const RVal right_operand;

    BinOp(RVal left_operand, Token::Op _operator, RVal right_operand) :
        left_operand(left_operand), _operator(_operator), right_operand(right_operand) {}

    // OPTIMIZE: Memoize.
    std::optional<Lang::WellKnownBinOp> well_known_op() const {
      return Lang::parse_well_known_binop(_operator.name.c_str());
    }

    const char *node_name() const override { return "BinOp"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A type restriction. Either of *real_part* or *virtual_part* must be set.
  struct Restriction : Node {
    /// The real part of the restriction. If set, the restriction operator would
    /// be `:`. Otherwise, it's `~`.
    const std::optional<Expression> real_part;

    /// The virtual part of the restriction.
    const std::optional<Expression> virtual_part;

    Restriction(
        std::optional<Expression> real_part, std::optional<Expression> virtual_part) :
        real_part(real_part), virtual_part(virtual_part) {
      assert((real_part || virtual_part) && "Either real or virtual part must be set");
    }

    const char *node_name() const override { return "Restriction"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A single variable declaration node with optional value.
  struct VarDef : Node, Exportable {
    /// Either one of `let`, `final` or `getter`; may be omitted.
    const std::optional<Token::Keyword> directive_keyword;

    /// Can be `private` or `static`. In a multi-var declaration, only the first
    /// variable is allowed to have a modifier.
    const Keywords modifiers;

    /// The optional alias label token, e.g. `foo` in `let foo: bar : T`.
    const std::optional<Token::ID> alias_token;

    /// The variable identifier node.
    const std::shared_ptr<ID> id;

    /// May be `nullptr`.
    const std::shared_ptr<Restriction> restriction;

    /// The variable's optional value.
    const std::optional<RVal> value;

    VarDef(
        std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword,
        Keywords modifiers,
        std::optional<Token::Keyword> directive_keyword,
        std::optional<Token::ID> alias_token,
        std::shared_ptr<ID> id,
        std::shared_ptr<Restriction> restriction,
        std::optional<RVal> value) :
        Exportable(export_keyword, default_keyword),
        modifiers(modifiers),
        directive_keyword(directive_keyword),
        alias_token(alias_token),
        id(id),
        restriction(restriction),
        value(value) {
      auto invalid =
          modifiers.disjoint({Token::Keyword::Private, Token::Keyword::Static});

      if (invalid)
        throw "TODO:"; // TODO:
    }

    std::string id_string() const { return id->string(); }

    std::string alias_or_id_string() const {
      if (alias_token)
        return alias_token->string();
      else
        return id->string();
    }

    const char *node_name() const override { return "VarDef"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A call node, e.g. `foo()` or `foo.bar()`.
  struct Call : Node {
    using Callee = std::variant<std::shared_ptr<ID>, std::shared_ptr<IDQuery>>;

    const Callee callee;
    const std::vector<RVal> args;

    Call(Callee callee, std::vector<RVal> args) : callee(callee), args(args) {}

    const char *node_name() const override { return "Call"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A `forall` modifier.
  struct Forall {
    /// The `forall` keyword token.
    const Token::Keyword keyword;

    /// Is it wrapped in `[]`?
    const bool wrapped;

    /// The temporary template args declaration.
    const MultiVarDecl args;
  };

  /// A type or function declaration node without a body.
  struct Decl : Node, Exportable {
    /// The declaration modifiers. Only `private` and `static` are allowed.
    const Keywords modifiers;

    /// The `decl` (or `def`) keyword.
    const std::optional<Token::Keyword> action_keyword;

    /// For a type declration, this would be a mandatory `trait`, `struct`,
    /// `class`, `enum`, `unit` or `annotation` keyword. For a function
    /// declaration, this would be an optional `function` keyword.
    const std::optional<Token::Keyword> category_keyword;

    /// The declaration ID query. Passing template arguments to the last query
    /// element would be illegal.
    const std::shared_ptr<IDQuery> id_query;

    /// An optional template arguments declaration following the ID query.
    const std::optional<MultiVarDecl> template_args;

    Decl(
        std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword,
        Keywords modifiers,
        std::optional<Token::Keyword> action_keyword,
        std::optional<Token::Keyword> category_keyword,
        std::shared_ptr<IDQuery> id_query,
        std::optional<MultiVarDecl> template_args) :
        Exportable(export_keyword, default_keyword),
        modifiers(modifiers),
        action_keyword(action_keyword),
        category_keyword(category_keyword),
        id_query(id_query),
        template_args(template_args) {}

    virtual const char *node_name() const = 0;
    virtual void print(std::ostream &o, unsigned indent = 0) const = 0;
  };

  /// A type or function implementation or definition node.
  struct Def : Node, Exportable {
    /// An optional `forall` modifier.
    const std::optional<Forall> forall;

    /// The definition modifiers. Only `private` and `static` are allowed.
    const Keywords modifiers;

    /// For a type implementation, it'd be one of `def`, `impl` or `extend`.
    /// For a function implementation, it'd be `def`, `impl` or `reimpl`. May be
    /// inferred for a definition.
    const std::optional<Token::Keyword> action_keyword;

    /// For a type implementation, it'd be one of `trait`, `struct`, `class`,
    /// `enum`, `unit` or `annotation`. For a function implementation, it'd be
    /// `function`. In some cases, the category may be inferred.
    const std::optional<Token::Keyword> category_keyword;

    /// The implemented ID query.
    const std::shared_ptr<IDQuery> id_query;

    /// An optional template arguments declaration following the ID query,
    /// applicable to definitions only.
    const std::optional<MultiVarDecl> template_args;

    /// The list of the ancestors, if any.
    /// TODO: An ancestor may be wrapped, each on separate line and documented.
    const std::vector<std::shared_ptr<IDQuery>> ancestors;

    Def(std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword,
        std::optional<Forall> forall,
        Keywords modifiers,
        std::optional<Token::Keyword> action_keyword,
        std::optional<Token::Keyword> category_keyword,
        std::shared_ptr<IDQuery> id_query,
        std::optional<MultiVarDecl> template_args,
        std::vector<std::shared_ptr<IDQuery>> ancestors) :
        Exportable(export_keyword, default_keyword),
        forall(forall),
        modifiers(modifiers),
        action_keyword(action_keyword),
        category_keyword(category_keyword),
        id_query(id_query),
        template_args(template_args),
        ancestors(ancestors) {}

    virtual const char *node_name() const = 0;
    virtual void print(std::ostream &o, unsigned indent = 0) const = 0;
  };

  /// A type definition node.
  struct TypeDef : Def {
    TypeDef(
        std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword,
        std::optional<Forall> forall,
        Keywords modifiers,
        std::optional<Token::Keyword> action_keyword,
        std::optional<Token::Keyword> category_keyword,
        std::shared_ptr<IDQuery> id_query,
        std::optional<MultiVarDecl> template_args,
        std::vector<std::shared_ptr<IDQuery>> ancestors) :
        Def(export_keyword,
            default_keyword,
            forall,
            modifiers,
            action_keyword,
            category_keyword,
            id_query,
            template_args,
            ancestors) {
      if (auto illegal =
              modifiers.disjoint({Token::Keyword::Private, Token::Keyword::Static})) {
        // TODO: Panic.
      }

      if (action_keyword.has_value())
        switch (action_keyword->kind) {
        case Token::Keyword::Def:
        case Token::Keyword::Extend:
          break; // OK
        default:
          throw "Panic: Unexpected keyword {}";
        }

      if (category_keyword.has_value())
        switch (category_keyword->kind) {
        case Token::Keyword::Trait:
        case Token::Keyword::Struct:
          // case Token::Keyword::Class:
          // case Token::Keyword::Enum:
          // case Token::Keyword::Unit:
          // case Token::Keyword::Annotation:
          break; // OK
        default:
          throw "Panic: Unexpected keyword {}";
        }
    }

    std::optional<Lang::TypeIonKind> type_ion_kind() const {
      if (action_keyword.has_value()) {
        switch (action_keyword->kind) {
        case Token::Keyword::Def:
          return Lang::TypeIonKind::Definition;
        case Token::Keyword::Extend:
          return Lang::TypeIonKind::Extension;
        default:
          throw "BUG";
        }
      } else {
        return std::nullopt;
      }
    }

    std::optional<Lang::TypeCategory> type_category() const {
      if (category_keyword.has_value()) {
        switch (category_keyword->kind) {
        case Token::Keyword::Trait:
          return Lang::TypeCategory::Trait;
        case Token::Keyword::Struct:
          return Lang::TypeCategory::Struct;
        case Token::Keyword::Class:
        //   return Lang::TypeCategory::Class;
        // case Token::Keyword::Enum:
        //   return Lang::TypeCategory::Enum;
        // case Token::Keyword::Unit:
        //   return Lang::TypeCategory::Unit;
        // case Token::Keyword::Annotation:
        //   return Lang::TypeCategory::Annotation;
        default:
          throw "BUG";
        }
      } else {
        return std::nullopt;
      }
    }

    const char *node_name() const override { return "TypeImpl"; }
    // void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A function declaration node.
  struct FuncDecl : Decl {
    FuncDecl(
        std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword,
        Keywords modifiers,
        Token::Keyword action_keyword,
        std::optional<Token::Keyword> function_keyword,
        std::shared_ptr<IDQuery> id_query,
        std::optional<MultiVarDecl> template_args) :
        Decl(
            export_keyword,
            default_keyword,
            modifiers,
            action_keyword,
            function_keyword,
            id_query,
            template_args) {
      if (auto illegal =
              modifiers.disjoint({Token::Keyword::Private, Token::Keyword::Static})) {
        // TODO: Panic.
      }
    }

    const char *node_name() const override { return "FuncDecl"; }
    void print(std::ostream &o, unsigned indent = 0) const override;
  };

  /// A function definition node.
  /// TODO: A builtin function definition doesn't have body.
  struct FuncDef : Def {
    FuncDef(
        std::optional<Token::Keyword> export_keyword,
        std::optional<Token::Keyword> default_keyword,
        std::optional<Forall> forall,
        Keywords modifiers,
        std::optional<Token::Keyword> action_keyword,
        std::optional<Token::Keyword> category_keyword,
        std::shared_ptr<IDQuery> id_query,
        std::optional<MultiVarDecl> template_args,
        std::vector<std::shared_ptr<IDQuery>> ancestors) :
        Def(export_keyword,
            default_keyword,
            forall,
            modifiers,
            action_keyword,
            category_keyword,
            id_query,
            template_args,
            ancestors) {
      if (auto illegal = modifiers.disjoint(
              {Token::Keyword::Builtin,
               Token::Keyword::Private,
               Token::Keyword::Static})) {
        // TODO: Panic.
      }
    }

    const char *node_name() const override { return "FuncDef"; }
    // void print(std::ostream &o, unsigned indent = 0) const override;
  };

#pragma endregion

  /// A CST root is comprised of nodes.
  std::vector<std::shared_ptr<Node>> root;
};

} // namespace Onyx
} // namespace Phoenix
} // namespace Fancysoft
