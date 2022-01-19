#pragma once

#include "../lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

struct Lexer : NXC::Lexer<Token::Any> {
  using NXC::Lexer<Token::Any>::Lexer;
  Util::Coro::Generator<Token::Any> lex() noexcept override;

private:
  // inline bool _is_op() const { return Token::Op::is(_code_point); }
  // inline bool _is_punct() const {
  //   return Token::Punct::char_to_kind(_code_point).has_value();
  // }
  auto _punct(Token::Punct::Kind kind) { return _token<Token::Punct>(kind); }
  std::string _lex_string_content(char terminator);
  long long _lex_int();
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
