#pragma once

#include <istream>
#include <optional>
#include <set>
#include <variant>

#include "../parser.hh"
#include "./ast.hh"
#include "./lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

struct Parser : NXC::Parser<Lexer, Token::Any> {
  using NXC::Parser<Lexer, Token::Any>::Parser;
  std::unique_ptr<AST> parse();

protected:
  inline const char *_debug_name() const override { return "Parser"; }

private:
  AST::RVal _parse_rval();
  AST::Expr _parse_expr();

  /// Skip horizontal space tokens.
  void _skip_space();

  /// Skip horizontal space and newline tokens.
  void _skip_space_newline();

  bool _is_punct(Token::Punct::Kind);
  bool _is_punct(std::set<Token::Punct::Kind>);
  bool _is_space() { return _is_punct(Token::Punct::HSpace); }
  bool _is_newline() { return _is_punct(Token::Punct::Newline); }
  bool _is_open_paren() { return _is_punct(Token::Punct::OpenParen); }
  bool _is_close_paren() { return _is_punct(Token::Punct::CloseParen); }
  bool _is_comma() { return _is_punct(Token::Punct::Comma); }

  /// Expect the token to be a specific punct token, throw otherwise.
  Token::Punct _as_punct(Token::Punct::Kind);

  bool _is_keyword(Token::Keyword::Kind);
  bool _is_keyword(std::set<Token::Keyword::Kind>);

  std::optional<Token::Keyword> _if_keyword(Token::Keyword::Kind);
  std::optional<Token::Keyword> _if_keyword(std::set<Token::Keyword::Kind>);

  /// Check if the token is specific operator (e.g. `"="`).
  bool _is_op(std::string op);
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
