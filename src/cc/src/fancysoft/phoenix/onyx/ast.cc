#include <assert.h>
#include <memory>
#include <type_traits>
#include <variant>

#include "fancysoft/phoenix/onyx/lang.hh"
#include "fmt/core.h"

#include "fancysoft/phoenix/onyx/ast.hh"
#include "fancysoft/phoenix/onyx/file.hh"
#include "fancysoft/phoenix/onyx/token.hh"
#include "fancysoft/phoenix/program.hh"
#include "fancysoft/phoenix/util/lambda.hh"
#include "fancysoft/phoenix/util/map.hh"
#include "fancysoft/phoenix/util/variant.hh"

namespace Fancysoft::Phoenix::Onyx {

template <typename CSTNodeT> std::string AST::Ion<CSTNodeT>::doc() const {
  if (doc_cst_node) {
    std::stringstream stream;

    bool first = true;
    for (auto token : doc_cst_node->tokens) {
      if (first)
        first = false;
      else
        stream << "\n";

      stream << token.value;
    }

    return stream.str();
  } else {
    return nullptr;
  }
}

void AST::compile(CST *cst) {
  if (!_root)
    _root = std::make_shared<Root>(Root(this));

  bool set_adjacent_comment = false;
  for (auto &node : cst->root) {
    if (auto comment = std::dynamic_pointer_cast<CST::Comment>(node)) {
      set_adjacent_comment = true;
      _adjacent_comment = comment;
    } else {
      _root->compile(node);
    }

    if (set_adjacent_comment)
      set_adjacent_comment = false;
    else
      _adjacent_comment = nullptr; // Reset if it wasn't a comment node
  }
}

template <typename SupT, typename DefT>
std::shared_ptr<DefT> AST::Root::compile(std::shared_ptr<CST::TypeDef> cst_node) {
  assert(
      !cst_node->action_keyword.has_value() ||
      cst_node->action_keyword->kind == Token::Keyword::Def);

  auto id = cst_node->id_query->simple_id();
  if (!id)
    throw "Panic! Expected simplie ID here";

  auto found = this->lookup(id);
  std::shared_ptr<SupT> superion;

  if (found) {
    if ((superion = std::dynamic_pointer_cast<std::shared_ptr<SupT>>(found))) {
      // Panic! Already defined
    } else {
      // Panic! Already declared as `class`
    }
  }

  superion = std::make_shared<SupT>(SupT());

  superion->def = std::make_shared<DefT>(
      DefT(superion, shared_from_this(), cst_node, ast()->adjacent_comment()));

  if (cst_node->template_args.has_value()) {
    for (auto targ : cst_node->template_args->decls) {
      superion->def->compile_targ(targ);
    }
  }

  this->_add_superion(superion);
  return superion->def;
}

template <>
std::shared_ptr<AST::TraitDef>
AST::Root::compile<AST::TraitSuperion>(std::shared_ptr<CST::TypeDef> cst_node);

template <>
std::shared_ptr<AST::StructDef>
AST::Root::compile<AST::StructSuperion>(std::shared_ptr<CST::TypeDef> cst_node);

std::shared_ptr<AST::Entity> AST::SyntaxScope::lookup(std::shared_ptr<CST::ID> id) {
  auto found = _search_in_this(id);

  switch (id->id.kind) {
  case Token::ID::Simple: {
    // Search builtin,
    // then parent syntax
  }

  case Token::ID::Literal: {
    switch (id->literal().value()) {
    case Lang::IDLiteral::Void: // `void`
      if (found)
        throw AlreadyDeclared(Lang::id_literal_string(Lang::IDLiteral::Void), found, id);
      else
        found = std::make_shared<IDLiteral>(IDLiteral(id));

    case Lang::IDLiteral::Nil: // `nil` ≡ `0u0`
      return std::make_shared<IDLiteral>(IDLiteral(id));

    case Lang::IDLiteral::Self:
      // TODO: Return self type? Can be used outside of type.
      // Returns root unit when used in root (God object, fragile access).
      // `self::` in root unit references its static vars.

    case Lang::IDLiteral::This:
      // Returns the caller copy instance in an instance field definition or a
      // function implementation.
      if (auto parent = parent_syntax_scope->lock()) {
        if (auto cast = std::dynamic_pointer_cast<FunctionDef>(parent)) {
          if (found)
            throw "TODO: Panic! Already declared `this` in this context";
          else
            found = std::make_shared<This>(This(cast));
        }

        else if (auto cast = std::dynamic_pointer_cast<FunctionImpl>(parent)) {
          if (found)
            throw "TODO: Panic! Already declared `this` in this context";
          else
            found = std::make_shared<This>(This(cast));
        }

        else if (auto cast = std::dynamic_pointer_cast<VarDef>(parent)) {
          if (found)
            throw "TODO: Panic! Already implicitly declared `this` in this context";
          else
            found = std::make_shared<This>(This(cast));
        }
      }

      // TODO: Allow `this` as local variable in static functions?
      // Or `this` expands to `Root` akin to `self`?
      throw "TODO: Panic can't use `this` in this context";
    }
  }

  case Token::ID::C: {
    // Search C AST (static, then global)
  }

  case Token::ID::Intrinsic: {
    // Lookup an intrinsic.
  }

  case Token::ID::Label: {
    // Ensure it doesn't conflict with local declarations, e.g. `def foo(a: b, a)` panics.
  }

  case Token::ID::Symbol: {
    // Semantic lookup instead?
  }
  }
}

std::shared_ptr<AST::Entity>
AST::SemanticScope::resolve(std::shared_ptr<CST::IDQuery> cst_node) const {
  std::shared_ptr<AST::Entity> found;

  for (auto &element : cst_node->path) {
    std::visit(
        [this, &found](auto &el) {
          using namespace std;
          using T = decay_t<decltype(el)>;

          if constexpr (is_same_v<T, shared_ptr<CST::ID>>) {
            if (found)
              throw "TODO: Panic! Ambigous ID";

            found = resolve(el);
          } else if constexpr (is_same_v<T, shared_ptr<CST::SpecRef>>) {
            // TODO: Result may be var def or (template) superion
            auto ref = resolve(el->id);

            for (auto &arg : el->args)
              ref->add_targ(resolve(arg));

            found = ref;
          } else
            static_assert(Util::Variant::False<T>, "non-exhaustive visitor!");
        },
        element->value);

    if (found)
      return found;
  }

  throw "TODO: Panic! Not found";
}

void AST::Root::compile(std::shared_ptr<CST::TypeDef> cst_node) {
  auto type_ion_kind = cst_node->type_ion_kind();
  auto type_category = cst_node->type_category();

  if (type_ion_kind.has_value()) {
    switch (type_ion_kind.value()) {
    case Lang::TypeIonKind::Definition: {
      goto switch_type_category;
    }

    case Lang::TypeIonKind::Extension: {
      switch (type_category.value()) {
      case Lang::TypeCategory::Trait:
        TraitExt::compile(shared_from_this(), cst_node);
        break;
      case Lang::TypeCategory::Struct:
        StructExt::compile(shared_from_this(), cst_node);
        break;
      }
    }
    }
  } else if (type_category.has_value()) {
  switch_type_category:
    switch (type_category.value()) {
    case Lang::TypeCategory::Trait:
      TraitDef::compile(shared_from_this(), cst_node);
      break;
    case Lang::TypeCategory::Struct:
      StructDef::compile(shared_from_this(), cst_node);
      break;
    }
  }
}

std::shared_ptr<AST::TemplateArgDecl> AST::TemplateArgDecl::compile_targ(
    Container container, std::shared_ptr<CST::VarDef> cst_node) {
  // A template argument identifier may not shadow an outer variable, container,
  // or another template identifier.
  //
  // ```nx
  // struct U {}
  // # def foo<U>() -> U()    # => Panic! Which one? (1)
  // # def foo<T: U>() -> U() # => Panic! Still shadows (2)
  // def foo<U: T>() -> U()   # OK
  //
  // # def foo<foo>() -> foo() # => Panic! Same name as container (3)
  // def foo<foo: T>() -> T()  # OK
  //
  // # def foo<A, A>() -> A() # => Panic! Already declared targ with name (4)
  // ```
  //
  // IDEA: An alternative would be require to always specify exact scope,
  // otherwise it's local.
  //
  // ```nx
  // def foo<U>() -> ::U()  # OK, call outer `U`
  // def foo<U>() -> U()    # OK, call local `U` (shadowing)
  // def foo<U: T>() -> U() # OK, call outer `U`
  //
  // def foo<foo>() -> ::foo() # OK, call the outer `foo` (the function)
  // def foo<foo>() -> foo()   # OK, call the local `foo` (the type)
  // def foo<foo: T>() -> T()  # OK, call local `T`
  // ```
  //

  auto container_id = std::visit([](auto cont) { return cont->id_string(); }, container);

  if (cst_node->id_string() == container_id)
    // (3).
    throw "Panic! A template argument ID shall not shadow its parent name";

  if (auto found = std::visit(
          [id = cst_node->id_string()](auto cont) { return cont->find_targ(id); },
          container)) {
    // (4).
    throw "Panic! Already declared argument with this ID";
  }

  std::shared_ptr<Scope> scope;

  std::visit([&scope](auto &cont) { scope = cont->parent_scope.lock(); }, container);

  if (auto found = scope->lookup(cst_node->id_string(), false))
    // (1) and (2).
    throw "Panic! Template argument shadowing is prohobited";

  std::shared_ptr<Restriction> restriction;

  if (cst_node->restriction)
    restriction = Restriction::compile(scope, cst_node->restriction);

  return std::make_shared<TemplateArgDecl>(
      TemplateArgDecl(cst_node, scope->adjacent_comment(), container, restriction));
}

std::shared_ptr<AST::Restriction> AST::Restriction::compile(
    std::shared_ptr<Scope> scope, std::shared_ptr<CST::Restriction> cst_node) {
  // NOTE: Real type concreteness is checked during specialization.
  // All we want now is to check declarations existence.
  //

  std::optional<Expression> real_part;
  if (cst_node->real_part)
    real_part = resolve(scope, cst_node->real_part.value());

  std::optional<Expression> virtual_part;
  if (cst_node->virtual_part)
    virtual_part = resolve(scope, cst_node->virtual_part.value());

  return std::make_shared<Restriction>(Restriction(cst_node, real_part, virtual_part));
}

AST::Expression
AST::resolve(std::shared_ptr<Scope> scope, CST::Expression cst_expression) {
  return std::visit([scope](auto expr) { return resolve(scope, expr); }, cst_expression);
}

AST::Expression
AST::resolve(std::shared_ptr<Scope> scope, std::shared_ptr<CST::ID> cst_node) {
  // TODO:
  if (auto found = scope->lookup(cst_node->id(), true)) {
    return std::make_shared<AST::EntityRef>(AST::EntityRef(cst_node, found));
  }
}

AST::Expression
AST::resolve(std::shared_ptr<Scope> scope, std::shared_ptr<CST::IDQuery> cst_node) {}
AST::Expression
AST::resolve(std::shared_ptr<Scope> scope, std::shared_ptr<CST::UnOp> cst_node) {}
AST::Expression
AST::resolve(std::shared_ptr<Scope> scope, std::shared_ptr<CST::BinOp> cst_node) {}

// std::variant<std::shared_ptr<AST::Self>, std::shared_ptr<AST::AnySuperdeclRef>>
// AST::_resolve(std::shared_ptr<CST::Query> cst_query) {
//   assert(cst_query);

//   /// TODO: Allow nested `self`, e.g. `T::self`.
//   /// TODO: Allow `self` w/ template args.
//   if (cst_query->is_self())
//     return std::make_shared<Self>(Self(cst_query));

//   /// TODO: Nested queries are not implemented yet.
//   if (cst_query->path.size() != 1)
//     throw "Unimplemented";

//   auto id = cst_query->last_id();

//   /// TODO: C entity lookups aren't implemented yet.
//   if (id.c)
//     throw "Unimplemented";

//   // TODO: Template args.

//   std::optional<AST::AnySuperdecl> superdecl;

//   /// 1. Lookup for a superdeclaration in self (also searches imports).
//   /// 2. Lookup for a superdeclaration in the global builtin AST.
//   if ((superdecl = lookup(id.value)) ||
//       (superdecl = _ctx.program->builtin_ast()->lookup(id.value)))
//     return std::make_shared<AnySuperdeclRef>(
//         AnySuperdeclRef(cst_query, id.escaped, superdecl.value()));

//   /// Panic if not found.
//   throw UndeclaredReference(id);
// }

} // namespace Fancysoft::Phoenix::Onyx
