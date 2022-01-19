#pragma once

#include "../util/node.hh"
#include "../util/variant.hh"
#include "./token.hh"

namespace Fancysoft {
namespace Phoenix {
namespace C {

/// A C Concrete Syntax Tree.
struct CST : Util::Node {
  struct TypeRef;
  struct FuncDecl;

  using TopLevelNode = std::variant<std::shared_ptr<FuncDecl>>;

  /// A type reference, e.g. `int` or `const unsigned int **`.
  struct TypeRef : Util::Node {
    // const Token::Keyword qualifier; // TODO:
    const Token::Id id;
    const std::vector<Token::Op> pointer_tokens;

    TypeRef(Token::Id id, std::vector<Token::Op> pointer_tokens) :
        id(id), pointer_tokens(pointer_tokens) {}

    int pointer_depth() const { return pointer_tokens.size(); }

    const char *node_name() const override { return "C/TypeRef"; }
    void trace(std::ostream &) const override;
    void print(std::ostream &, unsigned indent = 0) const override;
  };

  /// A C function prototype declaration.
  struct FuncDecl : Util::Node {
    struct Arg : Util::Node {
      std::shared_ptr<TypeRef> type;
      const std::optional<Token::Id> id;

      Arg(std::shared_ptr<TypeRef> type, std::optional<Token::Id> id) :
          type(type), id(id) {}

      const char *node_name() const override { return "C/FuncDecl/Arg"; }
      void trace(std::ostream &) const override;
      void print(std::ostream &, unsigned indent = 0) const override;
    };

    struct VArg : Util::Node {
      const Token::Punct token;
      VArg(Token::Punct token) : token(token) {}
      const char *node_name() const override { return "C/FuncDecl/VArg"; }

      void print(std::ostream &o, unsigned indent = 0) const override {
        print_tab(o, indent);
        o << "...";
      }
    };

    std::shared_ptr<TypeRef> return_type;
    const Token::Id id;
    std::vector<std::shared_ptr<Arg>> args;
    std::shared_ptr<VArg> varg;

    FuncDecl(
        std::shared_ptr<TypeRef> return_type,
        Token::Id id,
        std::vector<std::shared_ptr<Arg>> args,
        std::shared_ptr<VArg> varg) :
        return_type(return_type), id(id), args(args), varg(varg) {}

    const char *node_name() const override { return "C/FuncDecl"; }
    void trace(std::ostream &) const override;
    void print(std::ostream &, unsigned indent = 0) const override;
  };

  /// Append a child top-level node.
  void append(TopLevelNode node) { _children.push_back(node); };

  /// Return a constant reference to the children top-level nodes vector.
  const std::vector<TopLevelNode> &children() const { return _children; }

  const char *node_name() const override { return "C/CST"; }
  void print(std::ostream &, unsigned indent = 0) const override;

private:
  std::vector<TopLevelNode> _children;
};

} // namespace C
} // namespace Phoenix
} // namespace Fancysoft
