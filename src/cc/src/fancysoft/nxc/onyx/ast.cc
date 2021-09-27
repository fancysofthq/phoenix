#include <fmt/ostream.h>

#include "fancysoft/nxc/onyx/ast.hh"

namespace Fancysoft::NXC::Onyx {

static std::string node_prefix(unsigned short indent) {
  return std::string(indent * 3, ' ') + "|-";
}

static std::string attribute_prefix(unsigned short indent) {
  return std::string(indent * 3 + 3, ' ');
}

void AST::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(stream, "{0}{1}\n", node_prefix(indent), node_name());

  for (auto &node : this->_children) {
    std::visit(
        [&stream, indent](auto &&node) { node->inspect(stream, indent + 1); },
        node);
  }

  stream << "\n";
}

void AST::ExternDirective::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(stream, "{0}{1}\n", node_prefix(indent), node_name());
  this->block->ast()->inspect(stream, indent + 1);
}

// void AST::StringLiteral::inspect(
//     std::ostream &stream, unsigned short indent) const {
//   fmt::print(
//       stream,
//       "{0}{1}\n{2}Token: {3}\n",
//       node_prefix(indent),
//       node_name(),
//       attribute_prefix(indent),
//       this->token.Token::inspect());
// }

void AST::CStringLiteral::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Token: {3}\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->token.Token::inspect());
}

void AST::CCall::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Callee id: {3}\n{2}Arguments:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->callee.Token::inspect());

  for (auto &node : this->arguments) {
    std::visit(
        [&stream, indent](auto &&node) { node->inspect(stream, indent + 1); },
        node);
  }
}

void AST::VarDecl::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Final: {3}\n{2}Var id: {4}\n{2}Value:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->is_final(),
      this->id_token.Token::inspect());

  std::visit(
      [&stream, indent](auto &&node) { node->inspect(stream, indent + 1); },
      *this->value);
}

void AST::ExplicitSafetyStatement::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Safety: {3}\n{2}Expressions:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->safety());

  std::visit(
      [&stream, indent](auto &&node) { node->inspect(stream, indent + 1); },
      this->value);
}

void AST::UnOp::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Operator: {3}\n{2}Operand:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      operator_.Token::inspect());

  std::visit(
      [&stream, indent](auto &&node) { node->inspect(stream, indent + 1); },
      this->operand);
}

void AST::Id::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->token.Token::inspect());
}

} // namespace Fancysoft::NXC::Onyx
