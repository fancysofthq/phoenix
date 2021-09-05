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

namespace Keyword {

struct Extern : Base {
  using Base::Base;

  const char *name() const override { return "<Keyword/Extern>"; }
  const void print(std::ostream &stream) const override { stream << "extern"; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Let : Base {
  using Base::Base;

  const char *name() const override { return "<Keyword/Let>"; }
  const void print(std::ostream &stream) const override { stream << "let"; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Final : Base {
  using Base::Base;

  const char *name() const override { return "<Keyword/Final>"; }
  const void print(std::ostream &stream) const override { stream << "final"; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct UnsafeBang : Base {
  using Base::Base;

  const char *name() const override { return "<Keyword/Unsafe!>"; }

  const void print(std::ostream &stream) const override {
    stream << "unsafe!";
  };

  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct FragileBang : Base {
  using Base::Base;

  const char *name() const override { return "<Keyword/Fragile!>"; }

  const void print(std::ostream &stream) const override {
    stream << "fragile!";
  };

  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

} // namespace Keyword

namespace Punct {

struct EOL : Base {
  using Base::Base;

  const char *name() const override { return "<Punct/EOL>"; }
  const void print(std::ostream &stream) const override { stream << '\n'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Space : Base {
  using Base::Base;

  const char *name() const override { return "<Punct/Space>"; }
  const void print(std::ostream &stream) const override { stream << ' '; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct OpenParen : Base {
  using Base::Base;

  const char *name() const override { return "<Punct/OpenParen>"; }
  const void print(std::ostream &stream) const override { stream << '('; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct CloseParen : Base {
  using Base::Base;

  const char *name() const override { return "<Punct/CloseParen>"; }
  const void print(std::ostream &stream) const override { stream << ')'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Comma : Base {
  using Base::Base;

  const char *name() const override { return "<Punct/Comma>"; }
  const void print(std::ostream &stream) const override { stream << ','; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

} // namespace Punct

namespace BuiltInOp {

struct Assign : Base {
  using Base::Base;

  const char *name() const override { return "<Op/Assign>"; }
  const void print(std::ostream &stream) const override { stream << '='; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct AddressOf : Base {
  using Base::Base;

  const char *name() const override { return "<Op/PointerOf>"; }
  const void print(std::ostream &stream) const override { stream << '&'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

} // namespace BuiltInOp

/// A C identifier.
struct CId : Base {
  std::string id;
  CId(Placement plc, std::string id) : Base(plc), id(id) {}

  const char *name() const override { return "<CId>"; }

  const void print(std::ostream &stream) const override {
    stream << '$' << id;
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<CId `$" << id << "`>";
  };

  const std::string inspect() const override { return "<CId `$" + id + "`>"; };
};

/// An identifier, e.g. @c foo .
struct Id : Base {
  std::string value;
  Id(Placement plc, std::string value) : Base(plc), value(value) {}

  const char *name() const override { return "<Id>"; }
  const void print(std::ostream &stream) const override { stream << value; };

  const void inspect(std::ostream &stream) const override {
    stream << "<Id `" << value << "`>";
  };

  const std::string inspect() const override { return "<Id `" + value + "`>"; };
};

/// A generic operator, e.g. @c / .
struct Op : Base {
  std::string value;
  Op(Placement plc, std::string value) : Base(plc), value(value) {}

  const char *name() const override { return "<Op>"; }
  const void print(std::ostream &stream) const override { stream << value; };

  const void inspect(std::ostream &stream) const override {
    stream << "<Op `" << value << "`>";
  };

  const std::string inspect() const override { return "<Op `" + value + "`>"; };
};

/// A string literal, e.g. @c "foo" .
struct StringLiteral : Base {
  std::string value;

  StringLiteral(Placement plc, std::string value) : Base(plc), value(value) {}

  const char *name() const override { return "<StringLiteral>"; }

  const void print(std::ostream &stream) const override {
    stream << '"' << value << "'";
  };

  const void inspect(std::ostream &stream) const override {
    stream << "<StringLiteral \"" << value << "\">";
  };

  const std::string inspect() const override {
    return "<StringLiteral \"" + value + "\">";
  };
};

using Any = std::variant<
    Keyword::Extern,
    Keyword::Let,
    Keyword::Final,
    Keyword::UnsafeBang,

    Punct::EOL,
    Punct::Space,
    Punct::OpenParen,
    Punct::CloseParen,
    Punct::Comma,

    BuiltInOp::Assign,
    BuiltInOp::AddressOf,

    CId,
    Id,
    Op,
    StringLiteral>;

} // namespace Token

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
