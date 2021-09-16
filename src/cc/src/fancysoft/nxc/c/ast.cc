#include <fmt/ostream.h>

#include "fancysoft/nxc/c/ast.hh"

namespace Fancysoft::NXC::C {

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
}

void AST::TypeRef::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}\n{2}Pointer depth: {4}\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->id_token.Token::inspect(),
      this->pointer_depth());
}

void AST::FuncDecl::ArgDecl::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}\n{2}Type:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->id_token.has_value() ? this->id_token->Token::inspect() : "");

  this->type_node->inspect(stream, indent + 1);
}

void AST::FuncDecl::inspect(std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}\n{2}Return type:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->id_token.Token::inspect());

  this->return_type_node->inspect(stream, indent + 1);

  fmt::print(stream, "{0}Arguments:\n", attribute_prefix(indent));

  for (auto &arg : this->arg_nodes) {
    arg->inspect(stream, indent + 1);
  }
}

} // namespace Fancysoft::NXC::C
