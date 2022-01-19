#include "fancysoft/phoenix/onyx/cst.hh"

namespace Fancysoft::Phoenix::Onyx {

std::optional<Token::Keyword>
CST::Keywords::find(Token::Keyword::Kind kind) const {
  auto it = std::find_if(tokens.begin(), tokens.end(), [kind](auto &&tok) {
    return tok.kind == kind;
  });

  if (it == tokens.end())
    return std::nullopt; // Not found
  else
    return *it._Ptr;
}

bool CST::Keywords::includes(Token::Keyword::Kind kind) const {
  return std::find_if(tokens.begin(), tokens.end(), [kind](auto &&tok) {
           return tok.kind == kind;
         }) != tokens.end();
}

std::optional<Token::Keyword>
CST::Keywords::disjoint(std::set<Token::Keyword::Kind> allowed) const {
  for (auto token : tokens) {
    if (!allowed.contains(token.kind))
      return token;
  }

  return std::nullopt;
}

void CST::EmptyLine::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
}

void CST::Comment::print(std::ostream &o, unsigned indent) const {
  for (auto &token : tokens) {
    print_tab(o, indent);
    o << '#' << token.value << '\n';
  }
}

void CST::Extern::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << "extern ";
  block->print(o);
}

void CST::Import::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  o << "import ";

  bool wrapped = false;
  bool first = true;
  for (auto &element : elements) {
    // `import { a }, b, c, { d, e }` is legal (except `c`, but this is done
    // for example purposes).
    //

    bool need_comma = !first;

    if (first)
      first = false;

    if (element.destructed) {
      if (need_comma)
        o << ", ";

      if (!wrapped) {
        wrapped = true;
        o << "{ ";
      }
    } else {
      if (wrapped) {
        o << " }";
        wrapped = false;
      }

      if (need_comma)
        o << ", ";
    }

    std::visit([&o](auto token) { token.print(o); }, element.id);

    if (element.alias) {
      o << " as ";
      element.alias->alias_id.print(o);
    }
  }

  if (wrapped)
    o << " }";

  o << " from ";
  from_value.print(o);
}

void CST::Query::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  for (auto element : elements) {
    if (element->scope_access_token.has_value()) {
      element->scope_access_token.value().print(o);
    }

    element->name_token.print(o);

    if (element->args.size() > 0) {
      // TODO: Newlines between args, if any.
      //

      o << '<';

      bool first = true;
      for (auto varg : element->args) {
        if (first)
          first = false;
        else
          o << ", ";

        varg->print(o);
      }

      o << '>';
    }
  }
}

void CST::UnOp::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  _operator.print(o);
  std::visit([&o](auto node) { node->print(o); }, operand);
}

void CST::BinOp::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  std::visit([&o](auto node) { node->print(o); }, left_operand);
  o << ' ';
  _operator.print(o);
  o << ' ';
  std::visit([&o](auto node) { node->print(o); }, right_operand);
}

void CST::Restriction::print(std::ostream &o, unsigned indent) const {
  // NOTE: There is no identation. Maybe no need for a distinct `Restriction`
  // node?
  //

  if (real_part)
    o << " : ";
  else
    o << " ~ ";

  if (real_part)
    real_part->print(o);

  if (virtual_part.has_value()) {
    if (real_part)
      o << '~';

    std::visit([&o](auto node) { node->print(o); }, virtual_part.value());
  }
}

void CST::VarDecl::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  if (is_exported())
    o << "export ";

  if (is_exported_by_default())
    o << "default ";

  for (auto modifier : modifiers.tokens) {
    modifier.print(o);
    o << ' ';
  }

  if (alias) {
    alias->print(o);
    o << ' ';
  }

  id.print(o);

  if (restriction)
    restriction->print(o); // Would wrap in spaces

  if (value) {
    o << " = ";
    std::visit(
        [&o, indent](auto node) { node->print(o, indent); }, value.value());
  }
}

void CST::MultiVarDecl::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  bool first = true;
  for (auto decl : decls) {
    if (first)
      first = false;
    else
      o << ", ";

    decl->print(o, indent);
  }
}

void CST::Call::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  std::visit([&o, indent](auto node) { node->print(o, indent); }, callee);

  o << '(';

  bool first = true;
  for (auto arg : args) {
    if (first)
      first = false;
    else
      o << ", ";

    std::visit([&o, indent](auto node) { node->print(o, indent); }, arg);
  }

  o << ')';
}

} // namespace Fancysoft::Phoenix::Onyx
