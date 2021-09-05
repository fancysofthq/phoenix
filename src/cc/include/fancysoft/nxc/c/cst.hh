#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "../node.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

/// A C (language) Concrete Syntax Tree.
namespace CST {

namespace Node {
struct Type;
struct Proto;
} // namespace Node

/// A type, e.g. `int` or `char*`.
/// TODO: `unsigned int` is also a single id.
struct Node::Type : NXC::Node {
  const Token::Id id;

  /// E.g. @c char** has 2.
  const unsigned short pointer_depth;

  Type(Token::Id id, unsigned short pointer_depth) :
      id(id), pointer_depth(pointer_depth) {}

  const char *node_name() const override { return "<C/Type>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

/// A C function prototype declaration.
struct Node::Proto : NXC::Node {
  struct ArgDecl : NXC::Node {
    const std::shared_ptr<Type> type;
    const std::optional<Token::Id> name;

    ArgDecl(std::shared_ptr<Type> type, std::optional<Token::Id> name) :
        type(type), name(name) {}

    const char *node_name() const override { return "<C/ArgDecl>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
  };

  const std::shared_ptr<Type> return_type;
  const Token::Id name;
  const std::vector<std::shared_ptr<ArgDecl>> args;

  Proto(
      std::shared_ptr<Type> return_type,
      const Token::Id name,
      std::vector<std::shared_ptr<ArgDecl>> args) :
      return_type(return_type), name(name), args(args) {}

  const char *node_name() const override { return "<C/Proto>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

using RootChildNode = std::variant<Node::Proto>;

struct Root : NXC::Node {
  std::vector<std::shared_ptr<RootChildNode>> children;
  const char *node_name() const override { return "<C/Root>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;
};

} // namespace CST
} // namespace C
} // namespace NXC
} // namespace Fancysoft
