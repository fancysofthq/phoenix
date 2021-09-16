#include <memory>
#include <variant>

#include "fancysoft/nxc/onyx/hlir.hh"

namespace Fancysoft::NXC::Onyx {

void HLIR::IntLiteral::write(std::ostream &output) const {
  output << this->value;
}

void HLIR::StringLiteral::write(std::ostream &output) const {
  throw "Not implemented";
  // @String::new($"value");
}

void HLIR::CStringLiteral::write(std::ostream &output) const {
  output << "$\"" << this->value << '"';
}

void HLIR::VarDecl::write(std::ostream &output) const {
  std::visit([&output](auto &rest) { rest.write(output); }, this->type);
  output << " %" << this->id << "\n";
}

void HLIR::PointerOf::write(std::ostream &output) const {
  output << "&%" << this->value->id;
}

void HLIR::CCall::write(std::ostream &output) const {
  output << "@$" << this->callee->id << '(';

  bool first = true;
  for (auto &arg : this->args) {
    if (!first)
      output << ", ";
    else
      first = false;

    arg->write(output);
  }

  output << ")\n";
}

void HLIR::Assignment::write(std::ostream &output) const {
  output << '%' << this->lvalue->id << " = ";
  std::visit([&output](auto &val) { val->write(output); }, this->rvalue);
  output << "\n";
}

HLIR::HLIR(NXC::Program *program, const AST *onyx_ast) {
  for (auto &node : onyx_ast->children()) {
    std::visit(
        [this, program](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<
                            T,
                            std::shared_ptr<AST::ExternDirective>>) {
            _compile_extern_directive(program, arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<AST::VarDecl>>) {
            _implicit_main_scope->compile(arg);
          } else if constexpr (std::is_same_v<T, std::shared_ptr<AST::CCall>>) {
            _implicit_main_scope->compile(arg);
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<
                                       AST::ExplicitSafetyStatement>>) {
            _implicit_main_scope->compile(arg);
          } else
            throw "Not implemented!";
        },
        node);
  }
}

void HLIR::_compile_extern_directive(
    Program *program, std::shared_ptr<AST::ExternDirective> dir) {
  // TODO: What if it contains `#include`?
  _self_c_hlir->compile(dir->block.get());
}

std::shared_ptr<HLIR::VarDecl>
HLIR::Scope::compile(std::shared_ptr<AST::VarDecl> decl) {
  auto id = decl->id_token.id;

  if (_search_var_decl(id))
    // TODO: Point to the previous declaration.
    throw Panic("Variable already declared with name `" + id + "`");

  if (decl->value) {
    auto rval = compile(decl->value.value());

    TypeRestriction restriction =
        (decl->type_restriction) ? (throw "Not implemented") : _infer(&rval);

    auto hlir_decl = std::make_shared<VarDecl>(id, restriction);
    _add_var_decl(hlir_decl);

    auto hlir_assignment = std::make_shared<Assignment>(hlir_decl, move(rval));
    _add_expr(hlir_assignment);

    return hlir_decl;
  } else if (decl->type_restriction) {
    throw "Not implemented yet";
  } else {
    throw Panic(
        "Could not resolve variable declaration type",
        decl->keyword_token.placement);
  }
}

std::shared_ptr<HLIR::CCall>
HLIR::Scope::compile(std::shared_ptr<Onyx::AST::CCall> decl) {
  if (this->safety.value() > Safety::Unsafe) {
    throw Panic(
        "Can not call a C function from within a " +
        safety_name(this->safety.value()) + " context");
  }

  auto callee_id = decl->callee.id;
  std::shared_ptr<C::HLIR::FuncDecl> c_func_decl;

  if (!(c_func_decl = hlir->_find_func_decl(callee_id)))
    throw Panic(
        "Use of undeclared C function `" + callee_id + "`",
        decl->callee.placement);

  std::vector<std::unique_ptr<HLIR::PointerOf>> args;

  for (auto &arg : decl->arguments) {
    auto rval = compile(arg);

    if (auto pointer_of =
            std::get_if<std::unique_ptr<HLIR::PointerOf>>(&rval)) {
      args.push_back(std::move(*pointer_of));
    }
  }

  auto ptr = std::make_shared<HLIR::CCall>(c_func_decl, move(args));
  _add_expr(ptr);

  return ptr;
}

HLIR::RVal HLIR::Scope::compile(AST::RVal decl) {
  if (auto node = std::get_if<std::shared_ptr<AST::StringLiteral>>(&decl)) {
    return std::make_unique<HLIR::StringLiteral>(node->get()->token.string);
  } else if (
      auto node = std::get_if<std::shared_ptr<AST::CStringLiteral>>(&decl)) {
    return std::make_unique<HLIR::CStringLiteral>(node->get()->token.string);
  } else if (auto node = std::get_if<std::shared_ptr<AST::UnOp>>(&decl)) {
    if (node->get()->operator_.op == "&") {
      // A PointerOf operation.
      //

      auto operand = compile(node->get()->operand);

      if (auto var_decl =
              std::get_if<std::shared_ptr<HLIR::VarDecl>>(&operand)) {
        return std::make_unique<HLIR::PointerOf>(*var_decl);
      } else {
        throw "Not implemented";
      }
    } else
      throw "Not implemented";
  } else if (auto node = std::get_if<std::shared_ptr<AST::Id>>(&decl)) {
    auto id_token = node->get()->token;
    auto id = id_token.id;

    if (auto var_decl = _search_var_decl(id)) {
      return var_decl;
    } else {
      throw Panic(
          "Use of undeclared variable `" + id + "`", id_token.placement);
    }
  } else if (auto node = std::get_if<std::shared_ptr<AST::CCall>>(&decl)) {
    return compile(*node);
  } else
    throw "Not implemented";
}

HLIR::RVal
HLIR::Scope::compile(std::shared_ptr<Onyx::AST::ExplicitSafetyStatement> ast) {
  auto scope = this->create_child(ast->safety(), this->storage);
  auto rval = scope->compile(ast->value);
  _add_expr(scope);
  return rval;
}

void HLIR::Scope::write(std::ostream &output) const {
  for (auto &pair : this->_var_decls) {
    pair.second->write(output);
  }

  for (auto &expr : this->_exprs) {
    std::visit([&output](auto &expr) { expr->write(output); }, expr);
  }
}

HLIR::TypeRestriction HLIR::Scope::_infer(HLIR::RVal *hint) {
  if (auto c_string_literal =
          std::get_if<std::unique_ptr<CStringLiteral>>(hint)) {
    return C::HLIR::TypeRef(C::HLIR::BuiltInType::Char, 1);
  } else {
    throw "Not implemented";
  }
}

void HLIR::write(std::ostream &output) const {
  _self_c_hlir->write(output);
  _implicit_main_scope->write(output);
}

} // namespace Fancysoft::NXC::Onyx
