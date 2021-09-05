#include <exception>
#include <sstream>

#include "fancysoft/nxc/onyx/lexer.hh"

namespace Fancysoft::NXC::Onyx {

Util::Coro::Generator<Token::Any> Lexer::lex() noexcept {
  try {
    _advance();

    do {
      // End-of-file.
      if (_is_eof()) {
        co_return;
      }

      // End-of-line.
      else if (_is_eol()) {
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

      // Either a keyword or an identifier.
      else if (_is_latin_lowercase() || _is('_')) {
        std::stringbuf buff;

        while (_is_latin_lowercase() || _is({'_', '!', '?'}) || _is_decimal()) {
          buff.sputc(_code_point);
          _advance();
        }

        const auto string = buff.str();

        // TODO: Move this logic to "token.hh"`.
        if (!string.compare("extern")) {
          co_yield _token<Token::Keyword::Extern>();
          continue;
        } else if (!string.compare("let")) {
          co_yield _token<Token::Keyword::Let>();
          continue;
        } else if (!string.compare("final")) {
          co_yield _token<Token::Keyword::Final>();
          continue;
        } else if (!string.compare("unsafe!")) {
          co_yield _token<Token::Keyword::UnsafeBang>();
          continue;
        } else {
          co_yield _token<Token::Id>(string);
          continue;
        }
      }

      // Either a well-known or generic operator.
      else if (_is_op()) {
        std::stringbuf buff;

        while (_is_op()) {
          buff.sputc(_code_point);
          _advance();
        }

        const auto string = buff.str();

        if (!string.compare("=")) {
          co_yield _token<Token::BuiltInOp::Assign>();
          continue;
        } else if (!string.compare("&")) {
          co_yield _token<Token::BuiltInOp::AddressOf>();
          continue;
        } else {
          co_yield _token<Token::Op>(string);
          continue;
        }
      }

      else if (_is_punct()) {
        switch (_code_point) {
        case ',':
          co_yield _token<Token::Punct::Comma>();
          break;
        case '(':
          co_yield _token<Token::Punct::OpenParen>();
          break;
        case ')':
          co_yield _token<Token::Punct::CloseParen>();
          break;
        default:
          throw std::exception("Unhandled case");
        }

        _advance();
      }

      // A string literal.
      else if (_is('"')) {
        _advance(); // Consume opening `"`

        std::stringbuf buff;
        bool escaped = false;

        while (!(_is('"') && !escaped)) {
          buff.sputc(_code_point);
          _advance();
          escaped = _is('\\');
        }
        _advance(); // Consume closing `"`

        co_yield _token<Token::StringLiteral>(buff.str());
        continue;
      }

      // A C identifier.
      else if (_is('$')) {
        _advance(); // Consume `$`

        std::stringbuf buff;

        while (_is_latin_alpha() || _is_decimal() || _is('_')) {
          buff.sputc(_code_point);
          _advance();
        }

        co_yield _token<Token::CId>(buff.str());
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
};

} // namespace Fancysoft::NXC::Onyx
