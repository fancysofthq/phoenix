#include <iostream>
#include <string>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>

#include <fancysoft/util/variant.hh>

#include "fancysoft/nxc/exception.hh"
#include "fancysoft/nxc/logger.hh"
#include "fancysoft/nxc/mlir.hh"

namespace Fancysoft::NXC {

MLIR::MLIR(const Onyx::AST *ast, Program *program) {
  Util::logger.trace("MLIR") << "MLIR()\n";

  for (auto &node : ast->children()) {
    std::visit(
        [this](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<
                            T,
                            std::shared_ptr<Onyx::AST::ExternDirective>>) {
            _top_level_scope->compile_extern_directive(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<Onyx::AST::VarDecl>>) {
            _top_level_scope->compile_var_decl(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<Onyx::AST::Call>>) {
            _top_level_scope->compile_call(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<Onyx::AST::CCall>>) {
            _top_level_scope->compile_c_call(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<
                                       Onyx::AST::ExplicitSafetyStatement>>) {
            _top_level_scope->compile_rval(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<Onyx::AST::FuncDecl>>) {
            _top_level_scope->compile_func_decl(arg);
          } else
            static_assert(Util::Variant::false_t<T>, "Non-exhaustive visitor!");
        },
        node);
  }
}

MLIR::_CTypeRef MLIR::_CTypeRef::compile(std::shared_ptr<C::AST::TypeRef> ast) {
  Util::logger.trace({"MLIR", "_CTypeRef"})
      << __builtin_FUNCTION() << '(' << ast->trace() << ")\n";

  auto built_in_type = _search_c_built_in_type(ast->id_token.id);

  if (built_in_type) {
    return _CTypeRef(built_in_type.value(), ast->pointer_depth());
  } else {
    throw Panic(
        "Use of undeclared C type `" + ast->id_token.id + "`",
        ast->id_token.placement);
  }
}

#pragma region _CFuncDecl

MLIR::_CFuncDecl::ArgDecl MLIR::_CFuncDecl::ArgDecl::compile(
    std::shared_ptr<const C::AST::FuncDecl::ArgDecl> ast) {
  Util::logger.trace({"MLIR", "_CFuncDecl", "ArgDecl"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  auto type = _CTypeRef::compile(ast->type_node);

  if (ast->id_token.has_value()) {
    return ArgDecl(type, ast->id_token->id);
  } else {
    return ArgDecl(type, std::nullopt);
  }
}

MLIR::_CFuncDecl
MLIR::_CFuncDecl::compile(std::shared_ptr<const C::AST::FuncDecl> ast) {
  Util::logger.trace({"MLIR", "_CFuncDecl"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  auto return_type = _CTypeRef::compile(ast->return_type_node);

  auto id = ast->id_token.id;

  if (_is_c_reserved(id))
    throw Panic(
        "Can not use reserved C keyword `" + id + "` as C function id",
        ast->id_token.placement);

  std::vector<ArgDecl> args;
  std::map<std::string, Placement> arg_id_map;

  for (auto &arg : ast->arg_nodes) {
    if (arg->id_token.has_value()) {
      auto token = arg->id_token.value();
      auto name = token.id;

      if (arg_id_map.contains(name)) {
        throw Panic(
            "Already declared argument with this name",
            token.placement,
            {{"Previously declared here", arg_id_map.at(name)}});
      } else {
        arg_id_map.insert({name, token.placement});
      }
    }

    args.push_back(ArgDecl::compile(arg));
  }

  return _CFuncDecl(ast, return_type, id, args);
}

#pragma endregion

#pragma region _Scope

MLIR::_ConcreteType
MLIR::_Scope::compile_type(std::shared_ptr<Onyx::AST::TypeExpr> ast) {}

std::shared_ptr<MLIR::_VarDecl>
MLIR::_Scope::compile_var_decl(std::shared_ptr<Onyx::AST::VarDecl> ast) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  auto id = ast->id_token.id;

  if (auto previous = _search_var_decl(id))
    throw Panic(
        "Variable already declared with name `" + id + "`",
        ast->id_token.placement,
        {{"Previous declaration here", previous->ast->id_token.placement}});

  if (ast->value) {
    auto rval = compile_rval(ast->value.value());

    _ConcreteType type = (ast->type_restriction)
                             ? (compile_type(ast->type_restriction))
                             : _infer(&rval);

    auto decl = std::make_shared<_VarDecl>(ast, id, type, move(rval));
    _add_expr(decl);

    return decl;
  } else if (ast->type_restriction) {
    throw Unimplemented();
  } else {
    throw Panic(
        "Could not infer variable declaration type", ast->id_token.placement);
  }
}

std::shared_ptr<MLIR::_FuncDecl>
MLIR::_Scope::compile_func_decl(std::shared_ptr<Onyx::AST::FuncDecl> ast) {
  // TODO: Overloading.
  auto existing = _search_func_decl(ast->id_token.id);
  if (existing)
    throw Panic(
        "Already defined function with id `" + ast->id_token.id + "`",
        ast->id_token.placement,
        {{"Previosly defined here", existing->ast->id_token.placement}});

  // TODO: Infer type from body.
  auto return_type = compile_type(ast->return_type_restriction);

  std::vector<std::shared_ptr<_VarDecl>> arg_decls;
  for (auto &arg : ast->arg_decls) {
    arg_decls.push_back(compile_var_decl(arg));
  }

  std::shared_ptr<_Block> body = nullptr;
  if (ast->body.has_value()) {
    Safety safety = this->safety;
    Storage storage = this->storage;

    body = this->_create_child<_Block>(
        ast->id_token.id, safety, storage, shared_from_this());

    for (auto &expr : ast->body.value()->exprs) {
      body->compile_expr(expr);
    }
  }

  auto func_decl = std::make_shared<_FuncDecl>(
      ast, ast->id_token.id, return_type, arg_decls, body);

  _func_decls[ast->id_token.id] = func_decl;
  return func_decl;
}

std::shared_ptr<MLIR::_CCall>
MLIR::_Scope::compile_c_call(std::shared_ptr<Onyx::AST::CCall> ast) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  if (this->safety > Safety::Unsafe)
    throw Panic(
        "Can not call a C function from within a " + safety_name(this->safety) +
            " context",
        ast->callee.placement);

  auto callee_id = ast->callee.id;
  std::shared_ptr<_CFuncDecl> c_func_decl = _search_c_func_decl(callee_id);

  if (!c_func_decl)
    throw Panic(
        "Use of undeclared C function `" + callee_id + "`",
        ast->callee.placement);

  std::vector<_RVal> args;

  for (auto &arg : ast->arguments) {
    auto rval = compile_rval(arg);
    args.push_back(std::move(rval));
  }

  auto ptr = std::make_shared<_CCall>(c_func_decl, move(args));
  _add_expr(ptr);

  return ptr;
}

MLIR::_RVal MLIR::_Scope::compile_rval(Onyx::AST::RVal ast) {
  std::visit(
      [](auto &ast) {
        Util::logger.trace({"MLIR", "_Scope"})
            << "compile_rval(" << ast->trace() << ")\n";
      },
      ast);

  if (auto node =
          std::get_if<std::shared_ptr<Onyx::AST::CStringLiteral>>(&ast)) {
    return std::make_unique<_CStringLiteral>(node->get()->token.string);
  } else if (auto node = std::get_if<std::shared_ptr<Onyx::AST::UnOp>>(&ast)) {
    if (node->get()->operator_.op == "&") {
      // A PointerOf operation.
      //

      auto operand = compile_rval(node->get()->operand);

      if (auto var_ref = std::get_if<std::unique_ptr<_VarRef>>(&operand)) {
        return std::make_unique<_PointerOf>(*var_ref->get());
      } else {
        throw Unimplemented();
      }
    } else
      throw Unimplemented();
  } else if (auto node = std::get_if<std::shared_ptr<Onyx::AST::Id>>(&ast)) {
    auto id_token = node->get()->token;
    auto id = id_token.id;

    if (auto var_decl = _search_var_decl(id)) {
      return std::make_unique<_VarRef>(var_decl);
    } else {
      throw Panic(
          "Use of undeclared variable `" + id + "`", id_token.placement);
    }
  } else if (auto node = std::get_if<std::shared_ptr<Onyx::AST::CCall>>(&ast)) {
    return compile_c_call(*node);
  } else
    throw Unimplemented();
}

MLIR::_RVal MLIR::_Scope::compile_rval(
    std::shared_ptr<Onyx::AST::ExplicitSafetyStatement> ast) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  auto id = std::to_string(_children.size());
  auto scope = this->_create_child<_Block>(id, ast->safety(), this->storage);

  // TODO: `unsafe! fragile! foo`.
  auto rval = scope->compile_rval(ast->value);

  _add_expr(scope);
  return rval;
}

void MLIR::_Scope::compile_extern_directive(
    std::shared_ptr<Onyx::AST::ExternDirective> ast) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  // TODO: What if it contains `#include`?
  compile_c_ast(ast->block->ast());
}

void MLIR::_Scope::compile_c_ast(const C::AST *ast) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  for (auto &child : ast->chidren()) {
    std::visit(
        [this](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<C::AST::FuncDecl>>) {
            _add_c_func_decl(arg);
          } else
            static_assert(Util::Variant::false_t<T>, "non-exhaustive visitor!");
        },
        child);
  }
}

std::string MLIR::_Scope::function_prefix() const {
  std::string result = id + "/";

  if (parent) {
    auto parent_path = parent->function_prefix();

    if (!parent_path.empty())
      result.insert(0, parent_path + "/");
  }

  return result;
}

MLIR::_ConcreteType MLIR::_Scope::_infer(_RVal *hint) {
  Util::logger.trace({"MLIR", "_Scope"}) << __builtin_FUNCTION() << "()\n";

  if (auto c_string_literal =
          std::get_if<std::unique_ptr<_CStringLiteral>>(hint)) {
    return _CTypeRef(_CBuiltInType::Char, 1);
  } else {
    throw Unimplemented();
  }
}

std::shared_ptr<MLIR::_VarDecl> MLIR::_Scope::_search_var_decl(std::string id) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << id << ")\n";

  if (auto ptr = *Util::Map::get_if(_var_decls, id))
    return ptr;
  else if (parent)
    return parent->_search_var_decl(id);
  else
    return nullptr;
}

std::shared_ptr<MLIR::_FuncDecl>
MLIR::_Scope::_search_func_decl(std::string id) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << id << ")\n";

  if (auto ptr = *Util::Map::get_if(_func_decls, id))
    return ptr;
  else if (parent)
    return parent->_search_func_decl(id);
  else
    return nullptr;
}

std::shared_ptr<MLIR::_CFuncDecl>
MLIR::_Scope::_search_c_func_decl(std::string id) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << id << ")\n";

  auto ptr = *Util::Map::get_if(_c_func_decls, id);

  if (!ptr && parent)
    return parent->_search_c_func_decl(id);
  else
    return ptr;
}

void MLIR::_Scope::_add_c_func_decl(
    std::shared_ptr<const C::AST::FuncDecl> ast) {
  Util::logger.trace({"MLIR", "_Scope"})
      << __builtin_FUNCTION() << "(" << ast->trace() << ")\n";

  C::Token::Id id_token = ast->id_token;
  auto id = id_token.id;

  if (auto previous = _search_c_func_decl(id))
    throw Panic(
        "Already declared function with id `" + id + "`",
        id_token.placement,
        {{"Previously declared here", previous->ast->id_token.placement}});

  auto ptr = std::make_shared<_CFuncDecl>(_CFuncDecl::compile(ast));
  _c_func_decls[id] = ptr;
}

void MLIR::_Scope::_add_expr(_Expr expr) {
  Util::logger.trace({"MLIR", "_Scope"}) << __builtin_FUNCTION() << "()\n";

  if (auto var_decl = std::get_if<std::shared_ptr<_VarDecl>>(&expr)) {
    _var_decls[var_decl->get()->id] = *var_decl;
  }

  _exprs.push_back(move(expr));
}

#pragma endregion

std::optional<MLIR::_CBuiltInType>
MLIR::_search_c_built_in_type(std::string id) {
  if (!id.compare("void"))
    return _CBuiltInType::Void;
  else if (!id.compare("char"))
    return _CBuiltInType::Char;
  else
    return std::nullopt;
}

bool MLIR::_is_c_reserved(std::string id) {
  if (!id.compare("void"))
    return true;
  else if (!id.compare("char"))
    return true;
  else if (!id.compare("const"))
    return true;
  else
    return false;
}

} // namespace Fancysoft::NXC
