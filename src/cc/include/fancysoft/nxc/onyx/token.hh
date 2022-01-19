#pragma once

#include <optional>
#include <string>
#include <variant>

#include "../placement.hh"
#include "../token.hh"
#include "../unit.hh"
#include "./lang.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {
namespace Token {

using Base = NXC::Token;

/// A newline-terminated comment token.
struct Comment : Base {
  const std::string value;
  Comment(Placement plc, std::string value) : Base(plc), value(value) {}
  const void print(std::ostream &o) const override { o << '#' << value; };
  const char *token_name() const override { return "Comment"; }
};

/// A keyword token.
struct Keyword : Base {
  const char *token_name() const override { return "Keyword"; }

  enum Kind {
    Extern,

    Import,
    From,
    Export,

    Let,
    Final,

    Unsafe,
    Fragile,
    Threadsafe,
    UnsafeBang,     ///< `unsafe!`
    FragileBang,    ///< `fragile!`
    ThreadsafeBang, ///< `threadsafe!`

    Do,
    End,

    Decl,
    Impl,
    Def,
    Reimpl,

    Return,

    If,
    Then,
    Elif,
    Else,
    While,

    Alias,

    Sealed,

    Builtin,
    Trait,
  };

  static const char *kind_to_str(Kind kind) {
    switch (kind) {
    case Extern:
      return "extern";

    case Import:
      return "import";
    case From:
      return "from";
    case Export:
      return "export";

    case Let:
      return "let";
    case Final:
      return "final";

    case Unsafe:
      return "unsafe";
    case Fragile:
      return "fragile";
    case Threadsafe:
      return "threadsafe";

    case UnsafeBang:
      return "unsafe!";
    case FragileBang:
      return "fragile!";
    case ThreadsafeBang:
      return "threadsafe!";

    case Do:
      return "do";
    case End:
      return "end";

    case Decl:
      return "decl";
    case Impl:
      return "impl";
    case Def:
      return "def";
    case Reimpl:
      return "reimpl";

    case Return:
      return "return";

    case If:
      return "if";
    case Then:
      return "then";
    case Elif:
      return "elif";
    case Else:
      return "else";
    case While:
      return "while";

    case Sealed:
      return "sealed";
    case Alias:
      return "alias";

    case Builtin:
      return "builtin";
    case Trait:
      return "trait";
    }
  };

  static std::optional<Kind> parse_kind(std::string string) {
    if (!string.compare("extern"))
      return Extern;

    if (!string.compare("import"))
      return Import;
    if (!string.compare("from"))
      return From;
    if (!string.compare("export"))
      return Export;

    else if (!string.compare("let"))
      return Let;
    else if (!string.compare("final"))
      return Final;

    else if (!string.compare("unsafe"))
      return Unsafe;
    else if (!string.compare("fragile"))
      return Fragile;
    else if (!string.compare("threadsafe"))
      return Threadsafe;

    else if (!string.compare("unsafe!"))
      return UnsafeBang;
    else if (!string.compare("fragile!"))
      return FragileBang;
    else if (!string.compare("threadsafe!"))
      return ThreadsafeBang;

    else if (!string.compare("do"))
      return Do;
    else if (!string.compare("end"))
      return End;

    else if (!string.compare("decl"))
      return Decl;
    else if (!string.compare("impl"))
      return Impl;
    else if (!string.compare("def"))
      return Def;

    else if (!string.compare("return"))
      return Return;

    else if (!string.compare("if"))
      return If;
    else if (!string.compare("then"))
      return Then;
    else if (!string.compare("elif"))
      return Elif;
    else if (!string.compare("else"))
      return Else;
    else if (!string.compare("while"))
      return While;

    else if (!string.compare("sealed"))
      return Sealed;
    else if (!string.compare("alias"))
      return Alias;

    else if (!string.compare("builtin"))
      return Builtin;
    else if (!string.compare("trait"))
      return Trait;

    else
      return std::nullopt;
  }

  Kind kind;

  Keyword(Placement plc, Kind kind) : Base(plc), kind(kind) {}

  const void print(std::ostream &o) const override { o << kind_to_str(kind); };

  // const void inspect(std::ostream &o) const override {
  //   o << "<Keyword " << kind_to_str(kind) << ">";
  // };
};

/// A punctuation token.
struct Punct : Base {
  const char *token_name() const override { return "Punct"; }

  enum Kind {
    Newline,
    Space, ///< Horizontal space.

    Comma,
    Colon,
    Semi,
    At, ///< `@`.

    OpenParen,
    CloseParen,

    OpenBracket,
    CloseBracket,

    OpenAngle,
    CloseAngle,

    OpenSquare,
    CloseSquare,

    AccessStatic,   ///< `::`.
    AccessInstance, ///< `:`.
    AccessMember,   ///< `.`.

    ArrowAssign, ///< `=>`.
    ArrowBlock,  ///< `->`.
    ArrowLambda, ///< `~>`.

    Asterisk, ///< `*`, maybe-op, used as "all".
    Tilde,    ///< `~`, maybe-op, used in virtual types.
  };

  Kind kind;

  /// Special care shall be taken of multi-char puncts, maybe-ops and also of:
  ///
  /// * `Newline`
  /// * `Space`
  /// * `Colon` (space-wrapped `:`)
  /// * `AccessStatic` (`::`)
  /// * `AccessInstance` (`:`)
  ///
  static std::optional<Kind> char_to_kind(char c) {
    switch (c) {
    case ',':
      return Comma;
    case ';':
      return Semi;
    case '@':
      return At;

    case '(':
      return OpenParen;
    case ')':
      return CloseParen;

    case '{':
      return OpenBracket;
    case '}':
      return CloseBracket;

    case '<':
      return OpenAngle;
    case '>':
      return CloseAngle;

    case '[':
      return OpenSquare;
    case ']':
      return CloseSquare;

    case '.':
      return AccessMember;

    default:
      return std::nullopt;
    }
  }

  static const char *kind_to_str(Kind kind) {
    switch (kind) {
    case Newline:
      return "\n";
    case Space:
      return " ";

    case Comma:
      return ",";
    case Colon:
      return ":";
    case Semi:
      return ";";
    case At:
      return "@";

    case OpenParen:
      return "(";
    case CloseParen:
      return ")";

    case OpenBracket:
      return "{";
    case CloseBracket:
      return "}";

    case OpenAngle:
      return "<";
    case CloseAngle:
      return ">";

    case OpenSquare:
      return "[";
    case CloseSquare:
      return "]";

    case AccessStatic:
      return "::";
    case AccessInstance:
      return ":";
    case AccessMember:
      return ".";

    case ArrowAssign:
      return "=>";
    case ArrowBlock:
      return "->";
    case ArrowLambda:
      return "~>";

    case Asterisk:
      return "*";
    case Tilde:
      return "~";
    }
  }

  static const char *kind_to_safe_str(Kind kind) {
    switch (kind) {
    case Newline:
      return "\\n";
    case Space:
      return "space";
    default:
      return kind_to_str(kind);
    }
  }

  Punct(Placement plc, Kind kind) : Base(plc), kind(kind) {}
  const void print(std::ostream &o) const override { o << kind_to_str(kind); };

  // const void inspect(std::ostream &o) const override {
  //   o << "<Punct ";

  //   if (kind == Newline)
  //     o << "`\\n`";
  //   else
  //     o << "`" << kind_to_str(kind) << "`";

  //   o << ">";
  // };
};

/// A single-element identifier, e.g. `foo`.
struct Id : Base {
  const char *token_name() const override { return "Id"; }

  std::string value;
  Id(Placement plc, std::string value) : Base(plc), value(value) {}

  const void print(std::ostream &o) const override { o << value; };

  // const void inspect(std::ostream &o) const override {
  //   o << "<Id " << value << ">";
  // };
};

/// A C identifier, e.g. `$foo`.
struct CId : Base {
  const char *token_name() const override { return "CId"; }

  /// A C id may consist of multiple words, and it doesn't include pointers.
  /// NOTE: It is always normalized, e.g. `"long int"`, not `"long   int"`.
  std::string value;

  /// Is it wrapped in backticks? E.g. `` $`unsigned int` ``.
  bool is_wrapped;

  CId(Placement plc, std::string value) : Base(plc), value(value) {}

  const void print(std::ostream &o) const override {
    o << '$';

    if (is_wrapped)
      o << "`";

    o << value;

    if (is_wrapped)
      o << "`";
  };

  // const void inspect(std::ostream &o) const override {
  //   o << "<CId $`" << value << "`>";
  // };
};

/// A label token, e.g. `foo:`.
struct Label : Base {
  const char *token_name() const override { return "Label"; }

  std::string value;
  Label(Placement plc, std::string value) : Base(plc), value(value) {}

  const void print(std::ostream &o) const override { o << value << ':'; };

  // const void inspect(std::ostream &o) const override {
  //   o << "<Label " << value << ":>";
  // };
};

/// An operator, e.g. `+`.
struct Op : Base {
  const char *token_name() const override { return "Op"; }

  /// Is *c* a possible operator?
  static bool is(char c) {
    switch (c) {
    case '=':
    case '~':
    case '-':
    case '+':
    case '!':
    case '&':
    case '*':
    case '%':
    case '^':
    case '/':
    case '<':
    case '>':
      return true;
    default:
      return false;
    }
  }

  std::string value;
  Op(Placement plc, std::string value) : Base(plc), value(value) {}

  const void print(std::ostream &o) const override { o << value; };

  // const void inspect(std::ostream &o) const override {
  //   o << "<Op \"" << value << "\">";
  // };
};

/// A literal restriction token, e.g. `\bool`.
struct LiteralRestriction : Base {
  const Lang::LiteralRestriction kind;

  LiteralRestriction(Placement plc, Lang::LiteralRestriction kind) :
      Base(plc), kind(kind) {}

  const char *token_name() const override { return "LiteralRestriction"; }

  const void print(std::ostream &o) const override {
    switch (kind) {
    case Lang::LiteralRestriction::Bool:
      o << "\\bool";
      break;
    case Lang::LiteralRestriction::UInt:
      o << "\\uint";
      break;
    }
  };
};

/// A bool literal, e.g. `true`.
struct BoolLiteral : Base {
  const char *token_name() const override { return "BoolLiteral"; }
  bool value;
  BoolLiteral(Placement plc, bool value) : Base(plc), value(value) {}
  const void print(std::ostream &o) const override { o << value; }
};

/// An integer literal, e.g. `42`. Note that it doesn't include sign.
struct IntLiteral : Base {
  const char *token_name() const override { return "IntLiteral"; }
  unsigned long long value;
  IntLiteral(Placement plc, long long value) : Base(plc), value(value) {}
  const void print(std::ostream &o) const override { o << value; }
};

/// An Onyx string literal, e.g. `"foo"`.
struct StringLiteral : Base {
  const char *token_name() const override { return "StringLiteral"; }

  std::string value;

  StringLiteral(Placement plc, std::string value) : Base(plc), value(value) {}

  const void print(std::ostream &o) const override {
    o << '"' << value << '"';
  };

  // const void inspect(std::ostream &o) const override {
  //   o << "<StringLiteral \"" << value << "\">";
  // };
};

/// A null-terminated C string literal, e.g. `$"foo"`.
struct CStringLiteral : Base {
  const char *token_name() const override { return "CStringLiteral"; }

  std::string value;

  CStringLiteral(Placement plc, std::string value) : Base(plc), value(value) {}

  const void print(std::ostream &o) const override {
    o << "$\"" << value << "'";
  };

  // const void inspect(std::ostream &o) const override {
  //   o << "<CStringLiteral \"" << value << "\">";
  // };
};

using Any = std::variant<
    Comment,
    Keyword,
    Punct,
    Id,
    CId,
    Label,
    Op,
    LiteralRestriction,
    BoolLiteral,
    IntLiteral,
    StringLiteral,
    CStringLiteral>;

} // namespace Token
} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft

namespace std {
template <> struct less<Fancysoft::NXC::Onyx::Token::Label> {
  bool operator()(
      const Fancysoft::NXC::Onyx::Token::Label &k1,
      const Fancysoft::NXC::Onyx::Token::Label &k2) const {
    return k1.value < k2.value;
  }
};
} // namespace std
