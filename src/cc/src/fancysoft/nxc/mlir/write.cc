void MLIR::write(std::ostream &out) const {
  Util::logger.trace("MLIR") << __builtin_FUNCTION() << "()\n";
  _top_level_scope->write(out);
}

void MLIR::_CStringLiteral::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_CStringLiteral"})
      << __builtin_FUNCTION() << "()\n";

  out << "$\"" << this->value << '"';
}

void MLIR::_CTypeRef::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_CTypeRef"}) << __builtin_FUNCTION() << "()\n";

  _write(this->type, out);

  for (int i = 0; i < pointer_depth; i++)
    out << '*';
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

void MLIR::_VarRef::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_VarRef"}) << __builtin_FUNCTION() << "()\n";
  out << '%' << decl->id;
}

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
void MLIR::_VarDecl::write<MLIR::_Block>(
    std::ostream &out, unsigned indent) const {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";
  _write_local(out, indent);
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

#pragma endregion

void MLIR::_PointerOf::write(std::ostream &out) const {
  Util::logger.trace({"MLIR", "_PointerOf"}) << __builtin_FUNCTION() << "()\n";
  out << "pointerof(%" << this->value.decl->id << ")";
}

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
