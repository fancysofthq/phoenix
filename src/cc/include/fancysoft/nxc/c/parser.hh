#pragma once

#include "../parser.hh"
#include "./cst.hh"
#include "./lexer.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

struct Parser : NXC::Parser<Lexer, Token::Any> {
  using NXC::Parser<Lexer, Token::Any>::Parser;
  void parse(std::shared_ptr<CST::Root>, bool single_expression);

protected:
  inline const char *_debug_name() const override { return "C/Parser"; }

private:
  std::shared_ptr<CST::Node::Type> _parse_type();
  std::shared_ptr<CST::Node::Proto::ArgDecl> _parse_arg_decl();
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
