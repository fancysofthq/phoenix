#pragma once

#include <fancysoft/util/variant.hh>

#include "../node.hh"
#include "./token.hh"

namespace Fancysoft {
namespace NXC {
namespace C {

/// A C Concrete Syntax Tree.
struct CST : NXC::Node {
  struct TypeRef;
  struct FuncDecl;

  using TopLevelNode = std::variant<std::shared_ptr<FuncDecl>>;

  /// A type reference, e.g. `int` or `const unsigned int **`.
  struct TypeRef : NXC::Node {
    // const Token::Keyword qualifier; // TODO:
    const Token::Id id;
    const std::vector<Token::Op> pointer_tokens;

    TypeRef(Token::Id id, std::vector<Token::Op> pointer_tokens) :
        id(id), pointer_tokens(pointer_tokens) {}

    int pointer_depth() const { return pointer_tokens.size(); }

    const char *node_name() const override { return "C/TypeRef"; }
    // void inspect(std::ostream &, unsigned short indent = 0) const override;
    void trace(std::ostream &) const override;
    void print(std::ostream &, unsigned indent = 0) const override;
  };

  /// A C function prototype declaration.
  struct FuncDecl : NXC::Node {
    struct Arg : NXC::Node {
      std::shared_ptr<TypeRef> type;
      const std::optional<Token::Id> id;

      Arg(std::shared_ptr<TypeRef> type, std::optional<Token::Id> id) :
          type(type), id(id) {}

      const char *node_name() const override { return "C/FuncDecl/Arg"; }
      // void inspect(std::ostream &, unsigned short indent = 0) const override;
      void trace(std::ostream &) const override;
      void print(std::ostream &, unsigned indent = 0) const override;
    };

    struct VArg : NXC::Node {
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
    // void inspect(std::ostream &, unsigned short indent = 0) const override;
    void trace(std::ostream &) const override;
    void print(std::ostream &, unsigned indent = 0) const override;
  };

  /// Append a child node.
  void add_child(TopLevelNode node) { _children.push_back(node); };

  /// Return a constant reference to the children nodes vector.
  const std::vector<TopLevelNode> &chidren() const { return _children; }

  const char *node_name() const override { return "C/CST"; }
  // void inspect(std::ostream &, unsigned short indent = 0) const override;
  void print(std::ostream &, unsigned indent = 0) const override;

private:
  std::vector<TopLevelNode> _children;
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
