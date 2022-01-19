#pragma once

#include <fmt/core.h>

#include "./lexer.hh"
#include "./panic.hh"
#include "./util/node.hh"

namespace Fancysoft {
namespace Phoenix {

/// OPTIMIZE: Make the template argument `LexerT<TokenT>` instead.
template <typename LexerT, typename TokenT> struct Parser {
  static_assert(
      std::is_base_of<Lexer<TokenT>, LexerT>::value,
      "`LexerT` must derive from `NXC::Lexer<TokenT>`");

  Parser(LexerT *lexer, std::shared_ptr<Util::Logger> logger) :
      _lexer(lexer), _logger(logger) {}

private:
  /// The lexing coroutine yielding a token.
  std::unique_ptr<Util::Coro::Generator<TokenT>> _token_coro;

  /// The latest yielded token.
  std::optional<TokenT> _latest_token;

  void _debug(TokenT token) {
    auto &log = _logger->sdebug();
    log << "Lexer yielded ";
    std::visit([&log](auto &&token) { log << token.token_name(); }, token);
    log << "\n";
  }

protected:
  /// A logger instance.
  std::shared_ptr<Util::Logger> _logger;

  /// The lexer instance pointer.
  LexerT *_lexer;

  /// Debug log a *node*.
  void _debug(Util::Node *node) {
    auto &log = _logger->sdebug();
    log << "Parsed ";
    node->trace(log);
    log << "\n";
  }

  /// Must be called before `_advance()`.
  /// It fills up `_token_stack` with the next token value.
  void _initialize() {
    if (_token_coro)
      throw "The token coroutine has already been created";

    _token_coro =
        std::make_unique<Util::Coro::Generator<TokenT>>(_lexer->lex());

    _token_coro.get()->begin();
    _latest_token = _token_coro.get()->current();
    _debug(_latest_token.value());
  }

  /// Check if lexer has done yielding tokens.
  bool _lexer_done() const {
    if (_token_coro)
      return _token_coro.get()->done();
    else
      throw "The token coroutine hasn't been created yet";
  }

  /// Advance the parser, returning the old token value.
  /// `_initialize()` must be called beforeahead.
  TokenT _advance() {
    if (_lexer_done())
      throw _unexpected_eof();

    auto old = _latest_token.value();
    _latest_token = _token_coro.get()->next();
    _debug(_latest_token.value());

    if (_lexer->exception())
      throw _lexer->exception().value();
    else if (_lexer->panic())
      throw _lexer->panic().value();

    return old;
  }

  /// Check if current token is *T*.
  template <class T> bool _is() const {
    return std::holds_alternative<T>(_latest_token.value());
  }

  /// Check if current token is one of *T...*.
  template <class T1, class T2, class... Ts> bool _is() const {
    return _is<T1>() || _is<T2, Ts...>();
  }

  /// Check if current token is *T*, and return a copy of it.
  /// Otherwise return `std::nullopt`.
  template <class T> std::optional<T> _if() const {
    if (_latest_token.has_value()) {
      if (auto token = std::get_if<T>(&_latest_token.value())) {
        return *token;
      } else {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
  }

  /// Check if current token is one of *T...*, and return the variant of
  /// copies. Otherwise return `std::nullopt`.
  template <class T1, class T2, class... Ts>
  std::optional<std::variant<T1, T2, Ts...>> _if() const {
    if (auto token = _if<T1>()) {
      return std::variant<T1, T2, Ts...>(*token);
    } else {
      return _if<T2, Ts...>();
    }
  }

  /// Check if current token is *T* and return a copy of it.
  /// Otherwise throw an unexpected error.
  template <typename T> T _as() const {
    auto token = _if<T>();

    if (token.has_value()) {
      return token.value();
    } else {
      throw _expected<T>();
    }
  }

  /// Check if current token is one of *T...* and return the variant of
  /// copies. Otherwise throw an unexpected error.
  template <typename T1, typename T2, typename... Ts>
  std::variant<T1, T2, Ts...> _as() const {
    auto tokens = _if<T1, T2, Ts...>();

    if (tokens.has_value()) {
      return tokens;
    } else {
      throw _expected<T1>(); // TODO: List all expected tokens
    }
  }

  /// Consume current token as `T` and advance, returning the consumed token.
  template <typename T> T _consume() {
    auto token = _as<T>();
    _advance();
    return token;
  }

  /// Return an "Unexpected token" panic with message
  /// telling which token was expected instead.
  template <typename T> Panic _expected() const {
    return std::visit(
        [this](auto &&token) {
          return Panic(
              fmt::format(
                  "Unexpected token {}, expected {}",
                  token.token_name(),
                  T::token_name()),
              token.placement);
        },
        _latest_token.value());
  }

  /// Return an "Unexpected token" panic with message
  /// telling what was expected instead.
  Panic _expected(const std::string what_expected) const {
    return std::visit(
        [this, what_expected](auto &&token) {
          return Panic(
              fmt::format(
                  "Unexpected token {}, expected {}",
                  token.token_name(),
                  what_expected),
              token.placement);
        },
        _latest_token.value());
  }

  /// Return an "Unexpected token" panic with latest token.
  Panic _unexpected() const {
    return std::visit(
        [this](auto &&token) {
          return Panic(
              fmt::format("Unexpected token {}", token.token_name()),
              token.placement);
        },
        _latest_token.value());
  }

  struct UnexpectedEOF : Panic {
    UnexpectedEOF(Placement placement) :
        Panic(Panic::UnexpectedEOF, "Unexpected EOF", placement) {}
  };

  /// Return an "Unexpected EOF" panic instance.
  UnexpectedEOF _unexpected_eof() const {
    if (_latest_token.has_value()) {
      std::visit(
          [this](auto &&token) { throw UnexpectedEOF(token.placement); },
          _latest_token.value());
    } else {
      // If no token has been read at all.
      throw UnexpectedEOF(Placement(_lexer->unit));
    }
  }
};

} // namespace Phoenix
} // namespace Fancysoft
