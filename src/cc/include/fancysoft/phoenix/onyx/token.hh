#pragma once

#include <assert.h>
#include <optional>
#include <string.h>
#include <string>
#include <variant>
#include <xutility>

#include "../c/token.hh"
#include "../placement.hh"
#include "../token.hh"
#include "../unit.hh"
#include "./lang.hh"
#include "fancysoft/phoenix/util/utf8.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Onyx {
namespace Token {

using Base = Phoenix::Token;

/// A newline-terminated comment token.
struct Comment : Base {
  const std::string value;
  Comment(Placement plc, std::string value) : Base(plc), value(value) {}
  const void print(std::ostream &o) const override { o << '#' << value; };
  const char *token_name() const override { return "Comment"; }
};

/// A keyword token.
struct Keyword : Base {
  enum Kind {
    Extern,

    Import,
    As,
    From,

    Export,
    Default,

    Builtin,
    Private,
    Static,

    Let,
    Final,
    Getter,

    Unsafe,
    Fragile,
    Threadsafe,
    UnsafeBang,     ///< `unsafe!`
    FragileBang,    ///< `fragile!`
    ThreadsafeBang, ///< `threadsafe!`

    Decl,
    Redecl,
    Impl,
    Def,
    Reimpl,
    Extend,

    Return,
    Convey,

    Switch,
    Case,
    If,
    Then,
    Elif,
    Else,

    While,
    Do,
    Break,
    Continue,

    And,
    Or,
    Not,

    Distinct,
    Alias,
    To,

    Trait,
    Struct,
    Class,
    Enum,
    Unit,
    Annotation,
  };

  static const char *kind_to_str(Kind kind);
  static std::optional<Kind> parse_kind(std::string string);

  Kind kind;
  Keyword(Placement plc, Kind kind) : Base(plc), kind(kind) {}

  const char *token_name() const override { return "Keyword"; }
  const void print(std::ostream &o) const override { o << kind_to_str(kind); };
};

/// A punctuation token.
struct Punct : Base {
  enum Kind {
    Newline,
    Space, ///< Horizontal space.

    Comma, ///< `,`.
    Colon, ///< `:` (wrapped in spaces).
    Semi,  ///< `;`.

    AnnotationOpen, ///< `@[`.

    MacroOpen,            ///< `{%`.
    MacroClose,           ///< `%}`.
    EmitMacroOpen,        ///< `{{`.
    EmitMacroClose,       ///< `}}`.
    DelayedMacroOpen,     ///< `\{%`.
    DelayedEmitMacroOpen, ///< `\{{`.

    ParenOpen,  ///< `(`.
    ParenClose, ///< `)`.

    BracketOpen,  ///< `{`.
    BracketClose, ///< `}`.

    AngleOpen,  ///< `<`.
    AngleClose, ///< `>`.

    SquareOpen,  ///< `[`.
    SquareClose, ///< `]`.

    Pipe, ///< `|`, maybe-op.

    ScopeStatic,   ///< `::`.
    ScopeInstance, ///< `.`.
    ScopeUFCS,     ///< `:` (with adjacent identifier).

    ArrowGenerator, ///< `=>`.
    ArrowFunction,  ///< `->`.
    ArrowLambda,    ///< `~>`.
  };

  static const char *kind_to_str(Kind kind);
  static const char *kind_to_safe_str(Kind kind);

  Kind kind;
  Punct(Placement plc, Kind kind) : Base(plc), kind(kind) {}

  const char *token_name() const override { return "Punct"; }
  const void print(std::ostream &o) const override { o << kind_to_str(kind); };
};

/// An identifier, e.g. `foo`.
struct ID : Base {
  /// TIP: `T*l0` requires difference between simple and type Onyx IDs.
  enum Kind {
    Simple,    ///< A simple Onyx ID, e.g. `foo`.
    Literal,   ///< A literal ID, e.g. `this`.
    C,         ///< A C ID, e.g. `$foo`.
    Intrinsic, ///< An intrinsic ID, e.g. `@foo`.
    Label,     ///< A label ID, e.g. `foo:`.
    Symbol,    ///< A symbol ID, e.g. `:foo`.
  };

  /// Optional pointer params for this ID, applicable to `Simple` and `C` only.
  struct PointerSuffix {
    /// Zero depth means single `*`.
    unsigned depth;

    /// `u` (unsafe), `l` (local), `i` (instance), `s` (stroage).
    std::optional<Lang::PointerStorage> storage;

    /// `w` (writeable), `W` (not writeable) by default.
    std::optional<bool> writeability;

    // /// `0` by default.
    // std::optional<unsigned> address_space;
  };

  Kind kind;

  /// Is it wrapped in backticks, e.g. `` `foo` ``?
  /// NOTE: Wrapping turns a literal into an identifier, e.g. `` `this` ``.
  bool wrapped;

  /// The identifier value.
  std::variant<std::string, Lang::IDLiteral> value;

  /// Optional pointer suffixes for this ID, applicable to `Simple` and `C` only.
  /// One suffix: `T*`, two: `T**u` (unsafe pointer to pointer to `T`).
  std::vector<PointerSuffix> pointer_suffixes;

  ID(Placement plc,
     Kind kind,
     bool wrapped,
     std::variant<std::string, Lang::IDLiteral> value,
     std::vector<PointerSuffix> pointer_suffixes) :
      Base(plc),
      kind(kind),
      wrapped(wrapped),
      value(value),
      pointer_suffixes(pointer_suffixes) {}

  /// Return the string value, or convert the literal to string.
  std::string string() const {
    if (auto *string = std::get_if<std::string>(&value))
      return *string;
    else if (auto *literal = std::get_if<Lang::IDLiteral>(&value))
      return std::string(Lang::id_literal_string(*literal));
    else
      throw "BUG";
  }

  std::optional<Lang::IDLiteral> literal() const {
    if (std::get_if<std::string>(&value))
      return std::nullopt;
    else
      return std::get<Lang::IDLiteral>(value);
  }

  /// Check if the first codepoint matches `/[A-ZΑ-Ω]/`,
  /// would always return `false` if literal or _wrapped_.
  bool capitalized() const {
    if (wrapped)
      return false;

    auto string = std::get_if<std::string>(&value);
    if (!string)
      return false;

    auto cp = Util::UTF8::to_code_point((char8_t *)string->c_str());
    return (cp >= 0x41 && cp <= 0x5A) || (cp >= 0x0391 && cp <= 0x03A9);
  }

  /// Check if _cp_ matches `/[a-zA-Zα-ωΑ-Ω_0-9]/`.
  /// If wrapped, any Unicode codepoint other than U+60 (backtick) is valid.
  /// IDEA: Eliminate possible ambiguity?
  static bool check(char32_t cp, bool wrapped) {
    if (wrapped)
      return cp != 0x60;
    else
      return (cp >= 0x61 && cp <= 0x5A) || (cp >= 0x0391 && cp <= 0x03C9);
  }

  const char *token_name() const override { return "Id"; }

  const void print(std::ostream &o) const override {
    if (kind == Kind::C)
      o << '$';
    else if (kind == Kind::Intrinsic)
      o << '@';
    else if (kind == Kind::Symbol)
      o << ':';

    if (wrapped)
      o << '`';

    if (auto *string = std::get_if<std::string>(&value))
      o << string;
    else if (auto *literal = std::get_if<Lang::IDLiteral>(&value))
      o << Lang::id_literal_string(*literal);

    if (wrapped)
      o << '`';

    if (kind == Kind::Label)
      o << ':';
  };
};

/// An operator token, including Unicode Mathematical Operators block.
struct Op : Base {
  /// Is _c_ possibly an operator or its part?
  static bool check(wchar_t codepoint) {
    // Unicode Mathematical Operators block (U+2200–U+22FF).
    // See https://en.wikipedia.org/wiki/Mathematical_operators_and_symbols_in_Unicode.
    if (codepoint >= 0x2200 && codepoint <= 0x22ff)
      return true;

    switch (codepoint) {
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

  std::string name;
  Op(Placement plc, std::string name) : Base(plc), name(name) {}

  /// A protected unary operator can not be user-declared.
  bool is_protected_unnop() const {
    if (name[0] == '!')
      return true;

    if (name.back() == '=')
      return true;

    return (!name.compare("&") || !name.compare("*"));
  }

  /// A protected binary operator can not be user-declared.
  bool is_protected_binop() const {
    if (name[0] == '!')
      return true;

    return (!name.compare("~") || !name.compare("=") || !name.compare("==="));
  }

  /// A special assignment operator ends with `=`, e.g. `+=`; and it can be overloaded.
  /// Can also overload a special indexed setter, e.g. `[]+=(index, incr)` (ternary op).
  /// If not declared on a caller, would be lowered from `a X= b` to `a = a X b`,
  /// where `X` is the part preceding the assignment sign.
  bool is_special_assignment() const {
    if (name[0] != '!' && name.back() == '=' && name.length() > 1 &&
        !(name.compare(">=") || name.compare("<=") || name.compare("==") ||
          name.compare("~=") || name.compare("===")))
      return true;
    else
      return false;
  }

  const char *token_name() const override { return "Op"; }
  const void print(std::ostream &o) const override { o << name; };

private:
};

/// A well-known literal kind token, e.g. `\Bool`.
struct LiteralKind : Base {
  Lang::BeLiteralType kind;

  LiteralKind(Placement plc, Lang::BeLiteralType kind, std::string value) :
      Base(plc), kind(kind) {}

  const char *token_name() const override { return "Literal kind"; }

  const void print(std::ostream &o) const override {
    o << Lang::beliteral_type_string(kind);
  };
};

/// An identifier literal, e.g. `true`.
///
/// TODO: It may also have pointer params, e.g. `void*uw0`.
/// Move to `ID`?
struct IDLiteral : Base {
  Lang::IDLiteral value;
  IDLiteral(Placement plc, Lang::IDLiteral value) : Base(plc), value(value) {}
  const char *token_name() const override { return "IDLiteral"; }

  const void print(std::ostream &o) const override {
    o << Lang::id_literal_string(value);
  }
};

/// A bool literal, e.g. `true`.
struct BoolLiteral : Base {
  bool value;
  BoolLiteral(Placement plc, bool value) : Base(plc), value(value) {}
  const char *token_name() const override { return "BoolLiteral"; }
  const void print(std::ostream &o) const override { o << value; }
};

/// A numeric literal, e.g. `42`. Note that it doesn't include sign.
struct NumericLiteral : Base {
  /// TODO: Make it a sequence of digits instead.
  unsigned long long value;

  NumericLiteral(Placement plc, long long value) : Base(plc), value(value) {}
  const char *token_name() const override { return "NumericLiteral"; }
  const void print(std::ostream &o) const override { o << value; }
};

/// A string literal, e.g. `"foo"`.
struct StringLiteral : Base {
  std::string value;

  StringLiteral(Placement plc, std::string value) : Base(plc), value(value) {}

  const char *token_name() const override { return "StringLiteral"; }

  const void print(std::ostream &o) const override { o << '"' << value << '"'; };
};

using Any = std::variant<
    Comment,
    Keyword,
    Punct,
    ID,
    Op,
    LiteralKind,
    IDLiteral,
    BoolLiteral,
    NumericLiteral,
    StringLiteral>;

} // namespace Token
} // namespace Onyx
} // namespace Phoenix
} // namespace Fancysoft

namespace std {
template <> struct less<Fancysoft::Phoenix::Onyx::Token::ID> {
  /// Compare two ID tokens.
  bool operator()(
      const Fancysoft::Phoenix::Onyx::Token::ID &k1,
      const Fancysoft::Phoenix::Onyx::Token::ID &k2) const {
    return k1.kind != k2.kind || k1.value < k2.value;
  }
};
} // namespace std
