#pragma once

#include <istream>
#include <optional>
#include <variant>

#include "../parser.hh"
#include "./cst.hh"
#include "./lexer.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {

struct Parser : NXC::Parser<Lexer, Token::Any> {
  using NXC::Parser<Lexer, Token::Any>::Parser;
  void parse(std::shared_ptr<CST::Root>);

protected:
  inline const char *_debug_name() const override { return "Parser"; }

private:
  std::shared_ptr<CST::Node::Expression> _parse_expression(bool allow_empty);
};

} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
