#pragma once

#include <exception>
#include <iostream>
#include <istream>
#include <memory>
#include <optional>
#include <set>

#include <fancysoft/util/coro.hh>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "./exception.hh"
#include "./logger.hh"
#include "./radix.hh"
#include "./unit.hh"

namespace Fancysoft {
namespace NXC {

/// An abstract lexer yielding *Token* instance copies.
template <typename Token> struct Lexer {
  /// The unit being lexed.
  const std::shared_ptr<Unit> unit;

  Lexer(std::shared_ptr<Unit> unit, std::shared_ptr<Logger> logger) :
      unit(unit), _logger(logger) {}

  /// Begin the lexing, creating a resumable coroutine.
  virtual Util::Coro::Generator<Token> lex() = 0;

  /// Get the current cursor position within the unit.
  ///
  /// Note that the cursor not strictly tied to any stream, it is only
  /// used to mark a token's position within the unit.
  inline Position cursor() const { return _cursor; }

  /// Offset the `cursor()` *by*, returning its new position.
  Position offset(Position by) {
    _cursor = _cursor + by;
    return _cursor;
  }

  /// Check if the lexer has thrown a panic.
  std::optional<Panic> panic() { return _panic; }

  /// Check if the lexer has thrown an exception.
  std::optional<std::exception> exception() { return _exception; }

  void _unread() { unit->source_stream().unget(); }

private:
  inline void _debug_codepoint(std::ostream &stream) const {
    switch (_code_point) {
    case '\n':
      stream << "\\n";
      break;
    case EOF:
      stream << "EOF";
      break;
    default:
      stream << _code_point;
    }
  }

protected:
  /// A logger instance.
  std::shared_ptr<Logger> _logger;

  /// The lexer's panic thrown, if any.
  std::optional<Panic> _panic;

  /// The lexer's unhandled exception thrown, if any.
  std::optional<std::exception> _exception;

  /// The latest yielded cursor position.
  Position _latest_yieled_cursor;

  /// The current cursor position.
  Position _cursor;

  /// The latest read code point.
  /// TODO: Move to `char32_t`.
  char _code_point;

  inline Placement _placement() const {
    return Placement(unit, Location(_latest_yieled_cursor, _cursor));
  }

  /// Yield a token implicitly prepending the correct placement.
  ///
  /// @code{C++}
  /// co_yield _token<StringLiteral>(literal_value)
  /// @endcode
  template <typename YieldedToken, typename... Args>
  Token _token(Args... args) {
    auto plc = _placement();
    _latest_yieled_cursor = _cursor;
    return YieldedToken(plc, args...);
  }

  /// Read the next codepoint, returning the previous one.
  char _advance() {
    char old = _code_point;

    if (unit->source_stream().good()) {
      _code_point = unit->source_stream().get();

      auto &log = _logger->strace();
      log << "Read `";
      _debug_codepoint(log);
      fmt::print(log, "` at {}:{}\n", _cursor.row, _cursor.col);

      if (_is_newline()) {
        _cursor.row += 1;
        _cursor.col = 0;
      } else {
        _cursor.col += 1;
      }

      return old;
    } else if (_is_eof()) {
      throw Panic("Unexpected EOF in lexer", _placement());
    } else {
      throw Panic("Error reading from unit", _placement());
    }
  }

  Panic _unexpected() { return Panic("Unexpected input", _placement()); }

  Panic _unexpected(std::string expected) {
    return Panic("Expected " + expected, _placement());
  }

  Panic _unexpected(std::set<char> expected) {
    return Panic(
        fmt::format("Expected {}", fmt::join(expected, ", ")), _placement());
  }

  /// Is the latest codepoint equal to *cmp*?
  inline bool _is(char cmp) const { return _code_point == cmp; }

  /// Is the latest codepoint equal to *cmp*?
  inline bool _is(std::set<char> cmp) const {
    return cmp.contains(_code_point);
  }

  /// Has the stream ended?
  inline bool _is_eof() const {
    return _code_point == EOF || unit->source_stream().eof();
  }

  /// Match a line breaking (End-Of-Line, EOL) character.
  /// For now, it only matches system-specific newline.
  ///
  /// @todo Match EOL in Unicode-correct way.
  /// @see https://www.unicode.org/reports/tr14/
  inline bool _is_newline() const { return _code_point == '\n'; }

  /// Match characters U+0020 (space), U+0009 (horizontal tab) and
  /// U+0013 (vertical tab).
  ///
  /// @todo Also match Unicode characters with @c Zs category.
  inline bool _is_space() const {
    return (_code_point == ' ' || _code_point == '\t' || _code_point == '\v');
  }

  /// Match an ASCII digit in given radix, case insensitive.
  inline bool _is_num(Radix radix) const {
    switch (radix) {
    case Radix::Binary:
      return _code_point == '0' || _code_point == '1';
    case Radix::Octal:
      return _code_point >= '0' && _code_point <= '7';
    case Radix::Decimal:
      return _code_point >= '0' && _code_point <= '9';
    case Radix::Hexadecimal:
      return (_code_point >= '0' && _code_point <= '9') ||
             (_code_point >= 'a' && _code_point <= 'f') ||
             (_code_point >= 'A' && _code_point <= 'F');
    }
  }

  /// Match @c /0-1/ .
  inline bool _is_binary() const { return _is_num(Radix::Binary); };

  /// Match @c /0-7/ .
  inline bool _is_octal() const { return _is_num(Radix::Octal); };

  /// Match @c /0-9/ .
  inline bool _is_decimal() const { return _is_num(Radix::Decimal); };

  /// Match @c /[0-9a-fA-F]/ .
  inline bool _is_hexadecimal() const { return _is_num(Radix::Hexadecimal); }

  /// Match @c /a-z/ .
  inline bool _is_latin_lowercase() const {
    return _code_point >= 'a' && _code_point <= 'z';
  }

  /// Match @c /A-Z/ .
  inline bool _is_latin_uppercase() const {
    return _code_point >= 'A' && _code_point <= 'Z';
  }

  /// Match @c /[a-zA-Z]/ .
  inline bool _is_latin_alpha() const {
    return _is_latin_lowercase() || _is_latin_uppercase();
  }
};

} // namespace NXC
} // namespace Fancysoft
