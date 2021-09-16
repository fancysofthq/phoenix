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

      // Newline.
      else if (_is_newline()) {
        while (_is_newline())
          _advance();

        co_yield _punct(Token::Punct::Newline);
        continue;
      }

      // A horizontal space.
      else if (_is_space()) {
        while (_is_space())
          _advance();

        co_yield _punct(Token::Punct::HSpace);
        continue;
      }

      // Either a keyword or an identifier.
      else if (_is_latin_lowercase() || _is('_')) {
        std::stringbuf buf;

        while (_is_latin_lowercase() || _is({'_', '!', '?'}) || _is_decimal()) {
          buf.sputc(_code_point);
          _advance();
        }

        const auto string = buf.str();
        auto keyword_kind = Token::Keyword::parse_kind(string);

        if (keyword_kind.has_value())
          co_yield _token<Token::Keyword>(keyword_kind.value());
        else
          co_yield _token<Token::Id>(string);

        continue;
      }

      // A string literal.
      else if (_is('"')) {
        _advance(); // Consume opening `"`
        auto string = _lex_string_literal_content('"');
        co_yield _token<Token::StringLiteral>(string);
        continue;
      }

      // A C entity.
      else if (_is('$')) {
        _advance(); // Consume `$`

        if (_is('"')) {
          _advance(); // Consume opening `"`
          auto string = _lex_string_literal_content('"');
          co_yield _token<Token::CStringLiteral>(string);
          continue;
        } else {
          std::stringbuf buf;

          while (_is_latin_alpha() || _is_decimal() || _is('_')) {
            buf.sputc(_code_point);
            _advance();
          }

          co_yield _token<Token::CId>(buf.str());
          continue;
        }
      }

      // An operator.
      else if (_is_op()) {
        std::stringbuf buf;

        while (_is_op()) {
          buf.sputc(_code_point);
          _advance();
        }

        co_yield _token<Token::Op>(buf.str());
        continue;
      }

      // A punctuation token.
      else {
        switch (_code_point) {
        case ',':
          co_yield _punct(Token::Punct::Comma);
          break;
        case '(':
          co_yield _punct(Token::Punct::OpenParen);
          break;
        case ')':
          co_yield _punct(Token::Punct::CloseParen);
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
};

std::string Lexer::_lex_string_literal_content(char terminator) {
  std::stringbuf buf;
  bool escaped = false;

  while (!(_is(terminator) && !escaped)) {
    buf.sputc(_code_point);
    _advance();
    escaped = _is('\\');
  }

  _advance(); // Consume closing `"`
  return buf.str();
}

} // namespace Fancysoft::NXC::Onyx
