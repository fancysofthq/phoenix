#include "fancysoft/phoenix/onyx/lexer.hh"
#include "fancysoft/phoenix/exception.hh"

namespace Fancysoft::Phoenix::Onyx {

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

      // Either a keyword or an unwrapped identifier, e.g. `foo`.
      else if (_is_latin_alpha() || _is('_')) {
        std::stringbuf buf;

        while (_is_latin_alpha() || _is({'_', '!', '?'}) || _is_decimal()) {
          buf.sputc(_code_point);
          _advance();
        }

        const auto string = buf.str();
        auto keyword_kind = Token::Keyword::parse_kind(string);

        if (keyword_kind.has_value()) {
          co_yield _token<Token::Keyword>(keyword_kind.value());
        } else if (_is(':')) {
          _advance();
          co_yield _token<Token::Label>(false, string);
        } else {
          co_yield _token<Token::Id>(false, string);
        }

        continue;
      }

      /// A wrapped identifer, e.g. `` `фу` ``.
      else if (_is('`')) {
        _advance(); // Consume opening '`'
        std::stringbuf buf;

        while (!_is('`')) {
          buf.sputc(_code_point);
          _advance();
        }

        _advance(); // Consume closing '`'
        const auto string = buf.str();

        if (_is(':')) {
          _advance();
          co_yield _token<Token::Label>(true, string);
        } else {
          co_yield _token<Token::Id>(true, string);
        }

        continue;
      }

      // A string literal.
      else if (_is('"')) {
        _advance(); // Consume the opening `"`
        auto string = _lex_string('"');
        co_yield _token<Token::StringLiteral>(string);
        continue;
      }

      // An integer literal. Note that a sign would be lexed as an unop.
      else if (_is_decimal()) {
        auto _int = _lex_int();
        co_yield _token<Token::IntLiteral>(_int);
        continue;
      }

      // A C entity.
      else if (_is('$')) {
        _advance(); // Consume `$`

        if (_is('`')) {
          // A wrapped C ID, e.g. `` $`unsigned int` ``.
          //

          _advance(); // Consume opening '`'
          std::stringbuf buf;

          while (_is_c_id() || _is_space()) {
            buf.sputc(_code_point);
            _advance();
          }

          _advance(); // Consume closing '`'

          co_yield _token<Token::CId>(true, buf.str());
          continue;
        } else if (_is_latin_alpha() || _is('_')) {
          // An unwrapped C id, e.g. `$int`.
          //

          std::stringbuf buf;

          while (_is_c_id()) {
            buf.sputc(_code_point);
            _advance();
          }

          co_yield _token<Token::CId>(false, buf.str());
          continue;
        } else {
          // TODO: C literals.
          throw std::runtime_error("Unimplemented");
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

        // Some chars are considered a punctuation rather than an operator.
        // TODO: One-or-more asterisk, `!`.
        if (!str.compare("<"))
          co_yield _punct(Token::Punct::AngleOpen);
        else if (!str.compare(">"))
          co_yield _punct(Token::Punct::AngleClose);
        else
          co_yield _token<Token::Op>(str);

        continue;
      }

      // `::` (static scope access) is the only punctuation token consisting of
      // multiple codepoints. It thus shall be handled specifically.
      else if (_is(':')) {
        _advance();

        if (_is(':')) {
          _advance();
          co_yield _punct(Token::Punct::ScopeStatic);
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

std::string Lexer::_lex_string(char terminator) {
  std::stringbuf buf;
  bool escaped = false;

  while (!(_is(terminator) && !escaped)) {
    buf.sputc(_code_point);
    _advance();
    escaped = _is('\\');
  }

  _advance(); // Consume the terminator
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

} // namespace Fancysoft::Phoenix::Onyx
