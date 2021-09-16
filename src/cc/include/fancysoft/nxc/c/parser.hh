#pragma once

#include "../parser.hh"
#include "./ast.hh"
#include "./lexer.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

struct Parser : NXC::Parser<Lexer, Token::Any> {
  using NXC::Parser<Lexer, Token::Any>::Parser;
  std::unique_ptr<AST> parse(bool single_expression);

protected:
  inline const char *_debug_name() const override { return "C/Parser"; }

private:
  std::shared_ptr<AST::TypeRef> _parse_type_ref();
  std::shared_ptr<AST::FuncDecl::ArgDecl> _parse_arg_decl();

  bool _is_punct(Token::Punct::Kind kind) const {
    if (auto punct = _if<Token::Punct>())
      return punct->kind == kind;
    else
      return false;
  }

  bool _is_space() const { return _is_punct(Token::Punct::HSpace); }
  bool _is_comma() const { return _is_punct(Token::Punct::Comma); }
  bool _is_close_paren() const { return _is_punct(Token::Punct::CloseParen); }

  Token::Punct _as_punct(Token::Punct::Kind kind) const {
    if (auto punct = _if<Token::Punct>()) {
      if (punct->kind == kind)
        return punct.value();
    }

    throw _unexpected(Token::Punct::kind_to_expected(kind));
  }

  Token::Punct _as_open_paren() const {
    return _as_punct(Token::Punct::OpenParen);
  }

  Token::Punct _as_semi() const { return _as_punct(Token::Punct::Semi); }

  bool _is_op(const char *compared) const {
    if (auto op = _if<Token::Op>()) {
      return op->op == compared;
    }

    return false;
  }
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
