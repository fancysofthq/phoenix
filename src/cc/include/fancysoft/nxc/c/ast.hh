#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "../node.hh"
#include "../panic.hh"
#include "./cst.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

/// A C (language) Abstract Syntax Tree per Onyx unit.
/// It contains declarations and concrete definitions.
namespace AST {

namespace Node {

enum class BuiltInType {
  Void,
  Char,
};

struct Type;
struct Proto;

} // namespace Node

struct Node::Type : NXC::Node {
  const std::shared_ptr<CST::Node::Type> cst_node;
  Type(std::shared_ptr<CST::Node::Type> cst_node) : cst_node(cst_node) {}

  BuiltInType id() const {
    if (cst_node->id.value == "void") {
      return BuiltInType::Void;
    } else if (cst_node->id.value == "char") {
      return BuiltInType::Char;
    } else {
      throw Panic("Unrecognized C type: " + cst_node->id.value);
    }
  };

  unsigned short pointer_depth() const { return cst_node->pointer_depth; };

  const char *node_name() const override { return "<C/Type>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

struct Node::Proto : NXC::Node {
  struct ArgDecl : NXC::Node {
    std::shared_ptr<CST::Node::Proto::ArgDecl> cst_node;

    ArgDecl(std::shared_ptr<CST::Node::Proto::ArgDecl> cst_node) :
        cst_node(cst_node) {}

    Type type() const { return Type(cst_node->type); };

    std::optional<std::string> id() const {
      if (cst_node->name.has_value())
        return cst_node->name.value().value;
      else
        return std::nullopt;
    };

    const char *node_name() const override { return "<C/Proto/ArgDecl>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
  };

  const std::shared_ptr<CST::Node::Proto> cst_node;

  Proto(std::shared_ptr<CST::Node::Proto> cst_node) : cst_node(cst_node) {}

  Type return_type() const { return Type(cst_node->return_type); };
  std::string id() const { return cst_node->name.value; };

  std::vector<ArgDecl> arguments() const {
    std::vector<ArgDecl> vec;

    for (auto &node : cst_node->args) {
      vec.push_back(ArgDecl(node));
    }

    return vec;
  };

  const char *node_name() const override { return "<C/Proto>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

struct Root {
  std::map<std::string, std::shared_ptr<AST::Node::Proto>> prototypes;

  // const char *node_name() const override { return "<C/Root>"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
};

} // namespace AST
} // namespace C
} // namespace NXC
} // namespace Fancysoft
