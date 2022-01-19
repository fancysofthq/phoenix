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

        co_yield _punct(Token::Punct::Space);
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
          _advance();
          continue;
        case ')':
          co_yield _punct(Token::Punct::CloseParen);
          _advance();
          continue;
        case ';':
          co_yield _punct(Token::Punct::Semi);
          _advance();
          continue;
        case ',':
          co_yield _punct(Token::Punct::Comma);
          _advance();
          continue;
        case '.': {
          size_t count = 0;

          while (_is('.')) {
            count += 1;
            _advance();
          }

          switch (count) {
          case 1:
            co_yield _punct(Token::Punct::Access);
            continue;
          case 3:
            co_yield _punct(Token::Punct::Varg);
            continue;
          }
        }
        }

        throw _unexpected();
      }
    } while (!_is_eof());
  } catch (std::exception &e) {
    _exception = e;
    co_return;
  }
}

} // namespace Fancysoft::NXC::C
