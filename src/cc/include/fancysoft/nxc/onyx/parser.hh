#pragma once

#include <exception>
#include <istream>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <variant>

#include "../parser.hh"
#include "./cst.hh"
#include "./lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

struct Parser : NXC::Parser<Lexer, Token::Any> {
  using NXC::Parser<Lexer, Token::Any>::Parser;
  std::unique_ptr<CST> parse();

private:
  /// See `_parse_expr()`.
  struct _ExpectedExpression : std::runtime_error {
    _ExpectedExpression() : std::runtime_error("Expected expression") {}
  };

  /// See `_parse_rval()`.
  struct _ExpectedRVal : std::runtime_error {
    _ExpectedRVal() : std::runtime_error("Expected rval") {}
  };

  /// Parse an `extern` directive, starting at the `extern` keyword.
  CST::Extern _parse_extern();

  /// Try parsing any directive.
  std::optional<CST::Directive> _try_parse_directive();

  /// Parse a variable declaration. `let` or `final` keyword may be omitted if
  /// *require_keyword* is `false` (useful for arg declarations).
  CST::VarDecl _parse_var_decl(bool require_keyword);

  /// Parse a function declaration.
  CST::FuncDecl _parse_func_decl();

  /// Try parsing any declaration.
  std::optional<CST::Decl> _try_parse_decl();

  /// Parse a tuple, starting at the opening parenthesis.
  CST::Tuple _parse_tuple();

  /// Parse an ID path element.
  CST::Id::PathElement _parse_id_path_element(CST::Id::PathElement::Access);

  /// Parse a full-path ID. Omitting the latest template arguments is useful for
  /// declarations, where the arguments shall be handled appropriately.
  CST::Id _parse_id(bool omit_latest_template_args);

  /// Try parsing a value.
  std::optional<CST::Val> _try_parse_val();

  /// Parse an unary operation, starting at the operator token.
  /// NOTE: A freestanding angle bracket can not be an unary operator.
  CST::UnOp _parse_unop();

  /// Parse a binary operation, starting at the operator token.
  ///
  /// NOTE: A freestanding angle bracket (i.e. a punctuation token) may be a
  /// binary operator, thus shall be handled specifically.
  CST::BinOp _parse_binop(CST::RVal lval);

  /// Parse an expression.
  ///
  /// An encountered ID may be a freestanding ID, i.e. neither a callee nor
  /// operand. We could roll the parser back to cancel the ID parsing. However,
  /// this would increase the parser complexity. Instead, if the ID is really a
  /// freestanding ID, i.e. not an expression, `_ExpectedExpression` is thrown.
  /// This allows to handle the "not an expression" case on the caller side.
  /// Once again, the ID is already parsed, thus lost.
  // CST::Expr _parse_expr();

  /// Parse an rvalue. Throws `_ExpectedRVal` upon failure.
  /// See `_parse_expr()` for details.

  /// Try parsing an rvalue, i.e. a value or an expression.
  std::optional<CST::RVal> _try_parse_rval();

  /// Parse an rvalue, throwing "expected rvalue" upon failure.
  CST::RVal _parse_rval();

  /// Try parsing an explicit safety statement.
  std::optional<CST::ExplSafety> _try_parse_expl_safety();

  /// Parse a case branch with a condition, starting at the case keyword.
  CST::Case _parse_case();

  /// Parse an else branch, starting at the `else` keyword.
  CST::Else _parse_else();

  /// Try parsing an `if` statement, starting at the `if` keyword.
  std::optional<CST::If> _try_parse_if();

  /// Try parsing a `while` statement, starting at the `while` keyword.
  std::optional<CST::While> _try_parse_while();

  /// Try parsing any branch statement.
  std::optional<CST::BranchStatement> _try_parse_branch_statement();

  /// Try parsing a `return` statement.
  std::optional<CST::Return> _try_parse_return();

  /// Try parsing any control statement.
  std::optional<CST::ControlStatement> _try_parse_control_statement();

  /// Try parsing any statement.
  std::optional<CST::Statement> _try_parse_statement();

  /// Parse a block. If one of *extra_terminators* is encountered, it wouldn't
  /// become a part of the block, unlike `end`. Sometimes, the block's `do` has
  /// already been parsed, passed as *explicit_do*.
  CST::Block _parse_block(
      std::optional<Token::Keyword> explicit_do = std::nullopt,
      std::set<Token::Keyword::Kind> extra_terminators = {});

  /// Try parsing a generic block which may begin with either an opening bracket
  /// or a `do` keyword.
  std::optional<CST::Block> _try_parse_block();

  /// Parse an element node.
  CST::Element _parse_element();

  /// Parse a top-level node.
  CST::TopLevelNode _parse_top_level_node();

  /// Parse a type expression, starting at the expression.
  CST::TypeExpr _parse_type_expr();

  /// Skip horizontal space tokens, if any.
  void _skip_space();

  /// Skip any amount of horizontal space and newline tokens.
  void _skip_spaces_and_newlines();

  /// Skip any amount of horizontal spaces and at most one newline token.
  void _skip_spaces_and_single_newline();

  using NXC::Parser<Lexer, Token::Any>::_is;

  bool _is(Token::Punct::Kind);
  bool _is_space() { return _is(Token::Punct::Space); }
  bool _is_newline() { return _is(Token::Punct::Newline); }
  bool _is_open_paren() { return _is(Token::Punct::OpenParen); }
  bool _is_close_paren() { return _is(Token::Punct::CloseParen); }
  bool _is_open_bracket() { return _is(Token::Punct::OpenBracket); }
  bool _is_close_bracket() { return _is(Token::Punct::CloseBracket); }
  bool _is_open_angle() { return _is(Token::Punct::OpenAngle); }
  bool _is_close_angle() { return _is(Token::Punct::CloseAngle); }
  bool _is_comma() { return _is(Token::Punct::Comma); }
  bool _is_colon() { return _is(Token::Punct::Colon); }
  bool _is_semi() { return _is(Token::Punct::Semi); }

  /// Check if the token is terminator (newline or semi).
  bool _is_term() { return _is_newline() || _is_semi(); }

  // /// Expect the token to be a specific punct token, throw otherwise.
  // Token::Punct _as_punct(Token::Punct::Kind);

  bool _is(Token::Keyword::Kind);
  bool _is(std::set<Token::Keyword::Kind>);
  bool _is_end() { return _is(Token::Keyword::End); }
  bool _is_do() { return _is(Token::Keyword::Do); }

  std::optional<Token::Keyword> _if(Token::Keyword::Kind);
  std::optional<Token::Keyword> _if(std::set<Token::Keyword::Kind>);

  /// Check if the token is `Token::Op`.
  /// NOTE: Some punctuation tokens may be operators, e.g. `<`.
  bool _is_op() { return _is<Token::Op>(); }

  /// Check if the op token is a specific operator (e.g. `"="`).
  bool _is_op(std::string op);

  bool _is_id() { return _is<Token::Id>(); }

  using NXC::Parser<Lexer, Token::Any>::_if;
  using NXC::Parser<Lexer, Token::Any>::_consume;

  Token::Punct _consume(Token::Punct::Kind kind);
  Token::Punct _consume_space() { return _consume(Token::Punct::Space); }
  Token::Punct _consume_colon() { return _consume(Token::Punct::Colon); }
  Token::Punct _consume_comma() { return _consume(Token::Punct::Comma); }

  Token::Punct _consume_open_paren() {
    return _consume(Token::Punct::OpenParen);
  }

  Token::Punct _consume_term() {
    if (_is_term())
      return _consume<Token::Punct>();
    else
      throw _expected("terminator");
  }

  Token::Keyword _consume(Token::Keyword::Kind kind);
  Token::Keyword _consume_return() { return _consume(Token::Keyword::Return); }

  using NXC::Parser<Lexer, Token::Any>::_expected;

  Panic _expected(Token::Punct::Kind kind) const {
    return Parser::_expected(Token::Punct::kind_to_str(kind));
  }
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
