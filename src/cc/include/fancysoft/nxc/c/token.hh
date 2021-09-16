#pragma once

#include <string>
#include <variant>

#include "../token.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

namespace Token {

using Base = NXC::Token;

// TODO: Parser calls `_is_punct(Token::Punct::OpenParen)`.
// NOTE: Newline and HSpace don't preserve the original amount of characters.
struct Punct : Base {
  static const char *token_name() { return "<C/Punct>"; }

  enum Kind {
    Newline,
    HSpace, ///< Horizontal space.
    Comma,
    Semi,
    OpenParen,
    CloseParen,
  };

  Kind kind;

  static const char *kind_to_expected(Kind kind) {
    switch (kind) {
    case Newline:
      return "newline";
    case HSpace:
      return "space";
    case Comma:
      return "comma";
    case Semi:
      return "semicolon";
    case OpenParen:
      return "opening parenthesis";
    case CloseParen:
      return "closing parenthesis";
    }
  }

  Punct(Placement plc, Kind kind) : Base(plc), kind(kind) {}

  const void print(std::ostream &stream) const override {
    stream << kind_to_char(kind);
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<C/Punct ";

    if (kind == Newline)
      stream << "\\n";
    else
      stream << kind_to_char(kind);

    stream << ">";
  };

private:
  static char kind_to_char(Kind kind) {
    switch (kind) {
    case Newline:
      return '\n';
    case HSpace:
      return ' ';
    case Comma:
      return ',';
    case Semi:
      return ';';
    case OpenParen:
      return '(';
    case CloseParen:
      return ')';
    }
  }
};

struct Op : Base {
  static const char *token_name() { return "<C/Op>"; }

  std::string op;

  Op(Placement plc, std::string op) : Base(plc), op(op) {}

  const void print(std::ostream &stream) const override { stream << op; };

  const void inspect(std::ostream &stream) const override {
    stream << "<C/Op " << op << ">";
  };
};

// A C id, which may consist of multiple words, e.g. `unsigned int`.
struct Id : Base {
  static const char *token_name() { return "<C/Id>"; }

  // The id string is guaranteed to not contain any excessive spaces.
  std::string id;

  Id(Placement plc, std::string id) : Base(plc), id(id) {}

  const void print(std::ostream &stream) const override { stream << id; };

  const void inspect(std::ostream &stream) const override {
    stream << "<C/Id " << id << ">";
  };
};

using Any = std::variant<Punct, Op, Id>;

} // namespace Token
} // namespace C
} // namespace NXC
} // namespace Fancysoft
