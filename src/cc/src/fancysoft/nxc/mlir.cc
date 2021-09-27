#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Verifier.h"
#include <iostream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "fancysoft/nxc/exception.hh"
#include "fancysoft/nxc/mlir.hh"
#include "fancysoft/util/logger.hh"
#include "fancysoft/util/variant.hh"
#include "llvm/Support/raw_os_ostream.h"

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
                                   std::shared_ptr<Onyx::AST::CCall>>) {
            _top_level_scope->compile_c_call(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<
                                       Onyx::AST::ExplicitSafetyStatement>>) {
            _top_level_scope->compile_rval(arg);
          } else
            throw Unimplemented();
        },
        node);
  }
}

void MLIR::write(std::ostream &out) const {
  Util::logger.trace("MLIR") << __builtin_FUNCTION() << "()\n";
  _top_level_scope->write(out);
}

void MLIR::lower(llvm::Module *module) const {
  Util::logger.trace("MLIR") << __builtin_FUNCTION() << "()\n";
  _top_level_scope->lower(module);
}

#pragma region _CStringLiteral

void MLIR::_CStringLiteral::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_CStringLiteral"})
      << __builtin_FUNCTION() << "()\n";

  out << "$\"" << this->value << '"';
}

llvm::Value *
MLIR::_CStringLiteral::lower(llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_CStringLiteral"})
      << __builtin_FUNCTION() << "()\n";

  auto constant = builder->CreateGlobalString(this->value);
  // constant.
  return constant;
}

#pragma endregion

#pragma region _CTypeRef

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

void MLIR::_CTypeRef::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_CTypeRef"}) << __builtin_FUNCTION() << "()\n";

  _write(this->type, out);

  for (int i = 0; i < pointer_depth; i++)
    out << '*';
}

llvm::Type *MLIR::_CTypeRef::lower(llvm::Module *module) const {
  Util::logger.trace({"MLIR", "_CTypeRef"}) << __builtin_FUNCTION() << "()\n";

  switch (this->type) {
  case _CBuiltInType::Void: {
    if (pointer_depth == 0)
      return llvm::Type::getVoidTy(module->getContext());
    else if (pointer_depth == 1)
      return llvm::Type::getInt8PtrTy(module->getContext());
    else
      throw Unimplemented();
  }
  case _CBuiltInType::Char: {
    if (pointer_depth == 0)
      return llvm::Type::getInt8Ty(module->getContext());
    else if (pointer_depth == 1)
      return llvm::Type::getInt8PtrTy(module->getContext());
    else
      throw Unimplemented();
  }
  }
}

#pragma endregion

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

llvm::Type *MLIR::_CFuncDecl::ArgDecl::lower(llvm::Module *module) const {
  Util::logger.trace({"MLIR", "_CFuncDecl", "ArgDecl"})
      << __builtin_FUNCTION() << "()\n";

  return this->type.lower(module);
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

void MLIR::_CFuncDecl::write(std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_CFuncDecl"}) << __builtin_FUNCTION() << "()\n";

  out << std::string(indent, '\t');
  out << "decl ";
  this->return_type.write(out);
  out << " @" << this->id << "(";

  bool first = true;
  for (auto &arg : this->args) {
    if (!first)
      out << ", ";
    else
      first = false;

    arg.type.write(out);
  }

  out << ")\n";
}

llvm::Function *MLIR::_CFuncDecl::lower(llvm::Module *module) const {
  Util::logger.trace({"MLIR", "_CFuncDecl"}) << __builtin_FUNCTION() << "()\n";

  std::vector<llvm::Type *> llvm_args;

  for (auto &arg : args) {
    llvm_args.push_back(arg.lower(module));
  }

  auto llvm_return_type = return_type.lower(module);

  llvm::FunctionType *llvm_function_type =
      llvm::FunctionType::get(llvm_return_type, llvm_args, false);

  llvm::Function *llvm_function = llvm::Function::Create(
      llvm_function_type, llvm::Function::ExternalLinkage, this->id, module);

  llvm_function->addAttribute(-1, llvm::Attribute::NoFree);
  llvm_function->addAttribute(-1, llvm::Attribute::NoUnwind);

  unsigned i = 0;
  for (auto &arg : llvm_function->args()) {
    auto name = args[i++].id;

    if (name.has_value())
      arg.setName(name.value());
  }

  return llvm_function;
}

#pragma endregion

#pragma region _CCall

void MLIR::_CCall::write(std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_CCall"}) << __builtin_FUNCTION() << "()\n";

  out << std::string(indent, '\t');
  out << "call @" << this->callee->id << '(';

  bool first = true;
  for (auto &arg : this->args) {
    if (first)
      first = false;
    else
      out << ", ";

    std::visit([&out](auto &arg) { arg->write(out); }, arg);
  }

  out << ")\n";
}

llvm::Value *
MLIR::_CCall::lower(llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_CCall"}) << __builtin_FUNCTION() << "()\n";

  llvm::Function *llvm_function = module->getFunction(this->callee->id);

  if (!llvm_function)
    throw Panic(
        "Undeclared C function reference",
        this->callee->ast->id_token.placement);

  if (llvm_function->arg_size() != this->args.size())
    throw Panic("Arity mismatch", this->callee->ast->id_token.placement);

  std::vector<llvm::Value *> llvm_args;

  for (auto &arg : this->args) {
    std::visit(
        [&llvm_args, module, builder](auto &arg) {
          llvm_args.push_back(arg.get()->lower(module, builder));
        },
        arg);
  }

  if (llvm_function->getReturnType()->isVoidTy()) {
    // When void is returned, no temp value may be created.
    return builder->CreateCall(llvm_function, llvm_args);
  } else {
    return builder->CreateCall(llvm_function, llvm_args, "calltmp");
  }
}

#pragma endregion

#pragma region _VarRef

void MLIR::_VarRef::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_VarRef"}) << __builtin_FUNCTION() << "()\n";
  out << '%' << decl->id;
}

llvm::Value *
MLIR::_VarRef::lower(llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_VarRef"}) << __builtin_FUNCTION() << "()\n";
  assert(this->decl->_llvm_ref);
  return this->decl->_llvm_ref;
}

#pragma endregion

#pragma region _VarDecl

template <>
void MLIR::_VarDecl::write<MLIR::_TopLevelScope>(
    std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";

  // TODO: A top-level variable declaration is global if exported.
  //

  out << std::string(indent, '\t');
  out << "local ";
  std::visit([&out](auto type) { type.write(out); }, this->type);
  out << " %" << this->id;

  if (this->value.has_value()) {
    out << " = ";
    std::visit([&out](auto &rval) { rval->write(out); }, this->value.value());
  }

  out << "\n";
}

template <>
llvm::Value *MLIR::_VarDecl::lower<MLIR::_TopLevelScope>(
    llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";

  // TODO: A top-level variable declaration is global if exported.
  //

  return _lower_to_local(module, builder);
}

template <>
void MLIR::_VarDecl::write<MLIR::_Block>(
    std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";
  _write_local(out, indent);
}

template <>
llvm::Value *MLIR::_VarDecl::lower<MLIR::_Block>(
    llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";
  return _lower_to_local(module, builder);
}

void MLIR::_VarDecl::_write_local(std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";

  out << std::string(indent, '\t');
  out << "local ";
  std::visit([&out](auto type) { type.write(out); }, this->type);
  out << " %" << this->id;

  if (this->value.has_value()) {
    out << " = ";
    std::visit([&out](auto &rval) { rval->write(out); }, this->value.value());
  }

  out << "\n";
}

llvm::Value *MLIR::_VarDecl::_lower_to_local(
    llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";

  if (!(this->_llvm_ref)) {
    if (this->value.has_value()) {
      std::visit(
          [this, module, builder](auto &rval) {
            using T = std::decay_t<decltype(rval)>;

            if constexpr (std::is_same_v<T, std::unique_ptr<_VarRef>>) {
              // TODO: Alloca and then copy.
              throw Unimplemented();
            } else if constexpr (
                std::is_same_v<T, std::shared_ptr<_Block>> ||
                std::is_same_v<T, std::shared_ptr<_CCall>> ||
                std::is_same_v<T, std::unique_ptr<_PointerOf>>) {
              // TODO: Run the block and then copy.
              throw Unimplemented();
            } else if constexpr (std::is_same_v<
                                     T,
                                     std::unique_ptr<_CStringLiteral>>) {
              this->_llvm_ref =
                  builder->CreateGlobalStringPtr(rval->value, this->id);
            } else {
              static_assert(
                  Util::Variant::always_false_v<T>, "Unhandled option");
            }
          },
          this->value.value());
    } else {
      std::visit(
          [this, module, builder](auto &type) {
            this->_llvm_ref =
                builder->CreateAlloca(type.lower(module), nullptr, this->id);
          },
          this->type);
    }
  }

  return this->_llvm_ref;
}

#pragma endregion

#pragma region _Assignment

void MLIR::_Assignment::write(std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_Assignment"}) << __builtin_FUNCTION() << "()\n";
  out << std::string(indent, '\t');
  out << '%' << this->lvalue.decl->id << " = ";
  std::visit([&out](auto &val) { val->write(out); }, this->rvalue);
  out << "\n";
}

void MLIR::_Assignment::write_rvalue(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_Assignment"}) << __builtin_FUNCTION() << "()\n";
  std::visit([&out](auto &rval) { rval->write(out); }, this->rvalue);
}

llvm::Value *MLIR::_Assignment::lower(
    llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_Assignment"}) << __builtin_FUNCTION() << "()\n";
  llvm::StoreInst *llvm_result;

  std::visit(
      [this, module, builder, &llvm_result](auto &rval) {
        using T = std::decay_t<decltype(rval)>;

        if constexpr (std::is_same_v<T, std::unique_ptr<_VarRef>>) {
          // TODO: Alloca and then copy
          throw Unimplemented();
        } else if constexpr (std::is_same_v<T, std::shared_ptr<_Block>>) {
          throw Unimplemented();
        } else if constexpr (
            std::is_same_v<T, std::shared_ptr<_CCall>> ||
            std::is_same_v<T, std::unique_ptr<_PointerOf>> ||
            std::is_same_v<T, std::unique_ptr<_CStringLiteral>>) {
          auto llvm_rval = rval.get()->lower(module, builder);
          auto llvm_lval = this->lvalue.lower(module, builder);
          llvm_result = builder->CreateStore(llvm_lval, llvm_rval);
        } else {
          static_assert(Util::Variant::always_false_v<T>, "Unhandled option");
        }
      },
      this->rvalue);

  return llvm_result;
}

#pragma endregion

#pragma region _PointerOf

void MLIR::_PointerOf::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_PointerOf"}) << __builtin_FUNCTION() << "()\n";
  out << "pointerof(%" << this->value.decl->id << ")";
}

llvm::Value *MLIR::_PointerOf::lower(
    llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_PointerOf"}) << __builtin_FUNCTION() << "()\n";
  // TODO:
}

#pragma endregion

#pragma region _Scope

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

    _TypeRestriction restriction =
        (ast->type_restriction) ? (throw Unimplemented()) : _infer(&rval);

    auto decl = std::make_shared<_VarDecl>(ast, id, restriction, move(rval));
    _add_expr(decl);

    return decl;
  } else if (ast->type_restriction) {
    throw Unimplemented();
  } else {
    throw Panic(
        "Could not infer variable declaration type",
        ast->keyword_token.placement);
  }
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

  auto scope = this->_create_child<_Block>(ast->safety(), this->storage);

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
            static_assert(
                Util::Variant::always_false_v<T>, "non-exhaustive visitor!");
        },
        child);
  }
}

MLIR::_TypeRestriction MLIR::_Scope::_infer(_RVal *hint) {
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

  if (auto ptr = *Util::Map::get_if(_var_decl_index, id))
    return ptr;
  else if (parent)
    return parent->_search_var_decl(id);
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
    _var_decl_index[var_decl->get()->id] = *var_decl;
  }

  _exprs.push_back(move(expr));
}

#pragma endregion

#pragma region _TopLevelScope

void MLIR::_TopLevelScope::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_TopLevelScope"})
      << __builtin_FUNCTION() << "()\n";

  for (auto &decl : _c_func_decls) {
    decl.second->write(out);
  }

  out << "def @(implicit main)\n";

  for (auto &expr : this->_exprs) {
    std::visit(
        [&out](auto &expr) {
          using T = std::decay_t<decltype(expr)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<_VarDecl>>) {
            expr->template write<_TopLevelScope>(out, 1);
          } else {
            expr->write(out, 1);
          }
        },
        expr);
  }

  out << "end\n";
}

void MLIR::_TopLevelScope::lower(llvm::Module *module) const {
  Util::logger.trace({"MLIR", "_TopLevelScope"})
      << __builtin_FUNCTION() << "()\n";

  for (auto &decl : _c_func_decls) {
    decl.second->lower(module);
  }

  // i32 %0, i8** %1
  std::vector<llvm::Type *> main_args = {
      llvm::Type::getInt32Ty(module->getContext()),
      llvm::Type::getInt8PtrTy(module->getContext())->getPointerTo()};

  // i32
  llvm::Type *main_return = llvm::Type::getInt32Ty(module->getContext());

  auto *func_type = llvm::FunctionType::get(main_return, main_args, false);

  auto main = llvm::Function::Create(
      func_type, llvm::Function::ExternalLinkage, "main", module);

  main->addAttribute(-1, llvm::Attribute::NoFree);
  main->addAttribute(-1, llvm::Attribute::NoUnwind);
  main->addAttribute(-1, llvm::Attribute::UWTable);

  // i8** nocapture readnone %1
  main->addAttribute(2, llvm::Attribute::NoCapture);
  main->addAttribute(2, llvm::Attribute::ReadNone);

  // Create the basic entry block for the function.
  auto block = llvm::BasicBlock::Create(module->getContext(), "entry", main);

  // Create an IR builder pointing to the entry block.
  auto builder = llvm::IRBuilder<>(module->getContext());
  builder.SetInsertPoint(block);

  for (auto &expr : this->_exprs) {
    std::visit(
        [module, &builder](auto &expr) {
          using T = std::decay_t<decltype(expr)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<_VarDecl>>) {
            expr->template lower<_TopLevelScope>(module, &builder);
          } else {
            expr->lower(module, &builder);
          }
        },
        expr);
  }

  // ret i32 0;
  builder.CreateRet(
      llvm::ConstantInt::get(llvm::Type::getInt32Ty(module->getContext()), 0));

  llvm::raw_os_ostream llvm_cerr(std::cerr);
  if (llvm::verifyFunction(*main, &llvm_cerr))
    throw "LLVM failed to verify the implicit main function";
}

#pragma endregion

#pragma region _Block

llvm::Value *
MLIR::_Block::lower(llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_Block"}) << __builtin_FUNCTION() << "()\n";

  llvm::Value *last_value;

  for (auto &expr : this->_exprs) {
    std::visit(
        [module, builder, &last_value](auto &expr) {
          using T = std::decay_t<decltype(expr)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<_VarDecl>>) {
            last_value = expr->template lower<_Block>(module, builder);
          } else {
            last_value = expr->lower(module, builder);
          }
        },
        expr);
  }

  return last_value;
}

void MLIR::_Block::write(std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_Block"}) << __builtin_FUNCTION() << "()\n";

  for (auto &expr : this->_exprs) {
    std::visit(
        [&out, indent](auto &expr) {
          using T = std::decay_t<decltype(expr)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<_VarDecl>>) {
            expr->template write<_Block>(out, indent);
          } else {
            expr->write(out, indent);
          }
        },
        expr);
  }
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

void MLIR::_write(MLIR::_CBuiltInType type, std::ostream &out) {
  switch (type) {
  case _CBuiltInType::Void:
    out << "$void";
    break;
  case _CBuiltInType::Char:
    out << "$char";
    break;
  }
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
