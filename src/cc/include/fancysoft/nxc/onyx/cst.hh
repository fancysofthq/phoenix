#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <fmt/core.h>

#include "../../util/variant.hh"
#include "../c/block.hh"
#include "../node.hh"
#include "./safety.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// An Onyx CST.
namespace CST {

namespace Node {
struct Extern;
struct StringLiteral;
struct Id;
struct CCall;
struct VarDecl;
struct UnOp;
struct ExplicitSafety;

using Expression =
    std::variant<StringLiteral, CCall, ExplicitSafety, VarDecl, UnOp, Id>;
} // namespace Node

struct Node::Extern : NXC::Node {
  /// The `extern` keyword token.
  const Token::Keyword::Extern keyword;

  /// The C virtual code block containing the C CST.
  const std::shared_ptr<C::Block> block;

  Extern(Token::Keyword::Extern keyword, std::shared_ptr<C::Block> block) :
      keyword(keyword), block(block) {}

  const char *node_name() const override { return "<Extern>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

struct Node::StringLiteral : NXC::Node {
  const Token::StringLiteral token;
  StringLiteral(Token::StringLiteral token) : token(token) {}
  const char *node_name() const override { return "<StringLiteral>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

struct Node::CCall : NXC::Node {
  const Token::CId callee_id;
  const std::vector<std::shared_ptr<Expression>> arguments;

  CCall(
      const Token::CId callee_id,
      std::vector<std::shared_ptr<Expression>> arguments) :
      callee_id(callee_id), arguments(arguments) {}

  const char *node_name() const override { return "<CCall>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

struct Node::ExplicitSafety : NXC::Node {
  using KeywordToken =
      std::variant<Token::Keyword::UnsafeBang, Token::Keyword::FragileBang>;

  const KeywordToken keyword;

  Safety safety() const { return Safety::Unsafe; }

  enum Kind {
    /// Contains a single expression.
    Inline,
  };

  inline bool is_inline() const { return this->kind == Kind::Inline; }

  const Kind kind;

  const std::vector<std::shared_ptr<Expression>> expressions;

  explicit ExplicitSafety(
      KeywordToken keyword,
      Kind kind,
      const std::vector<std::shared_ptr<Expression>> expressions) :
      keyword(keyword), kind(kind), expressions(expressions) {}

  const char *node_name() const override { return "<ExplSafety>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

struct Node::VarDecl : NXC::Node {
  /// Either `let` or `final`.
  using KeywordToken = std::variant<Token::Keyword::Let, Token::Keyword::Final>;

  const KeywordToken keyword;

  inline bool is_final() const {
    return this->keyword.index() ==
           Util::Variant::index<KeywordToken, Token::Keyword::Final>();
  }

  inline bool is_let() const { return !is_final(); }

  /// The variable identifier, i.e. its name.
  const Token::Id id;

  /// The optional defined value, e.g. `let foo = "bar"`.
  const std::shared_ptr<Expression> value;

  explicit VarDecl(
      KeywordToken keyword, Token::Id id, std::shared_ptr<Expression> value) :
      keyword(keyword), id(id), value(value) {}

  const char *node_name() const override { return "<VarDecl>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

/// An unary operation node.
struct Node::UnOp : NXC::Node {
  using OperatorType = std::variant<Token::BuiltInOp::AddressOf, Token::Op>;

  const OperatorType operator_;
  const std::shared_ptr<Expression> operand;

  UnOp(OperatorType operator_, std::shared_ptr<Expression> operand) :
      operator_(operator_), operand(operand) {}

  const char *node_name() const override { return "<UnOp>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

/// A simple identifier node.
struct Node::Id : NXC::Node {
  const Token::Id id;
  Id(Token::Id id) : id(id) {}

  const char *node_name() const override { return "<Id>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

using RootChildNode =
    Util::Variant::flatten_t<std::variant<Node::Extern, Node::Expression>>;

struct Root : NXC::Node {
  std::vector<std::shared_ptr<RootChildNode>> children;

  const char *node_name() const override { return "<Root>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

}; // namespace CST

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
