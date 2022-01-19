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

private:
  /// Check if the code point could be a part of an operator token. Note that an
  /// operator may consist of multiple code points (e.g. `+=`).
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

  auto _punct(Token::Punct::Kind kind) { return _token<Token::Punct>(kind); }
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
