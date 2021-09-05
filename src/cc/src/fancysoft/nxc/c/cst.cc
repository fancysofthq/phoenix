#include <fmt/ostream.h>

#include "fancysoft/nxc/c/cst.hh"

namespace Fancysoft::NXC::C {

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

void CST::Node::Type::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}\n{2}Pointer depth: {4}\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->id.inspect(),
      this->pointer_depth);
}

void CST::Node::Proto::ArgDecl::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}\n{2}Type:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->name.has_value() ? this->name->inspect() : "");

  this->type->inspect(stream, indent + 1);
}

void CST::Node::Proto::inspect(
    std::ostream &stream, unsigned short indent) const {
  fmt::print(
      stream,
      "{0}{1}\n{2}Id: {3}\n{2}Return type:\n",
      node_prefix(indent),
      node_name(),
      attribute_prefix(indent),
      this->name.inspect());

  this->return_type->inspect(stream, indent + 1);

  fmt::print(stream, "{0}Arguments:\n", attribute_prefix(indent));

  for (auto &arg : this->args) {
    arg->inspect(stream, indent + 1);
  }
}

} // namespace Fancysoft::NXC::C
