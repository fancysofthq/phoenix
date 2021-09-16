#pragma once

#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <variant>
#include <vector>

#include "../../util/variant.hh"
#include "../node.hh"
#include "../panic.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

/// A C Abstract Syntax Tree.
struct AST : NXC::Node {
  struct TypeRef;
  struct FuncDecl;

  using TopLevelNode = std::variant<std::shared_ptr<FuncDecl>>;

  /// A type reference, e.g. `int` or `const unsigned int **`.
  struct TypeRef : NXC::Node {
    // const Token::Keyword modifier; // TODO:
    const Token::Id id_token;
    const std::vector<Token::Op> pointer_tokens;

    TypeRef(Token::Id id_token, std::vector<Token::Op> pointer_tokens) :
        id_token(id_token), pointer_tokens(pointer_tokens) {}

    int pointer_depth() const { return pointer_tokens.size(); }
    const char *node_name() const override { return "<C/TypeRef>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
  };

  /// A C function prototype declaration.
  struct FuncDecl : NXC::Node {
    struct ArgDecl : NXC::Node {
      std::shared_ptr<TypeRef> type_node;
      const std::optional<Token::Id> id_token;

      ArgDecl(
          std::shared_ptr<TypeRef> type_node,
          std::optional<Token::Id> id_token) :
          type_node(type_node), id_token(id_token) {}

      const char *node_name() const override { return "<C/ArgDecl>"; }
      void inspect(std::ostream &, unsigned short indent = 0) const override;
    };

    std::shared_ptr<TypeRef> return_type_node;
    const Token::Id id_token;
    std::vector<std::shared_ptr<ArgDecl>> arg_nodes;

    FuncDecl(
        std::shared_ptr<TypeRef> return_type_node,
        Token::Id id_token,
        std::vector<std::shared_ptr<ArgDecl>> arg_nodes) :
        return_type_node(return_type_node),
        id_token(id_token),
        arg_nodes(arg_nodes) {}

    const char *node_name() const override { return "<C/FuncDecl>"; }
    void inspect(std::ostream &, unsigned short indent = 0) const override;
  };

  /// Append a child node.
  void add_child(TopLevelNode node) { _children.push_back(node); };

  /// Return a constant reference to the children nodes vector.
  const std::vector<TopLevelNode> &chidren() const { return _children; }

  const char *node_name() const override { return "<C/AST>"; }
  void inspect(std::ostream &, unsigned short indent = 0) const override;

private:
  std::vector<TopLevelNode> _children;
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
