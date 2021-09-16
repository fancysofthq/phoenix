#pragma once

#include <optional>
#include <string>
#include <variant>

#include "../token.hh"
#include "../unit.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

namespace Token {

using Base = NXC::Token;

struct Keyword : Base {
  static const char *token_name() { return "<Keyword>"; }

  enum Kind {
    Extern,
    Let,
    Final,
    UnsafeBang,
    FragileBang,
    ThreadsafeBang,
  };

  static const char *kind_to_string(Kind kind) {
    switch (kind) {
    case Extern:
      return "extern";
    case Let:
      return "let";
    case Final:
      return "final";
    case UnsafeBang:
      return "unsafe!";
    case FragileBang:
      return "fragile!";
    case ThreadsafeBang:
      return "threadsafe!";
    }
  };

  static std::optional<Kind> parse_kind(std::string string) {
    if (!string.compare("extern"))
      return Extern;
    else if (!string.compare("let"))
      return Let;
    else if (!string.compare("final"))
      return Final;
    else if (!string.compare("unsafe!"))
      return UnsafeBang;
    else if (!string.compare("fragile!"))
      return FragileBang;
    else if (!string.compare("threadsafe!"))
      return ThreadsafeBang;
    else
      return std::nullopt;
  }

  Kind kind;

  Keyword(Placement plc, Kind kind) : Base(plc), kind(kind) {}

  const void print(std::ostream &stream) const override {
    stream << kind_to_string(kind);
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<Keyword " << kind_to_string(kind) << ">";
  };
};

struct Punct : Base {
  static const char *token_name() { return "<Punct>"; }

  enum Kind {
    Newline,
    HSpace, ///< Horizontal space.
    Comma,
    OpenParen,
    CloseParen,
  };

  Kind kind;

  /// Used in debug messages, e.g. "Panic! Expected space".
  static std::string kind_to_expected(Kind kind) {
    switch (kind) {
    case Newline:
      return "newline";
    case HSpace:
      return "space";
    case Comma:
      return "comma";
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
    stream << "<Punct ";

    if (kind == Newline)
      stream << "\\n";
    else if (kind == HSpace)
      stream << "(space)";
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
      return ';';
    case OpenParen:
      return '(';
    case CloseParen:
      return ')';
    }
  }
};

/// An identifier, e.g. @c foo .
struct Id : Base {
  static const char *token_name() { return "<Id>"; }

  std::string id;
  Id(Placement plc, std::string id) : Base(plc), id(id) {}

  const void print(std::ostream &stream) const override { stream << id; };

  const void inspect(std::ostream &stream) const override {
    stream << "<Id " << id << ">";
  };
};

/// A C identifier.
struct CId : Base {
  static const char *token_name() { return "<CId>"; }

  /// A C id may consist of multiple words, and it doesn't include pointers.
  /// NOTE: It is always normalized, e.g. `"long int"`, not `"long   int"`.
  std::string id;

  /// Is it wrapped in backticks? E.g. `` $`unsigned int` ``.
  bool is_wrapped;

  CId(Placement plc, std::string id) : Base(plc), id(id) {}

  const void print(std::ostream &stream) const override {
    stream << '$';

    if (is_wrapped)
      stream << "`";

    stream << id;

    if (is_wrapped)
      stream << "`";
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<CId $`" << id << "`>";
  };
};

/// An operator, e.g. @c / .
struct Op : Base {
  static const char *token_name() { return "<Op>"; }

  std::string op;
  Op(Placement plc, std::string op) : Base(plc), op(op) {}

  const void print(std::ostream &stream) const override { stream << op; };

  const void inspect(std::ostream &stream) const override {
    stream << "<Op " << op << ">";
  };
};

/// A string literal, e.g. @c "foo" .
struct StringLiteral : Base {
  static const char *token_name() { return "<StringLiteral>"; }

  std::string string;

  StringLiteral(Placement plc, std::string string) :
      Base(plc), string(string) {}

  const void print(std::ostream &stream) const override {
    stream << '"' << string << "'";
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<StringLiteral \"" << string << "\">";
  };
};

struct CStringLiteral : Base {
  static const char *token_name() { return "<CStringLiteral>"; }

  std::string string;

  CStringLiteral(Placement plc, std::string string) :
      Base(plc), string(string) {}

  const void print(std::ostream &stream) const override {
    stream << "$\"" << string << "'";
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<CStringLiteral \"" << string << "\">";
  };
};

using Any =
    std::variant<Keyword, Punct, Id, CId, Op, StringLiteral, CStringLiteral>;

} // namespace Token

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
