#pragma once

#include "../lexer.hh"
#include "./block.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

struct Lexer : NXC::Lexer<Token::Any> {
  using NXC::Lexer<Token::Any>::Lexer;
  Util::Coro::Generator<Token::Any> lex() noexcept override;

protected:
  inline const char *_debug_name() const override { return "C/Lexer"; }

private:
  inline bool _is_op() const {
    switch (_code_point) {
    case '=':
    case '~':
    case '+':
    case '-':
    case '&':
    case '*':
    case '%':
    case '^':
    case '/':
      return true;
    default:
      return false;
    }
  }

  inline bool _is_punct() const {
    switch (_code_point) {
    case '(':
    case ')':
    case ';':
    case ',':
      return true;
    default:
      return false;
    }
  }
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
