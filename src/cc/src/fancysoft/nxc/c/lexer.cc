#include <exception>
#include <sstream>

#include "fancysoft/nxc/c/lexer.hh"

namespace Fancysoft::NXC::C {

Util::Coro::Generator<Token::Any> Lexer::lex() noexcept {
  try {
    _advance();

    do {
      // End-of-line.
      if (_is_eol()) {
        while (_is_eol())
          _advance();

        co_yield _token<Token::Punct::EOL>();
        continue;
      }

      // A space.
      else if (_is_space()) {
        while (_is_space())
          _advance();

        co_yield _token<Token::Punct::Space>();
        continue;
      }

      // An identifier.
      else if (_is_latin_lowercase() || _is('_')) {
        std::stringbuf buff;

        while (_is_latin_lowercase() || _is('_') || _is_decimal()) {
          buff.sputc(_code_point);
          _advance();
        }

        const auto string = buff.str();

        co_yield _token<Token::Id>(string);
        continue;
      }

      // TODO: Needs much improvement.
      else if (_is_op()) {
        std::stringbuf buff;

        while (_is_op()) {
          buff.sputc(_code_point);
          _advance();
        }

        const auto string = buff.str();

        if (!string.compare("*")) {
          co_yield _token<Token::Op::Asterisk>();
          continue;
        } else {
          throw _unexpected("a C operator");
        }
      }

      else if (_is_punct()) {
        switch (_code_point) {
        case '(':
          co_yield _token<Token::Punct::OpenParen>();
          break;
        case ')':
          co_yield _token<Token::Punct::CloseParen>();
          break;
        case ';':
          co_yield _token<Token::Punct::Semi>();
          break;
        case ',':
          co_yield _token<Token::Punct::Comma>();
          break;
        default:
          throw std::exception(&"Unhandled punct char "[_code_point]);
        }

        _advance();
        continue;
      }

      else {
        throw _unexpected();
      }
    } while (!_is_eof());
  } catch (std::exception &e) {
    _exception = e;
    co_return;
  }
}

} // namespace Fancysoft::NXC::C
