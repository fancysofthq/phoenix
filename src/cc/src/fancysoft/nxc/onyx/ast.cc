#include <compare>
#include <memory>
#include <optional>
#include <type_traits>
#include <xstddef>

#include "fancysoft/nxc/exception.hh"
#include "fancysoft/nxc/onyx/cst.hh"
#include "fancysoft/util/map.hh"
#include "fancysoft/util/variant.hh"
#include "fmt/core.h"

#include "fancysoft/nxc/onyx/ast.hh"

namespace Fancysoft::NXC::Onyx::AST {

Scope::P0001 Scope::P0001::construct(
    std::shared_ptr<const Onyx::CST::TypeDecl> type_decl,
    AnySuperDecl superdecl) {
  return P0001(
      type_decl->id,
      type_decl->kind_token.value(),
      std::visit([](auto &d) { return d->kind_token(); }, superdecl));
}

SuperDeclRef create_superdecl_ref(
    std::shared_ptr<const CST::Id::Element> cst_node,
    AnySuperDecl superdecl,
    std::optional<CalleeArgTuple<DeclRefTVal>> targs) {
  return std::visit(
      [&cst_node, &targs](auto &superdecl) {
        using T = std::decay_t<decltype(superdecl)>;

        if constexpr (std::is_same_v<T, std::shared_ptr<FuncSuperDecl>>) {
          return SuperDeclRef(
              std::make_unique<FuncSuperDeclRef>(cst_node, superdecl, targs));
        } else if constexpr (std::is_same_v<
                                 T,
                                 std::shared_ptr<BuiltinTypeSuperDecl>>) {
          return SuperDeclRef(std::make_unique<BuiltinTypeSuperDeclRef>(
              cst_node, superdecl, targs));
        } else if constexpr (std::is_same_v<
                                 T,
                                 std::shared_ptr<TraitSuperDecl>>) {
          return SuperDeclRef(
              std::make_unique<TraitSuperDeclRef>(cst_node, superdecl, targs));
        } else {
          static_assert(Util::Variant::False<T>, "Non-exaustive visitor");
        }
      },
      superdecl);
}

Scope::LookupResult Root::lookup(std::shared_ptr<const CST::Id> cst_id) const {
  bool first = true;
  std::optional<Lang::Id::Element::Access> current_access;
  SuperDeclID id(cst_id, {});
  std::string current_name;

  for (auto &element : cst_id->elements) {
    current_name = element->name_token.value;
    switch (element->access()) {
    case Lang::Id::Element::Access::Self: {
      if (imports.contains(current_name)) {
      }
      if (superdecl_index.contains(element->name_token.value)) {
        auto superdecl = superdecl_index.at(element->name_token.value);
        auto superdecl_ref = create_superdecl_ref(element, superdecl);
        SuperDeclRefExpr expr =
            std::make_unique<SuperDeclRef>()
                id.elements.push_back(SuperDeclID::Element(
                    element->access(),
                    superdecl_index.at(element->name_token.value)));
      }
      // Search in self imports
    }
    case Lang::Id::Element::Access::Static: {
    }
    }
  }
}

std::shared_ptr<BuiltinTypeDecl> BuiltinTypeDecl::compile(
    std::shared_ptr<Scope> current_scope,
    std::shared_ptr<BuiltinTypeSuperDecl> superdecl,
    std::shared_ptr<const CST::TypeDecl> cst_node,
    Logger *logger) {
  auto superdecl_targs = superdecl->targs();

  if (superdecl_targs.has_value()) {
    if (cst_node->template_args_decl.has_value()) {
      if (auto confl = superdecl_targs.value().compare(subdecl->template_args))
    } else {
      // Panic! Incompatible template arguments declaration
    }
  }
}

void Root::compile(std::shared_ptr<const CST::TypeDecl> cst, Logger *logger) {
  switch (cst->action()) {
  case Lang::Action::Def: {
    if (cst->kind().has_value()) {
      switch (cst->kind().value()) {
      case CST::TypeDecl::Builtin: {
        BuiltinTypeDecl::compile(shared_from_this(), cst, logger);
        BuiltinTypeImpl::compile(shared_from_this(), cst, logger);

        auto subdecl = BuiltinTypeDecl::from_def(cst);

        if (superdecl.has_value()) {
          // Check if it's actually a builtin type superdecl.
          //

          auto builtin_superdecl =
              std::get_if<std::shared_ptr<BuiltinTypeSuperDecl>>(
                  &superdecl.value());

          if (builtin_superdecl) {
            // Compile the `decl` part of the `def`.
            //

            auto subdecl = BuiltinTypeDecl::from_def(cst);

            // First of all, check template arg declaration compatibility.
            //

            auto confl = builtin_superdecl->get()->template_args.compare(
                subdecl->template_args);

            if (confl.has_value()) {
              // Template argument declaration conflict! Would panic.
              //

              if (confl->first && confl->second)
                throw Panic(
                    "Incompatible template argument declaration",
                    confl->first->placement(),
                    {{"Conflicting declaration here",
                      confl->second->placement()}});
              else if (confl->first && !confl->second)
                throw Panic(
                    "Incompatible template argument declaration",
                    confl->first->placement(),
                    {{"Conflicting declarations here",
                      builtin_superdecl->get()->template_args.placement()}});
              else if (!confl->first && confl->second)
                throw Panic(
                    "Incompatible template argument declaration",
                    cst->template_args.placement(),
                    {{"Conflicting declaration here",
                      confl->second->placement()}});
              else
                throw "BUG:";
            }

            builtin_superdecl->get()->add_subdecl(subdecl);

            /// Now, add the impl.
            ///

            auto impl = BuiltinTypeImpl::from_def(cst);
            builtin_superdecl->get()->add_impl(impl);
          } else {
            /// For example:
            ///
            /// ```
            /// Panic! `Enumerable` already declared as `trait`, not `builtin`
            /// type
            ///
            ///   builtin Enumerable<T> {
            ///   ^
            ///
            /// Note: Previously declared here
            ///
            ///   trait Enumerable<T> {
            ///   ^
            /// ```
            throw Panic(
                fmt::format(
                    "`{0}` already declared as `{1}`, not `builtin` type",
                    cst->name(),
                    std::visit(
                        [](auto &decl) { return decl->kind(); },
                        superdecl.value())),
                cst->kind_token.value().placement, // TODO:
                {{"Previously declared here",
                  std::visit(
                      [](auto &decl) { return decl->initial_placement(); },
                      superdecl.value())}});
          }
        } else {
          if (cst->id->path.size() == 1) {
            /// If it's a single-element ID, e.g. `Foo` or `::Foo`.
          } else {
            /// Get `Foo` for a multi-element ID `Foo::Bar`.
            auto parent_superdecl = lookup(cst->id->to_lang_id().pop());

            if (!parent_superdecl.has_value())
            //
          }
        }
      }
      case CST::TypeDecl::Trait:
        // _compile_name_space<PrimitiveDecl, PrimitiveImpl>(cst, logger);
        break;
      }
    } else {
      // TODO: `def` requires the kind.
      throw "BUG:";
    }
  }
  }
}

void AST::NameSpace::compile(
    std::shared_ptr<const CST::NameSpace> cst, Logger *logger) {
  switch (cst->kind()) {
  case CST::NameSpace::Namespace:
    _compile_name_space_decl<NamespaceDecl>(cst, logger);
    break;
  case CST::NameSpace::Primitive:
    _compile_name_space<PrimitiveDecl, PrimitiveImpl>(cst, logger);
    break;
  }
}

void AST::NameSpace::compile(
    std::shared_ptr<const Onyx::CST::Func> cst, Logger *logger) {
  switch (cst->kind()) {
  case CST::Func::Def: {
    auto new_decl = FuncDecl::compile(cst, weak_from_this(), logger);
    auto name = cst->id->name();

    if (!_func_decls.contains(id)) {
      _func_decls.insert({id, std::make_shared<FuncDecl>(new_decl)});
    } else {
      auto range = _func_decls.equal_range(id);
      bool match = false;

      for (auto it = range.first; it != range.second; ++it) {
        if ((*it->second <=> new_decl) == std::partial_ordering::equivalent) {
          match = true;
          break;
        }
      }

      if (match)
        return; // Declaration already exists
      else
        _func_decls.insert({id, std::make_shared<FuncDecl>(new_decl)});
    }

    // 2. Create implementation or throw if exist.
  }
  }
}

AST::Table::LookupResult AST::Namespace::lookup(CST::Id::Path) const {
  if (path.front()->access() == CST::Id::Part::Static) {
    if (top_level().lock().get() == this) {
      // It's a top level, would continue the lookup within self.
    } else
      // Delegate the lookup to the parent until it's the top level.
      return parent.lock()->_lookup_decl(path);
  }

  // Recursive lookup within self.
  //

  auto name = path.front()->name_token.value;

  if (_decls.contains(name)) {
    bool is_terminal = path.size() == 1;

    if (is_terminal) {
      return _decls.at(name);
    } else {
      // Dig into every declaration trying to find the needle.
      //

      auto current_part = path.front();
      auto slice = std::vector(path.begin() + 1, path.end());
      std::optional<Decl> result;

      std::visit(
          [&result, &slice, current_part](auto &decl) {
            using T = std::decay_t<decltype(decl)>;

            if constexpr (std::is_same_v<T, std::shared_ptr<TypeSuperDecl>>)
              result.emplace(decl->_lookup_decl(slice));
            else if constexpr (
                std::is_same_v<T, std::shared_ptr<VarDecl>> ||
                std::is_same_v<T, std::shared_ptr<FuncSuperDecl>>) {
              throw NotAScope(decl, current_part);
            } else
              static_assert(Util::Variant::False<T>, "Non-exaustive");
          },
          _decls.at(name));

      if (result.has_value())
        return result.value(); // Found!
    }

    return std::nullopt; // Did not find anything
  }
}

std::optional<std::shared_ptr<FuncSuperDecl>>
_lookup_func_decl(std::vector<std::shared_ptr<CST::Id::Part>> path) {
  auto decl = _lookup_decl(path);

  if (!decl)
    return std::nullopt;

  if (auto f = std::get_if<std::shared_ptr<FuncSuperDecl>>(&decl.value()))
    return *f;
  else
    throw TypeMismatch(decl.value());
}

/// Compile a name space from a CST node. A name space may be either a
/// declaration, implementation or definition of a type.
void AST::Scope::compile(
    std::shared_ptr<const CST::TypeDecl> cst_node, Logger *logger) {
  switch (cst_node->action()) {
  case CST::TypeDecl::Action::Decl: {
    _compile_name_space_decl<DeclT>(cst_node, logger);
  }

  case CST::TypeDecl::Action::Impl: {
    auto decl = _lookup<DeclT>(cst_node->id.get());

    if (!decl)
      throw Panic(
          "Unable to implement an undeclared type `" + cst_node->id->name() +
              "`",
          cst_node->id->placement());

    if (decl->impls.size() > 0)
      throw Panic(
          "Alredy implemented type `" + cst_node->id->name() + "`",
          cst_node->id->placement(),
          {{"Previously implemented here",
            decl->impls.back().cst_node->id->placement()}});

    // CASE: Create a new implementation.
    decl->impls.push_back(ImplT::compile(cst_node, decl, logger));
  }

  case CST::TypeDecl::Action::Def: {
    auto decl = _lookup<DeclT>(cst_node->id.get());

    if (!decl) {
      // If the declaration doesn't exist, create a empty one.
      auto decl = DeclT(cst_node, shared_from_this());
      _decls.insert({cst_node->id->name(), decl});
    }

    decl->impls.push_back(ImplT::compile(cst_node, decl, logger));
  }
  }
}

/// Compile a name space declaration (neither an implementation nor a
/// definition). Can throw `TypeMismatch` or `PartialLookupFailure`.
template <typename DeclT>
std::shared_ptr<DeclT> _compile_name_space_decl(
    std::shared_ptr<const CST::NameSpace> cst, Logger *logger) {
  auto existing_decl = _lookup_decl<DeclT>(cst->id.get());
  auto new_decl = DeclT::compile(cst, shared_from_this(), logger);

  if (existing_decl) {
    existing_decl->parent._decls.insert({cst->id->name(), new_decl})
  } else {
    auto parent = _lookup_decl(cst->id->parts);
    _decls.insert({cst->id->name(), decl});
  }

  return decl;
}

AST::TypeRefExpr AST::TypeRefExpr::compile(
    std::shared_ptr<const CST::TypeExpr> cst,
    std::weak_ptr<Namespace> ns,
    Logger *logger) {
  return std::visit(
      [ns, logger](auto &node) {
        using T = std::decay_t<decltype(node)>;

        // CST::LiteralRestriction
        if constexpr (std::is_same_v<
                          T,
                          std::shared_ptr<CST::LiteralRestriction>>) {
          std::shared_ptr<const CST::LiteralRestriction> const_ptr(node);
          return TypeRefExpr(LiteralRestriction(node));
        }

        // CST::Id
        else if constexpr (std::is_same_v<T, std::shared_ptr<CST::Id>>) {
          return TypeRefExpr(TypeRef::compile(node, ns, logger));
        }

        // CST::BasicLiteral
        else if constexpr (std::is_same_v<T, CST::BasicLiteral>) {
          return std::visit(
              [](auto &literal_node) {
                using T = std::decay_t<decltype(literal_node)>;

                // CST::BoolLiteral
                if constexpr (std::is_same_v<
                                  T,
                                  std::shared_ptr<CST::BoolLiteral>>) {
                  return TypeRefExpr(BoolLiteral(literal_node));
                }

                // CST::IntLiteral
                else if constexpr (std::is_same_v<
                                       T,
                                       std::shared_ptr<CST::IntLiteral>>) {
                  return TypeRefExpr(IntLiteral(literal_node));
                }

                else
                  static_assert(
                      Util::Variant::False<T>, "Non-exhaustive visitor!");
              },
              node);

        }

        else
          static_assert(Util::Variant::False<T>, "Non-exhaustive visitor!");
      },
      cst->value);
}

std::unique_ptr<AST::TypeSpec> AST::TypeSpec::compile(
    std::shared_ptr<const CST::Id> cst,
    std::weak_ptr<Namespace> ns,
    Logger *logger) {}

std::unique_ptr<AST::TypeSpec> AST::TypeSpec::compile(
    CST::BasicLiteral cst, std::weak_ptr<Namespace> ns, Logger *logger) {}

std::partial_ordering AST::TypeExpr::operator<=>(TypeExpr const &other) const {
  if (auto left = std::get_if<LiteralRestriction>(&this->value)) {
    if (auto right = std::get_if<LiteralRestriction>(&other.value)) {
      // CASE: Compare literal restrictions.
      if (left->kind == right->kind)
        return std::partial_ordering::equivalent;
      else
        return std::partial_ordering::unordered;
    } else {
      // CASE: Self is literal restriction, other is type spec restriction.
      return std::partial_ordering::unordered;
    }
  } else if (
      auto right = std::get_if<std::shared_ptr<TypeSpec>>(&other.value)) {
    // CASE: Compare type specs.
    return *std::get<std::shared_ptr<TypeSpec>>(this->value).get() <=>
           *right->get();
  } else {
    // CASE: Self is type spec restriction, other is literal.
    return std::partial_ordering::unordered;
  }
}

std::shared_ptr<AST::FuncDecl> AST::FuncDecl::compile(
    std::shared_ptr<const CST::Func> cst,
    std::weak_ptr<Namespace> parent,
    Logger *logger) {
  if (cst->kind() != CST::Func::Kind::Decl)
    throw "BUG: Must be a decl CST node";

  auto func_decl = std::make_shared<FuncDecl>(cst, parent);

  for (auto &arg : cst->args) {
    func_decl->runtime_args.insert(
        {arg->id.value, VarDecl::compile(arg, func_decl, logger)});
  }

  if (cst->return_type) {
    func_decl->return_type.emplace(
        TypeExpr::compile(cst->return_type, parent, logger));
  }

  return func_decl;
}

std::partial_ordering AST::FuncDecl::operator<=>(FuncDecl const &other) const {
  auto id_cmp = this->id.compare(other.id);
  if (id_cmp)
    // CASE: Different IDs.
    return id_cmp <=> 0;

  auto arg_size_cmp = this->args.size() - other.args.size();
  if (arg_size_cmp)
    // CASE: Different arity.
    return arg_size_cmp <=> 0;

  for (auto &pair : this->args) {
    if (!Util::Map::contains(other.args, pair.first))
      // CASE: Argument ID mismatch.
      return std::partial_ordering::unordered;

    auto left_opt = pair.second.default_value.has_value();
    auto right_opt = other.args.at(pair.first).default_value.has_value();

    if (left_opt != right_opt)
      // CASE: Optional-ity mismatch.
      return left_opt ? std::partial_ordering::less
                      : std::partial_ordering::greater;

    auto type_cmp = *pair.second.restriction.get() <=>
                    *other.args.at(pair.first).restriction.get();
    if (type_cmp != std::partial_ordering::equivalent)
      // CASE: Type mismatch.
      return type_cmp;
  }

  return std::partial_ordering::equivalent;
}

std::partial_ordering AST::TypeSpec::operator<=>(TypeSpec const &other) const {
  // TODO: Ancestors & descendants:
  //
  // `Foo extends Bar::Foo` => `Foo < Bar::Foo` (same id, but diff namespaces)
  // `Foo extends Bar` => `Foo < Bar` (different ids, but inheritance)
  //
  // NOTE: `Int32 == Integer<true, 32>`, because aliases are resolved at this
  // point.
  //

  if (this->decl->id != other.decl->id)
    // CASE: Type declaration IDs are different.
    return std::partial_ordering::unordered;

  if (this->decl->parent != other.decl->parent)
    // CASE: Namespaces containing the declarations are different.
    return std::partial_ordering::unordered;

  auto arg_size_cmp = this->args.size() <=> other.args.size();
  if (arg_size_cmp != std::strong_ordering::equal)
    // CASE: Different arity.
    return arg_size_cmp;

  for (auto &pair : args) {
    if (!Util::Map::contains(other.args, pair.first))
      // CASE: Different args specialized, e.g. `Integer<Bitsize: 32> !=
      // Integer<Signed: true>`.
      return std::partial_ordering::unordered;

    if (auto left_literal = std::get_if<Literal>(&pair.second)) {
      auto right_literal = std::get_if<Literal>(&other.args.at(pair.first));

      if (!right_literal)
        // CASE: Right arg is not a literal, thus uncomparable.
        return std::partial_ordering::unordered;

      auto cmp = std::visit(
          [&right_literal](auto &left) {
            using T = std::decay_t<decltype(left)>;

            if (auto right = std::get_if<T>(right_literal)) {
              if (left.value == right->value)
                // CASE: Literal values are equal.
                return std::partial_ordering::equivalent;
              else
                // CASE: Literal values differ.
                return std::partial_ordering::unordered;
            } else
              // CASE: Right arg is not a literal.
              return std::partial_ordering::unordered;
          },
          *left_literal);

      if (cmp != std::partial_ordering::equivalent)
        // CASE: Literal args are not equal.
        return cmp;
    } else if (
        auto left_type = std::get_if<std::shared_ptr<TypeSpec>>(&pair.second)) {
      auto right_type =
          std::get_if<std::shared_ptr<TypeSpec>>(&other.args.at(pair.first));

      if (!right_type)
        // CASE: Right arg is not a type specialization, thus uncomparable.
        return std::partial_ordering::unordered;

      auto cmp = *left_type->get() <=> *right_type->get();
      if (cmp != std::partial_ordering::equivalent)
        // CASE: Type args are not equal.
        return cmp;
    } else {
      // TODO: Make it visit as well?
      assert(false && "BUG: Unimplemented arg kind");
    }
  }

  return std::partial_ordering::equivalent;
}

AST::AST(
    std::filesystem::path file_path,
    const CST *cst,
    std::shared_ptr<Logger> logger) :
    _logger(logger),
    _top_level(std::make_unique<TopLevel>(file_path)),
    _c_ast(std::make_unique<C::AST>(logger->dup("c_ast"))) {
  for (auto &child : cst->children()) {
    std::visit(
        [this](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, std::shared_ptr<Onyx::CST::Extern>>) {
            // For now, `extern` is only valid at the top level.
            this->_compile(arg);
          } else if constexpr (
              std::is_same_v<T, std::shared_ptr<Onyx::CST::Func>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::NamespaceDecl>>) {
            this->_top_level->Namespace::compile(arg, _logger.get());
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<Onyx::CST::VarDecl>>) {
            this->_top_level->Scope::compile(arg, _logger.get());
          } else if constexpr (
              std::is_same_v<T, std::shared_ptr<Onyx::CST::Call>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::ExplSafety>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::UnOp>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::BinOp>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::If>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::While>> ||
              std::is_same_v<T, std::shared_ptr<Onyx::CST::Block>>) {
            this->_top_level->compile(arg, _logger.get());
          } else if constexpr (std::is_same_v<
                                   T,
                                   std::shared_ptr<Onyx::CST::EmptyLine>>) {
            //  Do nothing.
          } else
            static_assert(Util::Variant::False<T>, "Non-exhaustive visitor!");
        },
        child);
  }
}

void AST::_compile(std::shared_ptr<const Onyx::CST::Extern> cst) {
  _c_ast->compile(cst->block->cst());
}

} // namespace Fancysoft::NXC::Onyx::AST
