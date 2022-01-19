#pragma once

#include <string>
#include <variant>

#include "../token.hh"

namespace Fancysoft {
namespace Phoenix {
namespace C {

namespace Token {

using Base = Phoenix::Token;

/// NOTE: `Newline` and `Space` won't preserve the original amount of
/// characters.
struct Punct : Base {
  const char *token_name() const override { return "C/Punct"; }

  enum Kind {
    Newline,    ///< `␤`
    Space,      ///< ` `
    Comma,      ///< `,`
    Semi,       ///< `;`
    OpenParen,  ///< `(`
    CloseParen, ///< `)`
    Query,      ///< `.`
    Varg,       ///< `...`
  };

  Kind kind;

  static const char *kind_to_expected(Kind kind) {
    switch (kind) {
    case Newline:
      return "\\n";
    case Space:
      return "space";
    case Comma:
      return ",";
    case Semi:
      return ";";
    case OpenParen:
      return "(";
    case CloseParen:
      return ")";
    case Query:
      return ".";
    case Varg:
      return "varg";
    }
  }

  Punct(Placement plc, Kind kind) : Base(plc), kind(kind) {}

  const void print(std::ostream &o) const override {
    switch (kind) {
    case Newline:
      o << '\n';
    case Space:
      o << ' ';
    case Comma:
      o << ',';
    case Semi:
      o << ';';
    case OpenParen:
      o << '(';
    case CloseParen:
      o << ')';
    case Query:
      o << '.';
    case Varg:
      o << "...";
    }
  };
};

/// TODO: The list of C operators is well-known.
struct Op : Base {
  std::string value;
  Op(Placement plc, std::string value) : Base(plc), value(value) {}
  const char *token_name() const override { return "C/Op"; }
  const void print(std::ostream &stream) const override { stream << value; };
};

// A C id, which may consist of multiple words, e.g. `unsigned int`.
struct Id : Base {
  std::string value;
  Id(Placement plc, std::string value) : Base(plc), value(value) {}
  const void print(std::ostream &stream) const override { stream << value; };
  const char *token_name() const override { return "C/Id"; }
};

using Any = std::variant<Punct, Op, Id>;

} // namespace Token
} // namespace C
} // namespace Phoenix
} // namespace Fancysoft