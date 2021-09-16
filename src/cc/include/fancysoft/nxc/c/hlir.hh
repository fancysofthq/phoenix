#pragma once

#include <assert.h>
#include <memory>
#include <ostream>

#include "../../util/map.hh"
#include "./ast.hh"
#include "./block.hh"

namespace Fancysoft {
namespace NXC {

namespace Onyx {
struct HLIR;
}

namespace C {

struct HLIR {
  enum BuiltInType {
    Void,
    Char,
    UnsignedInt,
  };

  struct TypeRef {
    const BuiltInType type;
    const unsigned short pointer_depth;

    static TypeRef compile(std::shared_ptr<AST::TypeRef> ast);

    TypeRef(BuiltInType type, unsigned short pointer_depth) :
        type(type), pointer_depth(pointer_depth) {}

    void write(std::ostream &) const;
  };

  struct FuncDecl {
    struct ArgDecl {
      const TypeRef type;
      const std::optional<std::string> id;

      static ArgDecl compile(std::shared_ptr<const AST::FuncDecl::ArgDecl> ast);

      ArgDecl(TypeRef type, std::optional<std::string> id) :
          type(type), id(id) {}

      void write(std::ostream &) const;
    };

    const TypeRef return_type;
    const std::string id;
    const std::vector<ArgDecl> args;

    static FuncDecl compile(std::shared_ptr<const AST::FuncDecl>);

    FuncDecl(TypeRef return_type, std::string id, std::vector<ArgDecl> args) :
        return_type(return_type), id(id), args(args) {}
  };

  const std::shared_ptr<Onyx::HLIR> parent_hlir;

  std::shared_ptr<FuncDecl> find_func_decl(std::string id);
  std::shared_ptr<FuncDecl> add_func_decl(std::shared_ptr<AST::FuncDecl> decl);

  /// Compile a *block* of C code into self.
  void compile(Block *block);

  HLIR(std::shared_ptr<Onyx::HLIR> parent_hlir) : parent_hlir(parent_hlir) {}

  /// Write the HLIR in human-readable format into an output stream.
  void write(std::ostream &) const;

private:
  std::map<std::string, std::shared_ptr<FuncDecl>> _func_decls;
  static std::optional<BuiltInType> _find_built_in_type(std::string id);
  static const char *_built_in_type_to_hlir(BuiltInType);
  static bool _is_reserved(std::string id);
};

} // namespace C
} // namespace NXC
} // namespace Fancysoft
