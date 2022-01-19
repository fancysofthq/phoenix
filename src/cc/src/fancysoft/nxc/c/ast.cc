#include <set>

#include <fancysoft/util/map.hh>

#include "fancysoft/nxc/c/ast.hh"
#include "fancysoft/nxc/exception.hh"

namespace Fancysoft::NXC::C {

AST::TypeRef AST::_compile(std::shared_ptr<const CST::TypeRef> cst) {
  std::optional<BuiltInType> type;
  if (!(type = _search_c_built_in_type(cst->id.value)))
    throw Panic("Undeclared C id `" + cst->id.value + "`", cst->id.placement);

  return TypeRef(cst, type.value(), cst->pointer_depth());
}

llvm::Type *AST::TypeRef::codegen(llvm::Module *module, Logger *logger) const {
  logger->trace();

  switch (this->type) {
  case BuiltInType::Void: {
    if (pointer_depth == 0)
      return llvm::Type::getVoidTy(module->getContext());
    else if (pointer_depth == 1)
      return llvm::Type::getInt8PtrTy(module->getContext());
    else
      throw Unimplemented(); // TODO:
  }
  case BuiltInType::Char: {
    if (pointer_depth == 0)
      return llvm::Type::getInt8Ty(module->getContext());
    else if (pointer_depth == 1)
      return llvm::Type::getInt8PtrTy(module->getContext());
    else
      throw Unimplemented(); // TODO:
  }
  }
}

AST::FuncDecl::Arg
AST::_compile(std::shared_ptr<const CST::FuncDecl::Arg> cst) {
  auto type = _compile(cst->type);

  std::optional<std::string> id;
  if (cst->id) {
    id = cst->id.value().value;
  }

  return FuncDecl::Arg(cst, type, id);
}

AST::FuncDecl::VArg
AST::_compile(std::shared_ptr<const CST::FuncDecl::VArg> cst) {
  return FuncDecl::VArg(cst);
}

llvm::Function *
AST::FuncDecl::codegen(llvm::Module *module, Logger *logger) const {
  logger->trace();

  if (auto existing = module->getFunction(this->id))
    return existing;

  auto llvm_return_type = this->return_type.codegen(module, logger);

  std::vector<llvm::Type *> llvm_args;
  for (auto &arg : this->args)
    llvm_args.push_back(arg.type.codegen(module, logger));

  llvm::FunctionType *llvm_function_type =
      llvm::FunctionType::get(llvm_return_type, llvm_args, !!(this->varg));

  llvm::Function::LinkageTypes linkage = llvm::Function::ExternalLinkage;

  llvm::Function *llvm_function =
      llvm::Function::Create(llvm_function_type, linkage, this->id, module);

  std::set<llvm::Attribute> llvm_function_attrs = {
      llvm::Attribute::NoFree, llvm::Attribute::NoUnwind};

  for (auto attr : llvm_function_attrs)
    llvm_function->addAttribute(-1, attr);

  unsigned i = 0;
  for (auto &arg : llvm_function->args()) {
    auto name = this->args[i++].id;

    if (name.has_value())
      arg.setName(name.value());
  }

  return llvm_function;
}

void AST::compile(const CST *cst) {
  for (auto &child : cst->chidren()) {
    std::visit(
        [this](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<CST::FuncDecl>>) {
            this->_compile(arg);
          } else
            static_assert(Util::Variant::False<T>, "Non-exhaustive visitor!");
        },
        child);
  }
}

void AST::_compile(std::shared_ptr<const CST::FuncDecl> cst) {
  std::string id = cst->id.value;
  if (auto previous = search_func_decl(id))
    throw Panic(
        "Already declared function `" + id + "`",
        cst->id.placement,
        {{"Previously declared here", previous->cst->id.placement}});

  auto return_type = _compile(cst->return_type);

  std::vector<std::variant<FuncDecl::Arg>> args;
  for (auto &arg : cst->args) {
    args.push_back(_compile(arg));
  }

  std::optional<FuncDecl::VArg> varg;
  if (cst->varg) {
    varg.emplace(_compile(cst->varg));
  }

  _func_decls.emplace(id, cst, id, return_type, args, varg);
}

std::shared_ptr<AST::FuncDecl> AST::search_func_decl(std::string id) {
  if (auto decl = Util::Map::get_if(_func_decls, id))
    return decl.value();
  else
    return nullptr;
}

void AST::codegen(llvm::Module *module) const {}

std::optional<AST::BuiltInType> AST::_search_c_built_in_type(std::string id) {
  if (!id.compare("void"))
    return BuiltInType::Void;
  else if (!id.compare("char"))
    return BuiltInType::Char;
  else
    return std::nullopt;
}

} // namespace Fancysoft::NXC::C
