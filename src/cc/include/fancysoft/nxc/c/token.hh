#pragma once

#include <string>
#include <variant>

#include "../token.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

namespace Token {

using Base = NXC::Token;

namespace Punct {

struct EOL : Base {
  using Base::Base;

  const char *name() const override { return "<C/Punct/EOL>"; }
  const void print(std::ostream &stream) const override { stream << '\n'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Space : Base {
  using Base::Base;

  const char *name() const override { return "<C/Punct/Space>"; }
  const void print(std::ostream &stream) const override { stream << ' '; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Comma : Base {
  using Base::Base;

  const char *name() const override { return "<C/Punct/Comma>"; }
  const void print(std::ostream &stream) const override { stream << ','; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct Semi : Base {
  using Base::Base;

  const char *name() const override { return "<C/Punct/Semi>"; }
  const void print(std::ostream &stream) const override { stream << ';'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct OpenParen : Base {
  using Base::Base;

  const char *name() const override { return "<C/Punct/OpenParen>"; }
  const void print(std::ostream &stream) const override { stream << '('; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

struct CloseParen : Base {
  using Base::Base;

  const char *name() const override { return "<C/Punct/CloseParen>"; }
  const void print(std::ostream &stream) const override { stream << ')'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

} // namespace Punct

namespace Op {

struct Asterisk : Base {
  using Base::Base;

  const char *name() const override { return "<C/Op/Asterisk>"; }
  const void print(std::ostream &stream) const override { stream << '*'; };
  const void inspect(std::ostream &stream) const override { stream << name(); };
  const std::string inspect() const override { return name(); };
};

}; // namespace Op

struct Id : Base {
  std::string value;

  Id(Placement placement, std::string value) : Base(placement), value(value) {}

  const char *name() const override { return "<C/Id>"; }
  const void print(std::ostream &stream) const override { stream << value; };

  const void inspect(std::ostream &stream) const override {
    stream << "<C/Id `" << value << "`>";
  };

  const std::string inspect() const override {
    return "<C/Id `" + value + "`>";
  };
};

using Any = std::variant<
    Punct::EOL,
    Punct::Space,
    Punct::Comma,
    Punct::Semi,
    Punct::OpenParen,
    Punct::CloseParen,

    Op::Asterisk,

    Id>;

} // namespace Token
} // namespace C
} // namespace NXC
} // namespace Fancysoft
