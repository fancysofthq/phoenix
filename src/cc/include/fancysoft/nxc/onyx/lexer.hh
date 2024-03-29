#pragma once

#include "../lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

struct Lexer : NXC::Lexer<Token::Any> {
  using NXC::Lexer<Token::Any>::Lexer;
  Util::Coro::Generator<Token::Any> lex() noexcept override;

protected:
  inline const char *_debug_name() const override { return "Lexer"; }

private:
  inline bool _is_op() const {
    switch (_code_point) {
    case '=':
    case '~':
    case '-':
    case '+':
    case '!':
    case '?':
    case '&':
    case '*':
    case '%':
    case '^':
    case ':':
    case '/':
      return true;
    default:
      return false;
    }
  }

  inline bool _is_punct() const {
    switch (_code_point) {
    case ',':
    case '(':
    case ')':
      return true;
    default:
      return false;
    }
  }

  auto _punct(Token::Punct::Kind kind) { return _token<Token::Punct>(kind); }

  std::string _lex_string_literal_content(char terminator);
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
