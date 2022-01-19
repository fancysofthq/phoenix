#pragma once

#include "../lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Onyx {

struct Lexer : Phoenix::Lexer<Token::Any> {
  using Phoenix::Lexer<Token::Any>::Lexer;
  Util::Coro::Generator<Token::Any> lex() noexcept override;

private:
  /// A shortcut to return a punctuation token.
  auto _punct(Token::Punct::Kind kind) { return _token<Token::Punct>(kind); }

  /// Lex a string literal value, terminated with *terminator*.
  std::string _lex_string(char terminator);

  /// Lex an integer literal value.
  long long _lex_int();

  inline bool _is_c_id() {
    return _is_latin_alpha() || _is_decimal() || _is('_');
  }
};

} // namespace Onyx
} // namespace Phoenix
} // namespace Fancysoft
