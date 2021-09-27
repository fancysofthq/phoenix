#pragma once

#include <iostream>
#include <memory>
#include <optional>
#include <variant>

#include <fmt/core.h>

#include "../util/coro.hh"
#include "../util/logger.hh"
#include "../util/variant.hh"

#include "./exception.hh"
#include "./lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {

/// OPTIMIZE: Make the template argument `LexerT<TokenT>` instead.
template <typename LexerT, typename TokenT> struct Parser {
  static_assert(
      std::is_base_of<Lexer<TokenT>, LexerT>::value,
      "`LexerT` must derive from `NXC::Lexer<TokenT>`");

  Parser(std::shared_ptr<LexerT> lexer) : _lexer(lexer) {}

private:
  /// The lexing coroutine yielding a token.
  std::unique_ptr<Util::Coro::Generator<TokenT>> _token_coro;

  /// A container for the latest token (not set until first advanced).
  std::optional<TokenT> _token_container;

  void _debug_token(std::ostream &output) const {
    std::visit(
        [&output](auto &&token) {
          output << "Lexer yielded ";
          token.inspect(output);
          output << '\n';
        },
        _token());
  }

protected:
  /// The lexer instance.
  const std::shared_ptr<LexerT> _lexer;

  virtual const char *_debug_name() const = 0;

  void _debug_parsed(std::string node_name) {
    fmt::print(Util::logger.debug(_debug_name()), "Parsed {}\n", node_name);
  }

  /// Must be called before `_advance()`.
  /// It fills up `_token()` with the next token value.
  void _initialize() {
    if (_token_coro)
      throw "The token coroutine has already been created";

    _token_coro =
        std::make_unique<Util::Coro::Generator<TokenT>>(_lexer->lex());

    _token_coro.get()->begin();
    _token_container = _token_coro.get()->current();
    _debug_token(Util::logger.debug(_debug_name()));
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
    if (_lexer_done()) {
      if (_lexer->exception()) {
        throw _lexer->exception().value();
      } else {
        throw "Lexer is already done, can not advance";
      }
    }

    auto old = _token();

    _token_container = _token_coro.get()->next();
    _debug_token(Util::logger.debug(_debug_name()));

    return old;
  }

  /// Force get the stored token pointer from the `_token_container`,
  /// throw otherwise.
  const TokenT &_token() const {
    if (this->_token_container.has_value())
      return this->_token_container.value();
    else
      throw "The token container is empty";
  }

  /// Advance and expect the next token to be `T`, returning its ref.
  template <class T> const T &_next_as() {
    _advance();

    if (auto matching = std::get_if<T>(&_token())) {
      return *matching;
    } else {
      throw _unexpected(T::token_name());
    }
  }

  /// Check if current token is *T*.
  template <class T> bool _is() const {
    return std::holds_alternative<T>(this->_token());
  }

  /// Check if current token is one of *T...*.
  template <class T1, class T2, class... Ts> bool _is() const {
    return _is<T1>() || _is<T2, Ts...>();
  }

  /// Check if current token is *T*, and return a copy of it.
  /// Otherwise return `std::nullopt`.
  template <class T> std::optional<T> _if() const {
    if (this->_token_container.has_value()) {
      if (auto token = std::get_if<T>(&this->_token())) {
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
      throw _unexpected();
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
      throw _unexpected();
    }
  }

  /// Return an unexpected token panic.
  Panic _unexpected() const {
    return std::visit(
        [this](auto &&token) {
          return Panic(
              fmt::format("Unexpected token {}", token.token_name()),
              token.placement);
        },
        this->_token());
  }

  /// Return an unexpected token panic with message
  /// telling what was expected instead.
  Panic _unexpected(const std::string what_expected) const {
    return std::visit(
        [this, what_expected](auto &&token) {
          return Panic(
              fmt::format(
                  "Unexpected token {}, expected {}",
                  token.token_name(),
                  what_expected),
              token.placement);
        },
        this->_token());
  }

  /// Return an "Unexpected EOF" panic.
  Panic _unexpected_eof() const {
    if (this->_token_container.has_value()) {
      return std::visit(
          [this](auto &&token) {
            return Panic("Unexpected EOF", token.placement);
          },
          this->_token());

    } else {
      return Panic("Unexpected EOF");
    }
  }
};

} // namespace NXC
} // namespace Fancysoft
