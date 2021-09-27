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
#include "../safety.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

/// An Onyx AST.
struct AST : NXC::Node {
  struct ExternDirective;
  // struct ImportDirective;
  // struct ExportDirective;

  using Directive = std::variant<std::shared_ptr<ExternDirective>
                                 // std::shared_ptr<ImportDirective>,
                                 // std::shared_ptr<ExportDirective>
                                 >;

  struct VarDecl;
  // struct FunctionDecl;
  // struct NamespaceDecl;
  // struct InterfaceDecl;
  // struct StructDecl;
  // struct ClassDecl;
  // struct UnitDecl;
  // struct EnumDecl;

  using Declaration = std::variant<std::shared_ptr<VarDecl>
                                   // std::shared_ptr<FunctionDecl>,
                                   // std::shared_ptr<StructDecl>,
                                   // std::shared_ptr<ClassDecl>,
                                   // std::shared_ptr<NamespaceDecl>
                                   >;

  // struct StringLiteral;
  // struct CharLiteral;
  // struct NumberLiteral;
  // struct ArrayLiteral;
  // struct MapLiteral;
  // struct TupleLiteral;
  // struct VectorLiteral;
  // struct TensorLiteral;
  // struct MagicLiteral;

  struct CStringLiteral;

  using Literal = std::variant<
      // std::shared_ptr<StringLiteral>,
      std::shared_ptr<CStringLiteral>
      // std::shared_ptr<CharLiteral>,
      // std::shared_ptr<NumberLiteral>,
      // std::shared_ptr<ArrayLiteral>,
      // std::shared_ptr<MapLiteral>,
      // std::shared_ptr<TupleLiteral>,
      // std::shared_ptr<VectorLiteral>,
      // std::shared_ptr<TensorLiteral>,
      // std::shared_ptr<MagicLiteral>
      >;

  struct Id;
  struct CId;
  struct Call;
  struct CCall;
  struct UnOp;
  // struct BinOp;

  // struct IfStatement;
  // struct WhileStatement;
  // struct SwitchStatement;
  struct ExplicitSafetyStatement;

  // /// TODO: `return`, `break`, `continue`
  // struct FlowControlStatement;

  using Statement = std::variant<
      // std::shared_ptr<IfStatement>,
      // std::shared_ptr<WhileStatement>,
      // std::shared_ptr<SwitchStatement>,
      std::shared_ptr<ExplicitSafetyStatement>
      // std::shared_ptr<FlowControlStatement>
      >;

  // struct Block;
  struct TypeExpr;

  /// An expression may be a part of a function body.
  using Expr = Util::Variant::flatten_t<std::variant<
      Declaration,
      std::shared_ptr<Call>,
      std::shared_ptr<CCall>,
      std::shared_ptr<UnOp>,
      // std::shared_ptr<BinOp>,
      Statement>>;

  /// An rvalue can be assigned, e.g. `let foo = "bar"` (`"bar"` is rval).
  using RVal = Util::Variant::flatten_t<
      std::variant<Literal, std::shared_ptr<Id>, std::shared_ptr<CId>, Expr>>;

  using TopLevelNode = Util::Variant::flatten_t<std::variant<Directive, Expr>>;

  /// An `extern` directive node.
  struct ExternDirective : NXC::Node {
    /// The `extern` keyword token.
    const Token::Keyword keyword;

    /// The virtual C code block.
    std::shared_ptr<C::Block> block;

    ExternDirective(Token::Keyword keyword, std::shared_ptr<C::Block> block) :
        keyword(keyword), block(block) {}

    const char *node_name() const override { return "<ExternDirective>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
    std::string trace() const override { return node_name(); }
  };

  // struct Block {
  //   std::vector<Expr> exprs;
  //   // TODO: Block style: brackets, inline-brackets, indented...
  // };

  struct TypeExpr : NXC::Node {
    const char *node_name() const override { return "<TypeExpr>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
    std::string trace() const override { return node_name(); }
  };

  // /// A variable declaration node.
  // struct VarDecl : NXC::Node {
  //   const Token::Keyword keyword_token;
  //   const std::shared_ptr<TypeExpr> type_restriction;

  //   bool is_final() const {
  //     return this->keyword_token.kind == Token::Keyword::Final;
  //   }

  //   /// The variable identifier token, i.e. its name.
  //   const Token::Id id_token;

  //   VarDecl(Token::Keyword keyword_token, Token::Id id_token) :
  //       keyword_token(keyword_token), id_token(id_token) {}

  //   const char *node_name() const override { return "<VarDecl>"; }
  //   void inspect(std::ostream &, unsigned short indent = 0) const override;
  // };

  /// A variable declaration node.
  struct VarDecl : NXC::Node {
    const Token::Keyword keyword_token;
    const Token::Id id_token;
    const std::shared_ptr<TypeExpr> type_restriction;
    const std::optional<RVal> value;

    VarDecl(
        Token::Keyword keyword_token,
        Token::Id id_token,
        std::shared_ptr<TypeExpr> type_restriction = nullptr,
        std::optional<RVal> value = std::nullopt) :
        keyword_token(keyword_token),
        id_token(id_token),
        type_restriction(type_restriction),
        value(value) {}

    bool is_final() const {
      return this->keyword_token.kind == Token::Keyword::Final;
    }

    const char *node_name() const override { return "<VarDef>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;

    std::string trace() const override {
      return "<VarDef " + id_token.id + ">";
    }
  };

  // /// A string literal node.
  // struct StringLiteral : NXC::Node {
  //   const Token::StringLiteral token;
  //   StringLiteral(Token::StringLiteral token) : token(token) {}
  //   const char *node_name() const override { return "<StringLiteral>"; }
  //   void inspect(std::ostream &, unsigned short indent = 0) const override;
  // };

  /// A C string literal node.
  struct CStringLiteral : NXC::Node {
    const Token::CStringLiteral token;
    CStringLiteral(Token::CStringLiteral token) : token(token) {}
    const char *node_name() const override { return "<CStringLiteral>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;

    std::string trace() const override {
      return "<CStringLiteral \"" + token.string + "\">";
    };
  };

  /// An Onyx identifier node.
  ///
  /// TODO: Make it compound, e.g. `Foo<T>:bar`, also `int.to<Float32>()`.
  /// `Specialization`?
  ///
  /// TODO: Generic arg extraction node: `Array::<T>::<Bitsize>`.
  struct Id : NXC::Node {
    const Onyx::Token::Id token;

    Id(Onyx::Token::Id token) : token(token) {}

    const char *node_name() const override { return "<Id>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
    std::string trace() const override { return "<Id `" + token.id + "`>"; }
  };

  /// An C identifier node, e.g. `$void`.
  struct CId : NXC::Node {
    const Onyx::Token::CId token;

    CId(Onyx::Token::CId token) : token(token) {}

    const char *node_name() const override { return "<CId>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
    std::string trace() const override { return "<CId $`" + token.id + "`>"; }
  };

  /// An Onyx call node.
  struct Call : NXC::Node {
    const Onyx::Token::Id callee;
    std::vector<RVal> arguments;
    // bool is_intrinsic;

    Call(Onyx::Token::Id callee, std::vector<RVal> arguments) :
        callee(callee), arguments(arguments) {}

    const char *node_name() const override { return "<Call>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
    std::string trace() const override { return "<Call " + callee.id + "()>"; }
  };

  /// A C call node, e.g. `$exit()`,
  struct CCall : NXC::Node {
    const Onyx::Token::CId callee;
    std::vector<RVal> arguments;

    CCall(Onyx::Token::CId callee, std::vector<RVal> arguments) :
        callee(callee), arguments(arguments) {}

    const char *node_name() const override { return "<CCall>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;

    std::string trace() const override {
      return "<CCall $" + callee.id + "()>";
    }
  };

  /// An explicit safety statement node.
  struct ExplicitSafetyStatement : NXC::Node {
    const Token::Keyword keyword;

    Safety safety() const {
      switch (keyword.kind) {
      case Token::Keyword::UnsafeBang:
        return Safety::Unsafe;
      case Token::Keyword::FragileBang:
        return Safety::Fragile;
      case Token::Keyword::ThreadsafeBang:
        return Safety::Threadsafe;
      default:
        throw "Invalid safety keyword";
      };
    }

    RVal value;

    ExplicitSafetyStatement(Token::Keyword keyword, RVal value) :
        keyword(keyword), value(value) {}

    const char *node_name() const override { return "<ExplSafety>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;

    std::string trace() const override {
      return "<ExplSafety " + safety_name(safety()) + ">";
    }
  };

  /// An unary operation node.
  struct UnOp : NXC::Node {
    const Token::Op operator_;
    RVal operand;

    UnOp(Token::Op operator_, RVal operand) :
        operator_(operator_), operand(move(operand)) {}

    const char *node_name() const override { return "<UnOp>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
    std::string trace() const override { return node_name(); }
  };

  // /// A binary operation node.
  // struct BinOp : NXC::Node {
  //   RVal left_operand;
  //   const Token::Op operator_;
  //   RVal right_operand;

  //   BinOp(RVal left_operand, Token::Op operator_, RVal right_operand) :
  //       left_operand(move(left_operand)),
  //       operator_(operator_),
  //       right_operand(move(right_operand)) {}

  //   const char *node_name() const override { return "<BinOp>"; }
  //   void inspect(std::ostream &, unsigned short indent = 0) const override;
  // };

  const std::vector<TopLevelNode> &children() const { return _children; }
  void add_child(TopLevelNode child) { _children.push_back(child); }

  const char *node_name() const override { return "<AST>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
  std::string trace() const override { return node_name(); }

private:
  std::vector<TopLevelNode> _children;
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
