#pragma once

#include "../parser.hh"
#include "./cst.hh"
#include "./lexer.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

struct Parser : NXC::Parser<Lexer, Token::Any> {
  using NXC::Parser<Lexer, Token::Any>::Parser;
  std::unique_ptr<CST> parse(bool single_expression);

private:
  std::shared_ptr<CST::TypeRef> _parse_type_ref();
  std::shared_ptr<CST::FuncDecl::Arg> _parse_func_decl_arg();

  bool _is_punct(Token::Punct::Kind kind) const {
    if (auto punct = _if<Token::Punct>())
      return punct->kind == kind;
    else
      return false;
  }

  bool _is_space() const { return _is_punct(Token::Punct::Space); }
  bool _is_comma() const { return _is_punct(Token::Punct::Comma); }
  bool _is_close_paren() const { return _is_punct(Token::Punct::CloseParen); }

  Token::Punct _as_punct(Token::Punct::Kind kind) const {
    if (auto punct = _if<Token::Punct>()) {
      if (punct->kind == kind)
        return punct.value();
    }

    throw _expected(Token::Punct::kind_to_expected(kind));
  }

  Token::Punct _as_open_paren() const {
    return _as_punct(Token::Punct::OpenParen);
  }

  Token::Punct _as_semi() const { return _as_punct(Token::Punct::Semi); }

  bool _is_op(const char *compared) const {
    if (auto op = _if<Token::Op>()) {
      return op->value == compared;
    }

    return false;
  }
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
