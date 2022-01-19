#include <exception>

#include "fancysoft/nxc/onyx/lexer.hh"
#include "fancysoft/nxc/onyx/token.hh"

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

        co_yield _punct(Token::Punct::Space);
        continue;
      }

      // Either a keyword or an identifier.
      else if (_is_latin_alpha() || _is('_')) {
        std::stringbuf buf;

        while (_is_latin_alpha() || _is({'_', '!', '?'}) || _is_decimal()) {
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
        auto string = _lex_string_content('"');
        co_yield _token<Token::StringLiteral>(string);
        continue;
      }

      // An integer literal. Note that it sign would be lexed as an unop.
      else if (_is_decimal()) {
        auto _int = _lex_int();
        co_yield _token<Token::IntLiteral>(_int);
        continue;
      }

      // A C entity.
      else if (_is('$')) {
        _advance(); // Consume `$`

        if (_is('"')) {
          _advance(); // Consume opening `"`
          auto string = _lex_string_content('"');
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
      else if (Token::Op::is(_code_point)) {
        std::stringbuf buf;

        while (Token::Op::is(_code_point)) {
          buf.sputc(_code_point);
          _advance();
        }

        auto str = buf.str();

        // A freestanding angle breacket is considered a punctuation.
        if (!str.compare("<"))
          co_yield _punct(Token::Punct::OpenAngle);
        else if (!str.compare(">"))
          co_yield _punct(Token::Punct::CloseAngle);
        else
          co_yield _token<Token::Op>(str);

        continue;
      }

      // `::` (static access) is the only punctuation token consisting of
      // multiple codepoints. It thus shall be handled specifically.
      else if (_is(':')) {
        _advance();

        if (_is(':')) {
          _advance();
          co_yield _punct(Token::Punct::AccessStatic);
          continue;
        } else {
          co_yield _punct(Token::Punct::Colon);
          continue;
        }
      }

      // A punctuation token.
      else if (auto kind = Token::Punct::char_to_kind(_code_point)) {
        co_yield _punct(kind.value());
        _advance();
        continue;
      }

      else {
        throw _unexpected();
      }
    } while (!_is_eof());
  } catch (Panic &p) {
    _panic = p;
    co_return;
  } catch (std::exception &e) {
    _exception = e;
    co_return;
  }
};

std::string Lexer::_lex_string_content(char terminator) {
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

long long Lexer::_lex_int() {
  std::stringbuf buf;

  while (_is_decimal()) {
    buf.sputc(_code_point);
    _advance();
  }

  return atoll(buf.str().c_str());
};

} // namespace Fancysoft::NXC::Onyx
