#include "fancysoft/nxc/c/hlir.hh"
#include <optional>

namespace Fancysoft::NXC::C {

HLIR::TypeRef HLIR::TypeRef::compile(std::shared_ptr<AST::TypeRef> ast) {
  auto built_in_type = _find_built_in_type(ast->id_token.id);

  if (built_in_type) {
    return TypeRef(built_in_type.value(), ast->pointer_depth());
  } else {
    throw Panic(
        "Use of undeclared C type `" + ast->id_token.id + "`",
        ast->id_token.placement);
  }
}

void HLIR::TypeRef::write(std::ostream &out) const {
  out << _built_in_type_to_hlir(type);

  for (int i = 0; i < pointer_depth; i++)
    out << '*';
}

HLIR::FuncDecl::ArgDecl HLIR::FuncDecl::ArgDecl::compile(
    std::shared_ptr<const AST::FuncDecl::ArgDecl> ast) {
  auto type = TypeRef::compile(ast->type_node);

  if (ast->id_token.has_value()) {
    return ArgDecl(type, ast->id_token->id);
  } else {
    return ArgDecl(type, std::nullopt);
  }
}

void HLIR::FuncDecl::ArgDecl::write(std::ostream &out) const {
  this->type.write(out);
}

HLIR::FuncDecl
HLIR::FuncDecl::compile(std::shared_ptr<const AST::FuncDecl> ast) {
  auto return_type = TypeRef::compile(ast->return_type_node);

  auto id = ast->id_token.id;

  if (_is_reserved(id))
    throw Panic(
        "Can not use reserved C keyword `" + id + "` as C function id",
        ast->id_token.placement);

  std::vector<ArgDecl> args;
  std::set<std::string> arg_names;

  for (auto &arg : ast->arg_nodes) {
    std::optional<std::string> arg_name;

    if (arg->id_token.has_value()) {
      arg_name = arg->id_token.value().id;

      if (arg_names.contains(arg_name.value())) {
        // TODO: Point at current and previous declarations.
        throw Panic(
            "Already declared argument with this name",
            arg->id_token.value().placement);
      } else {
        arg_names.insert(arg_name.value());
      }
    }

    args.push_back(ArgDecl::compile(arg));
  }

  return FuncDecl(return_type, id, args);
}

std::shared_ptr<HLIR::FuncDecl> HLIR::find_func_decl(std::string id) {
  return *Util::Map::get_if(_func_decls, id);
}

std::shared_ptr<HLIR::FuncDecl>
HLIR::add_func_decl(std::shared_ptr<AST::FuncDecl> decl) {
  Token::Id id_token = decl->id_token;
  auto id = id_token.id;

  if (find_func_decl(id))
    throw Panic(
        "Already declared function with id `" + id + "`", id_token.placement);

  auto ptr = std::make_shared<FuncDecl>(FuncDecl::compile(decl));
  _func_decls[id] = ptr;

  return ptr;
}

void HLIR::compile(Block *block) {
  for (auto &child : block->ast()->chidren()) {
    std::visit(
        [this](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<AST::FuncDecl>>) {
            add_func_decl(arg);
          } else
            static_assert(
                Util::Variant::always_false_v<T>, "non-exhaustive visitor!");
        },
        child);
  }
}

void HLIR::write(std::ostream &out) const {
  for (auto &pair : _func_decls) {
    out << "declare ";
    pair.second->return_type.write(out);
    out << " @$" << pair.second->id << "(";

    bool first = true;
    for (auto &arg : pair.second->args) {
      if (!first)
        out << ", ";
      else
        first = false;

      arg.write(out);
    }

    out << ")\n";
  }
}

std::optional<HLIR::BuiltInType> HLIR::_find_built_in_type(std::string id) {
  if (!id.compare("void"))
    return BuiltInType::Void;
  else if (!id.compare("char"))
    return BuiltInType::Char;
  else if (!id.compare("unsigned int"))
    return BuiltInType::UnsignedInt;
  else
    return std::nullopt;
}

const char *HLIR::_built_in_type_to_hlir(HLIR::BuiltInType type) {
  switch (type) {
  case BuiltInType::Void:
    return "$void";
  case BuiltInType::Char:
    return "$char";
  case BuiltInType::UnsignedInt:
    return "$uint";
  }
}

bool HLIR::_is_reserved(std::string id) {
  if (!id.compare("void"))
    return true;
  else if (!id.compare("char"))
    return true;
  else if (!id.compare("unsigned int"))
    return true;
  else if (!id.compare("const"))
    return true;
  else
    return false;
}

} // namespace Fancysoft::NXC::C
