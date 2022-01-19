#pragma once

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>

#include <fancysoft/util/variant.hh>
#include <fmt/core.h>

#include "../c/block.hh"
#include "../exception.hh"
#include "../node.hh"
#include "../placement.hh"
#include "./lang.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// An Onyx Concrete Syntax Tree.
namespace CST {

struct Node : NXC::Node {
  virtual Placement placement() const;
};

struct EmptyLine;
struct Comment;

struct Extern;
struct Import;
struct Export;

// A directive may only reside at the top level.
using Directive = std::variant<
    std::shared_ptr<Extern>,
    std::shared_ptr<Import>,
    std::shared_ptr<Export>>;

struct VarDecl;
struct FuncDecl;
struct TypeDecl;
// struct EnumDecl;

// A declaration shall can not be passed around.
using Decl = std::variant<
    std::shared_ptr<VarDecl>,
    std::shared_ptr<FuncDecl>,
    std::shared_ptr<TypeDecl>
    // std::shared_ptr<EnumDecl>,
    >;

// struct String;
// struct Char;
struct Bool;
struct Int;
// struct Float;

/// A basic literal may be used as a type restriction, e.g. in `Int<32>`.
using BasicLiteral = std::variant<
    // std::shared_ptr<String>,
    // std::shared_ptr<Char>,
    std::shared_ptr<Bool>,
    std::shared_ptr<Int>
    // std::shared_ptr<Float>
    >;

struct RTuple;
struct Array;

// struct Map;
// struct Vector;
// struct Tensor;
// struct Magic;

struct CString;
// struct CInt;

// A literal which may be used as a runtime value.
using RuntimeLiteral = Util::Variant::Flatten<std::variant<
    BasicLiteral,
    std::shared_ptr<RTuple>,
    std::shared_ptr<Array>,
    // std::shared_ptr<Map>,
    // std::shared_ptr<Vector>,
    // std::shared_ptr<Tensor>,
    // std::shared_ptr<Magic>,
    std::shared_ptr<CString>>>;

struct GreedyArg;
struct Id;
struct CId;

/// A runtime value.
using Val = Util::Variant::Flatten<std::variant<
    RuntimeLiteral,
    std::shared_ptr<GreedyArg>,
    std::shared_ptr<Id>,
    std::shared_ptr<CId>>>;

struct RuntimeAssign;
struct Call;
// struct NoOp;

struct UnOp;
struct BinOp;
// struct Ternary;

/// An expression is ought to return a value. It shall be terminated unless
/// inline. It may be either freestanding or used directly as an rvalue.
using Expr = std::variant<
    std::shared_ptr<RuntimeAssign>,
    std::shared_ptr<Call>,
    // std::shared_ptr<NoOp>,
    std::shared_ptr<UnOp>,
    std::shared_ptr<BinOp>
    // std::shared_ptr<Ternary>
    >;

struct ExplSafetyExpr;
struct Block;

/// An RVal may be used as an rvalue, i.e. directly as an argument without
/// wrapping it into a monuple.
///
/// ```
/// let x = 42            # OK
/// let x = foo()         # OK, a call is an rvalue
/// let x = !foo          # OK, an unop is an rvalue
/// let x = foo + bar     # OK, a binop is an rvalue
/// let x = unsafe! foo   # OK, an expl. safety expr. is an rvalue
/// let x = (unsafe! foo) # OK, a monuple is an rvalue
/// let x = { foo() }     # OK, a block is evaluated to a value
/// ```
using RVal = Util::Variant::Flatten<std::variant<
    Val,
    Expr,
    std::shared_ptr<ExplSafetyExpr>,
    std::shared_ptr<Block>>>;

struct If;
struct While;
// struct Switch;

/// A branch statement, e.g. `if`.
using BranchStatement =
    std::variant<std::shared_ptr<If>, std::shared_ptr<While>>;

struct Return;
// struct Break;
// struct Continue;

/// A control statement.
using ControlStatement = std::variant<std::shared_ptr<Return>>;

struct Alias;

/// A statement shall be freestanding and terminated,
/// and it may not be used directly as an rvalue.
using Statement = Util::Variant::Flatten<
    std::variant<BranchStatement, ControlStatement, std::shared_ptr<Alias>>>;

struct TypeExpr;

/// A entity which may have an `export` keyword.
struct Exportable {
  const std::optional<Token::Keyword> export_keyword;
  const std::optional<Token::Keyword> default_keyword;

  Exportable(
      std::optional<Token::Keyword> export_keyword,
      std::optional<Token::Keyword> default_keyword) :
      export_keyword(export_keyword), default_keyword(default_keyword) {
    if (default_keyword.has_value() && !export_keyword.has_value())
      throw "BUG: Can not be default but not exported";
  }

  bool is_exported() const { return export_keyword.has_value(); }
  bool is_exported_default() const { return default_keyword.has_value(); }
};

/// A comment node.
struct Comment : Node {
  const std::vector<Token::Comment> tokens;
  Comment(std::vector<Token::Comment> tokens) : tokens(tokens) {}
  const char *node_name() const override { return "Comment"; }

  void print(std::ostream &o, unsigned indent = 0) const override {
    for (auto &token : tokens) {
      print_tab(o, indent);
      o << '#' << token.value << '\n';
    }
  }
};

/// A CST node which can have a documentation, compiled from adjacent
/// preceding comment lines.
struct HasDocs {
  const std::shared_ptr<Comment> docs_node;

  HasDocs(std::shared_ptr<Comment> docs_node) : docs_node(docs_node) {}

  /// Return the compiled docs string.
  std::string docs() const {
    std::stringstream buf;

    for (auto &token : docs_node->tokens)
      buf << token.value;

    return buf.str();
  }
};

/// A empty line node.
struct EmptyLine : Node {
  const Token::Punct token;
  EmptyLine(Token::Punct token) : token(token) {}
  const char *node_name() const override { return "EmptyLine"; }

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);
  };
};

/// An extern directive imprints raw C code.
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

/// A full-path identifier node, e.g. `&(Foo & Bar<T>)::Baz<U>.qux`. Template
/// arguments are passed, not declared.
struct Id : Node {
  struct Element {
    /// One of `.`, `:` or `::`.
    const std::optional<Token::Punct> access_token;

    /// TODO: May be an expression instead.
    const Token::Id name_token;

    /// Variadic, i.e. indexed, template args, e.g. `<T, U>`.
    const std::vector<std::shared_ptr<Id>> vargs;

    /// Key-value, i.e. named, template args, e.g. `<T: U, V: W>`.
    const std::map<Token::Label, std::shared_ptr<Id>> kwargs;

    Element(
        std::optional<Token::Punct> access_token,
        Onyx::Token::Id name_token,
        std::vector<std::shared_ptr<Id>> vargs = {},
        std::map<Token::Label, std::shared_ptr<Id>> kwargs = {}) :
        access_token(access_token),
        name_token(name_token),
        vargs(vargs),
        kwargs(kwargs) {}

    Lang::Id::Element::Access access() const {
      if (access_token)
        switch (access_token.value().kind) {
        case Token::Punct::AccessStatic:
          return Lang::Id::Element::Static;
        case Token::Punct::AccessInstance:
          return Lang::Id::Element::Instance;
        case Token::Punct::AccessMember:
          return Lang::Id::Element::Member;
        default:
          __FNX__UNREACHEABLE("BUG");
        }
      else
        return Lang::Id::Element::Access::Self;
    }

    Placement placement() const;
  };

  using Elements = std::vector<std::shared_ptr<Element>>;

  const Elements elements;
  Id(Elements elements) : elements(elements) {}

  /// Return the last part name, e.g. `bar` for `Foo::bar<Baz>`.
  std::string name() const { return elements.back()->name_token.value; }

  /// Check if the first part begins with `::`.
  bool is_top_level() const {
    return elements.front()->access() == Lang::Id::Element::Static;
  }

  /// Return the whole ID node placement.
  Placement placement() const override {
    return Placement(
        elements.front()->name_token.placement.unit,
        {elements.front()->name_token.placement.location.start,
         elements.back()->name_token.placement.location.end});
  }

  Lang::Id to_lang_id() const {
    std::vector<Lang::Id::Element> elements;

    for (auto &element : this->elements) {
      std::vector<Lang::Id> vargs;
      for (auto &arg : element->vargs) {
        vargs.push_back(arg->to_lang_id());
      }

      std::unordered_map<std::string, Lang::Id> kwargs;
      for (auto &arg : element->kwargs) {
        kwargs.insert({arg.first.value, arg.second->to_lang_id()});
      }

      elements.push_back(Lang::Id::Element(
          element->name_token.value, element->access(), vargs, kwargs));
    }

    return Lang::Id(elements);
  }

  const char *node_name() const override { return "Id"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// An `import` node.
struct Import : Node {
  // Implemented variants:
  //
  //   * [x] `import T`
  //   * [x] `import * as T`
  //   * [x] `import { T }`
  //   * [x] `import { T as U }`
  //

  /// An optional `as` clause, e.g. `import { Foo as Bar }`.
  struct Alias {
    const Token::Punct as_keyword;
    const Token::Id alias_id;

    Alias(Token::Punct as_keyword, Token::Id alias_id) :
        as_keyword(as_keyword), alias_id(alias_id) {}
  };

  /// The imported element, e.g. `Foo` in `import Foo from "path"`.
  struct Element {
    /// An extracted element is non-default.
    const bool extracted;

    /// The punct token `*` is only allowed when not *extracted*,
    /// i.e. `import { * }` is illegal.
    std::variant<Token::Id, Token::Punct> id;

    /// The optional `as` clause, e.g. `import { Foo as Bar }`.
    const std::optional<Alias> alias;

    Element(
        bool extracted,
        std::variant<Token::Id, Token::Punct> id,
        std::optional<Alias> alias) :
        extracted(extracted), id(id), alias(alias) {}
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

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);

    o << "import ";

    bool wrapped = false;
    bool first = true;
    for (auto &element : elements) {
      // `import { a }, b, c, { d, e }` is legal (except `c`, but this is done
      // for example purposes).
      //

      bool need_comma = !first;

      if (first)
        first = false;

      if (element.extracted) {
        if (need_comma)
          o << ", ";

        if (!wrapped) {
          wrapped = true;
          o << "{ ";
        }
      } else {
        if (wrapped) {
          o << " }";
          wrapped = false;
        }

        if (need_comma)
          o << ", ";
      }

      std::visit([&o](auto token) { token.print(o); }, element.id);

      if (element.alias) {
        o << " as ";
        element.alias->alias_id.print(o);
      }
    }

    if (wrapped)
      o << " }";

    o << " from ";
    from_value.print(o);
  }
};

/// A freestanding `export` node.
struct Export : Node {
  // TODO:
};

/// A variable declaration node, also used as a runtime and template argument
/// declaration.
struct VarDecl : Node, Exportable, HasDocs {
  /// The declaration keyword token, e.g. `let`.
  const std::optional<Token::Keyword> keyword;

  /// The identifier token, e.g. `bar` in `foo: bar : T`.
  const Token::Id name;

  /// The optional alias label token, e.g. `foo` in `foo: bar : T`.
  const std::optional<Token::Label> alias;

  /// May be `nullptr`.
  const std::shared_ptr<TypeExpr> type_restriction;

  /// May be `nullptr`.
  const std::optional<RVal> value;

  VarDecl(
      std::shared_ptr<Comment> docs_node,
      std::optional<Token::Keyword> keyword,
      Token::Id name,
      std::optional<Token::Keyword> export_keyword = std::nullopt,
      std::optional<Token::Keyword> default_keyword = std::nullopt,
      std::optional<Token::Label> alias = std::nullopt,
      std::shared_ptr<TypeExpr> type_restriction = nullptr,
      std::optional<RVal> value = std::nullopt) :
      Exportable(export_keyword, default_keyword),
      HasDocs(docs_node),
      keyword(keyword),
      alias(alias),
      name(name),
      type_restriction(type_restriction),
      value(value) {}

  std::string alias_or_name() const {
    if (alias.has_value())
      return alias->value;
    else
      return name.value;
  }

  std::optional<Lang::Writeability> writeability() const {
    if (this->keyword.has_value()) {
      switch (this->keyword.value().kind) {
      case Token::Keyword::Let:
        return Lang::Writeability::Let;
      case Token::Keyword::Final:
        return Lang::Writeability::Final;
      default:
        __FNX__UNREACHEABLE("BUG")
      }
    } else
      return std::nullopt;
  }

  std::string writeability_str() const {
    if (writeability().has_value()) {
      switch (writeability().value()) {
      case Lang::Writeability::Let:
        return "let";
      case Lang::Writeability::Final:
        return "final";
      }
    } else {
      return "undefined";
    }
  }

  const char *node_name() const override { return "VarDecl"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A multiple args decl, either runtime (`(a, b)`) or template (`<T, U>`).
/// Also allows the greedy arg (`*`).
struct ArgsDecl {
  const std::vector<
      std::variant<std::shared_ptr<GreedyArg>, std::shared_ptr<VarDecl>>>
      args;

  /// Return the spanning placement from the first up to the last arg.
  Placement placement() const;

  void print(std::ostream &, unsigned indent = 0) const;
};

/// A function declaration, implementation or definition node.
/// TODO: Allow missing name for default exports.
struct FuncDecl : Node, Exportable, HasDocs {
  /// The optional `builtin` keyword.
  const std::optional<Token::Keyword> builtin_keyword;

  /// The optional action keyword, e.g. `decl`.
  const Token::Keyword action_keyword;

  /// The optional `function` keyword.
  const std::optional<Token::Keyword> function_keyword;

  /// The function ID node. It'd include template args for (re)impls.
  const std::shared_ptr<Id> id;

  /// Template argument declarations are legal for a function declaration or a
  /// function definition. For an implementation, template arguments passed
  /// would be a part of the ID.
  ///
  /// ```
  /// decl convert<Return: R, Value: V>(arg : V) : R # Is `template_args`
  /// def convert<Return: R, Value: V>(arg : V) : R  # Is `template_args`
  /// impl convert<Return: Float32>(arg : Int32) {}  # Part of the ID
  ///
  /// # Some edge case example:
  /// decl Int<Bitsize: 32>:convert<Return: R>(arg: Int<32>) : R
  /// #       ^                    ^ Is `template_args`
  /// #       | Part of the ID
  /// ```
  const std::optional<ArgsDecl> template_args_decl;

  /// A `forall` modifier is used in partial implementations.
  ///
  /// ```
  /// forall <T : Number> impl convert<Return: Float32>(arg : T) : Float32 {}
  /// ```
  const std::optional<ArgsDecl> forall;

  /// Runtime argument declarations.
  const ArgsDecl args;

  /// The optional return type node.
  const std::shared_ptr<TypeExpr> return_type;

  /// TODO: `def foo() -> T => expr()` (a single `Tuple::Element` ?)
  const std::shared_ptr<Block> body;

  FuncDecl(
      std::shared_ptr<Comment> docs_node,
      std::optional<Token::Keyword> builtin_keyword,
      Token::Keyword action_keyword,
      std::shared_ptr<Id> id,
      std::optional<Token::Keyword> export_keyword = std::nullopt,
      std::optional<Token::Keyword> default_keyword = std::nullopt,
      ArgsDecl args = {},
      std::shared_ptr<TypeExpr> return_type = nullptr,
      std::shared_ptr<Block> body = nullptr) :
      HasDocs(docs_node),
      Exportable(export_keyword, default_keyword),
      builtin_keyword(builtin_keyword),
      action_keyword(action_keyword),
      id(id),
      args(args),
      return_type(return_type),
      body(body) {}

  Lang::Action action() const {
    switch (action_keyword.kind) {
    case Token::Keyword::Decl:
      return Lang::Action::Decl;
    case Token::Keyword::Impl:
      return Lang::Action::Impl;
    case Token::Keyword::Def:
      return Lang::Action::Def;
    default:
      throw "BUG: Unexpected keyword";
    }
  }

  std::string name() const { return id->name(); }

  const char *node_name() const override { return "FuncDecl"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A type declaration node, such as struct.
struct TypeDecl : Node, Exportable, HasDocs {
  enum Kind { Trait, Builtin };

  /// The optional action token, e.g. `decl`.
  const std::optional<Token::Keyword> action_token;

  /// The optional kind token, e.g. `trait`.
  const std::optional<Token::Keyword> kind_token;

  /// The template arguments declaration, if any.
  const std::optional<ArgsDecl> template_args_decl;

  /// A `forall` modifier is used in partial implementations.
  ///
  /// ```
  /// forall T : Number impl Std::List<T> { }
  /// ```
  const std::optional<ArgsDecl> forall;

  /// The type ID node.
  const std::shared_ptr<Id> id;

  /// Would be `nullptr` for decls and no-body impls.
  const std::shared_ptr<Block> body;

  Lang::Action action() const {
    if (action_token) {
      switch (action_token.value().kind) {
      case Token::Keyword::Decl:
        return Lang::Action::Decl;
      case Token::Keyword::Impl:
        return Lang::Action::Impl;
      case Token::Keyword::Def:
        return Lang::Action::Def;
      case Token::Keyword::Reimpl:
        return Lang::Action::Reimpl;
      default:
        throw "BUG";
      }
    } else
      return Lang::Action::Def; // The implicit default
  }

  std::optional<Kind> kind() const {
    if (kind_token.has_value()) {
      switch (kind_token.value().kind) {
      case Token::Keyword::Trait:
        return Kind::Trait;
      case Token::Keyword::Builtin:
        return Kind::Builtin;
      default:
        throw "BUG";
      }
    } else
      return std::nullopt;
  }

  std::string kind_to_str() const {
    if (kind().has_value()) {
      switch (kind().value()) {
      case Kind::Trait:
        return "trait";
      case Kind::Builtin:
        return "builtin";
      }
    } else {
      throw "BUG: Unexpected empty kind";
    }
  }

  TypeDecl(
      std::shared_ptr<Comment> docs_node,
      std::optional<Token::Keyword> export_keyword,
      std::optional<Token::Keyword> default_keyword,
      std::optional<Token::Keyword> action_token,
      std::optional<Token::Keyword> kind_token,
      std::optional<ArgsDecl> template_args_decl,
      std::shared_ptr<Block> body = nullptr) :
      HasDocs(docs_node),
      Exportable(export_keyword, default_keyword),
      action_token(action_token),
      kind_token(kind_token),
      template_args_decl(template_args_decl),
      body(body) {}

  std::string name() const { return id->name(); }
  const char *node_name() const override { return "TypeDecl"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A tuple contains zero or more anonymous or labeled elements, i.e. an
/// anonymous struct. A single-element anonymous tuple *is* the element itself,
/// i.e. `Tuple<T> : T`, `(x) ≣ x`.
struct Tuple {
  using Element = Util::Variant::Flatten<std::variant<
      // std::shared_ptr<EmptyLine>,
      // Decl,
      RVal,
      BranchStatement,
      std::shared_ptr<ExplSafetyExpr>,
      std::shared_ptr<Block>>>;

  const std::vector<Element> indexed_elements;
  const std::unordered_map<Token::Label, Element> labeled_elements;

  Tuple(
      std::vector<Element> indexed_elements,
      std::unordered_map<Token::Label, Element> labeled_elements) :
      indexed_elements(indexed_elements), labeled_elements(labeled_elements) {}

  size_t size() const {
    return indexed_elements.size() + labeled_elements.size();
  }
};

/// A (r)untime tuple contains zero or more heterogeneous runtime elements,
/// e.g. `(1, "foo")`.
struct RTuple : Node {
  const std::pair<Token::Punct, Token::Punct> parens;
  Tuple tuple;

  const char *node_name() const override { return "RTuple"; }
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A (t)ype tuple contains zero or more heterogeneous type elements,
/// e.g. `<1, "foo">`.
struct TTuple : Node {
  const std::pair<Token::Punct, Token::Punct> brackets;
  Tuple tuple;

  const char *node_name() const override { return "TTuple"; }
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// An array contains zero or more anonymous homogeneous elements,
/// e.g. `[1, 2]`.
struct Array : Node {
  const std::pair<Token::Punct, Token::Punct> brackets;
  Tuple tuple;

  const char *node_name() const override { return "Array"; }
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A string literal node.
struct StringLiteral : Node {
  const Token::StringLiteral token;
  StringLiteral(Token::StringLiteral token) : token(token) {}

  const char *node_name() const override { return "StringLiteral"; }

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);
    token.print(o);
  }
};

/// A C string literal node.
struct CString : Node {
  const Token::CStringLiteral token;
  CString(Token::CStringLiteral token) : token(token) {}

  const char *node_name() const override { return "CString"; }
  void trace(std::ostream &) const override;

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);
    token.print(o);
  }
};

/// An boolean literal node.
struct Bool : Node {
  const Token::BoolLiteral token;
  Bool(Token::BoolLiteral token) : token(token) {}

  const char *node_name() const override { return "Bool"; }
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// An integer literal node.
struct Int : Node {
  const Token::IntLiteral token;
  Int(Token::IntLiteral token) : token(token) {}

  const char *node_name() const override { return "Int"; }
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A C identifier node.
struct CId : Node {
  const Token::CId token;
  CId(Token::CId token) : token(token) {}

  const char *node_name() const override { return "CId"; }
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A runtime assignment node, e.g. `x = 42`.
struct RuntimeAssign : Node {
  const std::shared_ptr<Id> lval;
  const RVal rval;

  RuntimeAssign(std::shared_ptr<Id> lval, RVal rval) : lval(lval), rval(rval) {}
};

/// A call node.
struct Call : Node {
  const Val callee;
  const RTuple args;
  // bool is_intrinsic; // TODO: `@foo`

  Call(Val callee, RTuple args) : callee(callee), args(args) {}

  bool is_c() const { return (std::get_if<std::shared_ptr<CId>>(&callee)); }

  const char *node_name() const override { return "Call"; }
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// An unary operation node.
struct UnOp : Node {
  const Token::Op the_operator;
  RVal operand;

  UnOp(Token::Op the_operator, RVal operand) :
      the_operator(the_operator), operand(operand) {}

  const char *node_name() const override { return "UnOp"; }

  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A binary operation node.
struct BinOp : Node {
  RVal left;
  const std::variant<Token::Op> the_operator;
  RVal right;

  BinOp(RVal left, Token::Op the_operator, RVal right) :
      left(move(left)), the_operator(the_operator), right(move(right)) {}

  const char *node_name() const override { return "BinOp"; }
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// An explicit safety expression.
struct ExplSafetyExpr : Node {
  /// E.g. `unsafe!`.
  const Token::Keyword safety_keyword;

  const RVal value;

  ExplSafetyExpr(Token::Keyword safety_keyword, RVal value) :
      safety_keyword(safety_keyword), value(value) {}

  std::optional<Lang::Safety> safety() const {
    switch (safety_keyword.kind) {
    case Token::Keyword::UnsafeBang:
      return Lang::Safety::Unsafe;
    case Token::Keyword::FragileBang:
      return Lang::Safety::Fragile;
    case Token::Keyword::ThreadsafeBang:
      return Lang::Safety::Threadsafe;
    default:
      __FNX__UNREACHEABLE("BUG")
    }
  }

  const char *node_name() const override { return "ExplSafetyExpr"; }
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// The context-dependent `*` thingie.
struct GreedyArg : Node {
  Token::Punct token;

  const char *node_name() const override { return "GreedyArg"; }

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);
    o << '*';
  }
};

/// A freestanding `alias` node.
struct Alias : Node {
  struct Source {
    const std::shared_ptr<Id> id;

    /// Would be `std::nullopt` if it's not a call alias.
    const std::optional<ArgsDecl> rargs;

    /// Would be `std::nullopt` if there is no template args decl.
    const std::optional<ArgsDecl> targs;
  };

  struct Target {
    const std::shared_ptr<Id> id;
    const std::shared_ptr<RTuple> targs;
    const std::shared_ptr<TTuple> rargs;
  };

  /// The `alias` keyword token.
  const Token::Keyword alias_keyword;

  /// The `=>` token.
  const Token::Punct assignment_token;

  const std::vector<Source> sources;
  const Target target;

  const char *node_name() const override { return "Alias"; }

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);

    o << "alias ";

    bool first = true;
    for (auto &source : sources) {
      if (first)
        first = false;
      else
        o << ", ";

      source.id->print(o);

      if (source.targs) {
        o << '<';
        source.targs->print(o);
        o << '>';
      }

      if (source.rargs) {
        o << '(';
        source.rargs->print(o);
        o << ')';
      }
    }

    o << " => ";

    target.id->print(o);

    if (target.targs) {
      o << '<';
      target.targs->print(o);
      o << '>';
    }

    if (target.rargs) {
      o << '(';
      target.rargs->print(o);
      o << ')';
    }
  }
};

/// A generic branch used in branch statements.
struct Branch {
  /// The optional branch delimiter keyword, which is only required for
  /// non-block bodies; otherwise it's optional.
  ///
  /// Clause    | Keyword
  /// ---       | ---
  /// `if`      | `then`
  /// `elif`    | `then`
  /// `case`    | `then`
  /// `else`    | `then`
  /// `while`   | `do`
  ///
  const std::optional<Token::Keyword> delimeter_keyword;

  const RVal body;

  Branch(std::optional<Token::Keyword> delimeter_keyword, RVal body) :
      delimeter_keyword(delimeter_keyword), body(body) {}

  void print(std::ostream &, unsigned indent) const;
};

/// A branch with a condition, beginning with either `if`, `elif` or `case`.
struct Case {
  /// Either `if`, `elif` or `case`.
  const Token::Keyword case_keyword;

  const RVal cond;
  const Branch branch;

  Case(Token::Keyword case_keyword, RVal cond, Branch branch) :
      case_keyword(case_keyword), cond(cond), branch(branch) {}

  void print(std::ostream &, unsigned indent = 0) const;
};

/// A branch without a condition, starting at an `else` keyword.
struct Else {
  const Token::Keyword else_keyword;
  const Branch branch;

  Else(Token::Keyword else_keyword, Branch branch) :
      else_keyword(else_keyword), branch(branch) {}

  void print(std::ostream &, unsigned indent = 0) const;
};

/// An `if` statement.
struct If : Node {
  const Case self;
  const std::vector<Case> elifs;
  const std::optional<Else> or_else;

  If(Case self, std::vector<Case> elifs, std::optional<Else> or_else) :
      self(self), elifs(elifs), or_else(or_else) {}

  const char *node_name() const override { return "If"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// @brief A `while` statement.
struct While : Node {
  const Token::Keyword while_keyword;
  const RVal cond;
  const Branch branch;

  While(Token::Keyword while_keyword, RVal cond, Branch branch) :
      while_keyword(while_keyword), cond(cond), branch(branch) {}

  const char *node_name() const override { return "While"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A `return` statement with an optional argument.
struct Return : Node {
  const Token::Keyword return_keyword;
  std::optional<RVal> value;

  Return(
      Token::Keyword return_keyword, std::optional<RVal> value = std::nullopt) :
      return_keyword(return_keyword), value(value) {}

  const char *node_name() const override { return "Return"; }
  void trace(std::ostream &) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A freestanding block of code wrapped in brackets.
struct Block : Node {
  using Node = Util::Variant::Flatten<std::variant<
      std::shared_ptr<Comment>,
      std::shared_ptr<EmptyLine>,
      Decl,
      RVal,
      Statement>>;

  const std::pair<Token::Punct, Token::Punct> brackets;

  /// `true` for `{\n...`, `false` for `{ ...`.
  const bool is_multiline;

  /// The nodes comprising the block.
  const std::vector<Node> nodes;

  Block(
      std::pair<Token::Punct, Token::Punct> brackets,
      bool is_multiline,
      const std::vector<Node> nodes) :
      brackets(brackets), is_multiline(is_multiline), nodes(nodes) {}

  const char *node_name() const override { return "Block"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void print(std::ostream &, unsigned indent = 0) const override;
};

/// A literal restriction node, e.g. `\bool`.
struct LiteralRestriction : Node {
  const Token::LiteralRestriction token;

  LiteralRestriction(Token::LiteralRestriction token) : token(token) {}

  Lang::LiteralRestriction kind() const { return token.kind; }
  const char *node_name() const override { return "LiteralRestriction"; }

  void print(std::ostream &o, unsigned indent = 0) const override {
    print_tab(o, indent);
    token.print(o);
  }
};

/// A type expression node.
struct TypeExpr : Node {
  using Value = std::variant<
      std::shared_ptr<LiteralRestriction>,
      BasicLiteral,
      std::shared_ptr<Id>>;

  const Value value;
  TypeExpr(Value value) : value(value) {}

  const char *node_name() const override { return "TypeExpr"; }
  void print(std::ostream &, unsigned indent = 0) const override;
};

struct Root : Node {
  /// The root node allows directives, but disallows values and control
  /// statements.
  using Node = Util::Variant::Flatten<std::variant<
      std::shared_ptr<EmptyLine>,
      Directive,
      Decl,
      Expr,
      BranchStatement,
      std::shared_ptr<ExplSafetyExpr>,
      std::shared_ptr<Alias>,
      std::shared_ptr<Block>>>;

  const std::vector<Node> &children() const { return _children; }
  void add_child(Node child) { _children.push_back(child); }

  const char *node_name() const override { return "CST"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void print(std::ostream &, unsigned indent = 0) const override;

private:
  std::vector<Node> _children;
};

}; // namespace CST

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
