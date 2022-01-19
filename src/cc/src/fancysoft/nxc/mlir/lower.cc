void MLIR::lower(llvm::Module *module) const {
  Util::logger.trace("MLIR") << __builtin_FUNCTION() << "()\n";
  _top_level_scope->lower(module);
}

llvm::Value *
MLIR::_CStringLiteral::lower(llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_CStringLiteral"})
      << __builtin_FUNCTION() << "()\n";

  auto constant = builder->CreateGlobalString(this->value);
  // constant.
  return constant;
}

#pragma region _CFuncDecl

#pragma endregion

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

llvm::Value *
MLIR::_VarRef::lower(llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_VarRef"}) << __builtin_FUNCTION() << "()\n";
  assert(this->decl->_llvm_ref);
  return this->decl->_llvm_ref;
}

#pragma region _VarDecl

template <>
llvm::Value *MLIR::_VarDecl::lower<MLIR::_TopLevelScope>(
    llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";

  // TODO: A top-level variable declaration is global if exported.
  //

  return _lower_to_local(module, builder);
}

template <>
llvm::Value *MLIR::_VarDecl::lower<MLIR::_Block>(
    llvm::Module *module, llvm::IRBuilder<> *builder) {
  Util::logger.trace({"MLIR", "_VarDecl"}) << __builtin_FUNCTION() << "()\n";
  return _lower_to_local(module, builder);
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
                std::is_same_v<T, std::shared_ptr<_Call>> ||
                std::is_same_v<T, std::unique_ptr<_PointerOf>>) {
              // TODO: Run the block and then copy.
              throw Unimplemented();
            } else if constexpr (std::is_same_v<
                                     T,
                                     std::unique_ptr<_CStringLiteral>>) {
              this->_llvm_ref =
                  builder->CreateGlobalStringPtr(rval->value, this->id);
            } else {
              static_assert(Util::Variant::false_t<T>, "Unhandled option");
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

llvm::Value *MLIR::_FuncDecl::lower(llvm::Module *, std::string name_prefix) {
  // TODO:
  // Lower self
  // Lower the body if exists
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
            std::is_same_v<T, std::shared_ptr<_Call>> ||
            std::is_same_v<T, std::unique_ptr<_PointerOf>> ||
            std::is_same_v<T, std::unique_ptr<_CStringLiteral>>) {
          auto llvm_rval = rval.get()->lower(module, builder);
          auto llvm_lval = this->lvalue.lower(module, builder);
          llvm_result = builder->CreateStore(llvm_lval, llvm_rval);
        } else {
          static_assert(Util::Variant::false_t<T>, "Unhandled option");
        }
      },
      this->rvalue);

  return llvm_result;
}

llvm::Value *MLIR::_PointerOf::lower(
    llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_PointerOf"}) << __builtin_FUNCTION() << "()\n";
  // TODO:
}

void MLIR::_TopLevelScope::lower(llvm::Module *module) const {
  Util::logger.trace({"MLIR", "_TopLevelScope"})
      << __builtin_FUNCTION() << "()\n";

  for (auto &decl : _c_func_decls) {
    decl.second->lower(module);
  }

  for (auto &decl : _func_decls) {
    decl.second->lower(module, nullptr);
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
          } else if constexpr (std::is_same_v<T, std::shared_ptr<_FuncDecl>>) {
            // Function are lowered outside of the main function.
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

llvm::Value *
MLIR::_Block::lower(llvm::Module *module, llvm::IRBuilder<> *builder) const {
  Util::logger.trace({"MLIR", "_Block"}) << __builtin_FUNCTION() << "()\n";

  // Functions declared within a block are declared on the top level of LLIR.
  for (auto &decl : _func_decls) {
    decl.second->lower(module, function_prefix());
  }

  llvm::Value *last_value;

  for (auto &expr : this->_exprs) {
    std::visit(
        [module, builder, &last_value](auto &expr) {
          using T = std::decay_t<decltype(expr)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<_VarDecl>>) {
            last_value = expr->template lower<_Block>(module, builder);
          } else if constexpr (std::is_same_v<T, std::shared_ptr<_FuncDecl>>) {
            // Handled above.
          } else {
            last_value = expr->lower(module, builder);
          }
        },
        expr);
  }

  return last_value;
}
