#include "fancysoft/nxc/c/lexer.hh"

namespace Fancysoft::NXC::C {

Util::Coro::Generator<Token::Any> Lexer::lex() noexcept {
  try {
    _advance();

    do {
      if (_is_newline()) {
        while (_is_newline())
          _advance();

        co_yield _punct(Token::Punct::Newline);
        continue;
      }

      else if (_is_space()) {
        while (_is_space())
          _advance();

        co_yield _punct(Token::Punct::HSpace);
        continue;
      }

      // An identifier.
      else if (_is_latin_lowercase() || _is('_')) {
        std::stringbuf buf;

        while (_is_latin_lowercase() || _is('_') || _is_decimal()) {
          buf.sputc(_code_point);
          _advance();
        }

        const auto string = buf.str();

        co_yield _token<Token::Id>(string);
        continue;
      }

      // TODO: The list of C operators is well-known.
      else if (_is_op()) {
        std::stringbuf buf;

        while (_is_op()) {
          buf.sputc(_code_point);
          _advance();
        }

        const auto string = buf.str();

        co_yield _token<Token::Op>(string);
        continue;
      }

      else {
        switch (_code_point) {
        case '(':
          co_yield _punct(Token::Punct::OpenParen);
          break;
        case ')':
          co_yield _punct(Token::Punct::CloseParen);
          break;
        case ';':
          co_yield _punct(Token::Punct::Semi);
          break;
        case ',':
          co_yield _punct(Token::Punct::Comma);
          break;
        default:
          throw _unexpected();
        }

        _advance();
        continue;
      }
    } while (!_is_eof());
  } catch (std::exception &e) {
    _exception = e;
    co_return;
  }
}

} // namespace Fancysoft::NXC::C
