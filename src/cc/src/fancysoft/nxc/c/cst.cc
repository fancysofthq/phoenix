#include <fmt/ostream.h>

#include "fancysoft/nxc/c/cst.hh"

namespace Fancysoft::NXC::C {

void CST::TypeRef::trace(std::ostream &o) const {
  fmt::print(
      o,
      "<{0} {1}{2}>",
      node_name(),
      id.value,
      std::string(pointer_depth(), '*'));
}

void CST::TypeRef::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << id.value;

  if (pointer_depth() > 0)
    o << ' ' << std::string(pointer_depth(), '*');
}

void CST::FuncDecl::Arg::trace(std::ostream &o) const {
  o << '<' << node_name() << ' ';

  if (id)
    o << id->value << ' ';

  type->trace(o);
  o << '>';
}

void CST::FuncDecl::Arg::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  type->print(o);

  if (id)
    o << ' ' << id->value;
}

void CST::FuncDecl::trace(std::ostream &o) const {
  o << '<' << node_name() << id.value << '>';
}

void CST::FuncDecl::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  return_type->print(o);
  o << ' ' << id.value << '(';

  bool first = true;
  for (auto &arg : args) {
    if (first)
      first = false;
    else
      o << ", ";

    arg->print(o);
  }

  o << ");";
}

void CST::print(std::ostream &o, unsigned indent) const {
  for (auto &child : _children) {
    std::visit([&o, indent](auto &node) { node->print(o, indent); }, child);
  }
}

} // namespace Fancysoft::NXC::C
