#include <fmt/ostream.h>

#include "fancysoft/nxc/onyx/cst.hh"

namespace Fancysoft::NXC::Onyx {

static std::string node_prefix(unsigned short indent) {
  return std::string(indent * 3, ' ') + "|-";
}

static std::string attribute_prefix(unsigned short indent) {
  return std::string(indent * 3 + 3, ' ');
}

void CST::Root::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(stream, "{0}{1}\n", node_prefix(indent), node_name());

  for (auto &node : this->children) {
    std::visit(
        [&stream, indent](auto &&node) { node.inspect(stream, indent + 1); },
        *node.get());
  }
}

void CST::Node::Extern::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(stream, "{0}{1}\n", node_prefix(indent), node_name());
  this->block->cst->inspect(stream, indent + 1);
}

void CST::Node::StringLiteral::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Token: {3}\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->token.inspect());
}

void CST::Node::CCall::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Callee id: {3}\n{2}Arguments:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->callee_id.inspect());

  for (auto &node : this->arguments) {
    std::visit(
        [&stream, indent](auto &&node) { node.inspect(stream, indent + 1); },
        *node.get());
  }
}

void CST::Node::VarDecl::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Final: {3}\n{2}Var id: {4}\n{2}Value:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->is_final(),
      this->id.inspect());

  std::visit(
      [&stream, indent](auto &&node) { node.inspect(stream, indent + 1); },
      *this->value);
}

void CST::Node::ExplicitSafety::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Inline: {3}\n{2}Expressions:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->is_inline());

  for (auto &node : this->expressions) {
    std::visit(
        [&stream, indent](auto &&node) { node.inspect(stream, indent + 1); },
        *node.get());
  }
}

void CST::Node::UnOp::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Operator: ",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent));

  std::visit(
      [&stream](auto &&token) { token.inspect(stream); }, this->operator_);

  fmt::print(stream, "\n{0}Operand:\n", attribute_prefix(indent));

  std::visit(
      [&stream, indent](auto &&node) { node.inspect(stream, indent + 1); },
      *this->operand.get());
}

void CST::Node::Id::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->id.inspect());
}

} // namespace Fancysoft::NXC::Onyx
