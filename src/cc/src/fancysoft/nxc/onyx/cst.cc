#include <fmt/ostream.h>
#include <ostream>

#include "fancysoft/nxc/onyx/cst.hh"
#include "fancysoft/nxc/onyx/token.hh"
#include "fancysoft/nxc/storage.hh"

namespace Fancysoft::NXC::Onyx {

#pragma region Extern

// void CST::Extern::inspect(std::ostream &o, unsigned short i) const {
//   fmt::print(o, "{0}<{1}>\n", inspect_prefix(i), node_name());
//   this->block->ast()->inspect(o, i + 1);
// }

void CST::Extern::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << "extern ";
  this->block->cst()->print(o);
}

#pragma endregion

#pragma region Var

// void CST::Var::inspect(std::ostream &o, unsigned short indent) const {
//   std::string writeability;

//   fmt::print(
//       o,
//       "{0}<{1}>\n{2}Writeability: {3}\n{2}Id: {4}\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->writeability_str(),
//       this->id.Token::inspect());

//   if (this->type) {
//     o << inspect_attribute_prefix(indent) << "Type:\n";
//     this->type->inspect(o, indent + 1);
//   }

//   if (this->value.has_value()) {
//     o << inspect_attribute_prefix(indent) << "Value:\n";

//     std::visit(
//         [&o, indent](auto &&node) { node->inspect(o, indent + 1); },
//         *this->value);
//   }
// }

void CST::Var::trace(std::ostream &o) const {
  fmt::print(o, "<{} {}>", node_name(), id.id);
}

void CST::Var::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  if (this->writeability() != Undefined) {
    o << this->writeability_str() << ' ';
  }

  o << this->id.id;

  if (this->type) {
    o << " : ";
    this->type->print(o);
  }

  if (this->value) {
    o << " = ";

    std::visit(
        [&o, indent](auto &node) { node->print(o); }, this->value.value());
  }
}

#pragma endregion

#pragma region Func

// void CST::Func::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}{1}\n{2}Id: {3}\n{2}Args:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->id.Token::inspect());

//   for (auto &arg_decl : this->args) {
//     arg_decl->inspect(o, indent + 1);
//   }

//   if (this->body.has_value()) {
//     o << inspect_attribute_prefix(indent) << "Body:\n";
//     this->body.value()->inspect(o, indent + 1);
//   }
// }

void CST::Func::trace(std::ostream &o) const {
  fmt::print(o, "<{} {}>", node_name(), id.id);
}

void CST::Func::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << "def " << this->id.id << '(';

  bool first = true;
  for (auto &arg : args) {
    if (first)
      first = false;
    else
      o << ", ";

    arg->print(o);
  }

  o << ')';

  if (this->return_type) {
    o << " : ";
    this->return_type->print(o);
  }

  if (this->body) {
    this->body.value()->print(o, indent);
  }
}

#pragma endregion

#pragma region Tuple

// void CST::Tuple::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}<{1}>\n{2}Size: {3}\nElements:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->size());

//   for (auto &el : elements) {
//     std::visit([&o, indent](auto &node) { node->inspect(o, indent + 1); },
//     el);
//   }
// };

void CST::Tuple::trace(std::ostream &o) const {
  fmt::print(o, "<{}[{}]>", node_name(), size());
};

void CST::Tuple::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  o << '(';

  for (auto &el : elements) {
    std::visit([&o, indent](auto &node) { node->print(o); }, el);
  }

  o << ')';
};

#pragma endregion

#pragma region Array

// void CST::Array::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}<{1}>\n{2}Size: {3}\n{2}Elements:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->size());

//   for (auto &el : elements) {
//     std::visit([&o, indent](auto &node) { node->inspect(o, indent + 1); },
//     el);
//   }
// };

void CST::Array::trace(std::ostream &o) const {
  fmt::print(o, "<{0}[{1}]>", node_name(), size());
};

void CST::Array::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  o << '[';

  for (auto &el : elements) {
    std::visit([&o, indent](auto &node) { node->print(o); }, el);
  }

  o << ']';
};

#pragma endregion

  // void CST::StringLiteral::inspect(
  //     std::ostream &o, unsigned short indent) const {
  //   fmt::print(
  //       o,
  //       "{0}{1}\n{2}Token: {3}\n",
  //       inspect_prefix(indent),
  //       node_name(),
  //       inspect_attribute_prefix(indent),
  //       this->token.Token::inspect());
  // }

#pragma region CString

// void CST::CString::inspect(std::ostream &o, unsigned short i) const {
//   fmt::print(
//       o,
//       "{0}<{1}>\n{2}Token: {3}\n",
//       inspect_prefix(i),
//       node_name(),
//       inspect_attribute_prefix(i),
//       this->token.Token::inspect());
// }

void CST::CString::trace(std::ostream &o) const {
  fmt::print(o, "<{} \"{}\">", node_name(), token.string);
};

void CST::CString::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << "$\"" << token.string << '"';
};

#pragma endregion

#pragma region Int

void CST::Int::trace(std::ostream &o) const {
  fmt::print(o, "<{0} {1}>", node_name(), token.value);
};

void CST::Int::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << token.value;
};

#pragma endregion

#pragma region Id

// void CST::Id::PathElement::inspect(std::ostream &o, unsigned indent) const {
//   fmt::print(o, "{0}<{1}>\n")
// }

void CST::Id::PathElement::print(std::ostream &o) const {
  switch (this->access) {
  case Undefined:
    break;
  case Static:
    o << "::";
    break;
  case Instance:
    o << '.';
    break;
  }

  o << this->token.id;

  if (this->vargs.size() > 0 || this->kwargs.size() > 0) {
    o << '<';

    bool first = true;

    for (auto &varg : vargs) {
      if (first)
        first = false;
      else
        o << ", ";

      varg.second->print(o);
    }

    for (auto &kwarg : kwargs) {
      if (first)
        first = false;
      else
        o << ", ";

      o << kwarg.first.value << ": ";
      kwarg.second->print(o);
    }

    o << '>';
  }
}

// void CST::Id::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}<{1}>\n{2}Path elements:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent));

//   for (auto &p : path) {
//     p.inspect(o, indent + 1);
//   }
// }

void CST::Id::trace(std::ostream &o) const { print(o); }

void CST::Id::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);

  for (auto &p : path) {
    p.print(o);
  }
}

#pragma endregion

#pragma region CId

void CST::CId::trace(std::ostream &o) const {
  fmt::print(o, "<{0} $`{1}`>", node_name(), token.id);
}

void CST::CId::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << '$' << this->token.id;
}

#pragma endregion

#pragma region Call

void CST::Call::trace(std::ostream &o) const {
  o << '<' << node_name() << ' ';
  std::visit([&o](auto &val) { val->trace(o); }, this->callee);
  o << '(' << this->args.size() << ")>";
}

void CST::Call::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  std::visit([&o](auto &val) { val->print(o); }, this->callee);
  this->args.print(o);
}

// void CST::CCall::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}{1}\n{2}Callee id: {3}\n{2}Arguments:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->callee.Token::inspect());

//   for (auto &node : this->arguments) {
//     std::visit(
//         [&o, indent](auto &&node) { node->inspect(o, indent + 1); }, node);
//   }
// }

#pragma endregion

#pragma region UnOp

// void CST::UnOp::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}{1}\n{2}Operator: {3}\n{2}Operand:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       operator_.Token::inspect());

//   std::visit(
//       [&o, indent](auto &&node) { node->inspect(o, indent + 1); },
//       this->operand);
// }

void CST::UnOp::trace(std::ostream &o) const {
  fmt::print(o, "<{0} {1}>", node_name(), operator_.op);
}

void CST::UnOp::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << this->operator_.op;
  std::visit([&o](auto &rval) { rval->print(o); }, this->operand);
}

#pragma endregion

#pragma region BinOp

// void CST::BinOp::inspect(std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}{1}\n{2}Left:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent));

//   std::visit(
//       [&o, indent](auto &&node) { node->inspect(o, indent + 1); },
//       this->left_operand);

//   fmt::print(
//       o,
//       "\n{0}Operator: {1}\n{0}Right:\n",
//       inspect_attribute_prefix(indent),
//       this->operator_.Token::inspect());

//   std::visit(
//       [&o, indent](auto &&node) { node->inspect(o, indent + 1); },
//       this->right_operand);
// }

void CST::BinOp::trace(std::ostream &o) const {
  o << "<" << node_name() << " `";
  std::visit([&o](auto &token) { token.print(o); }, this->op);
  o << "`>";
}

void CST::BinOp::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  std::visit([&o](auto &rval) { rval->print(o); }, this->left);
  o << " ";
  std::visit([&o](auto &token) { token.print(o); }, this->op);
  o << " ";
  std::visit([&o](auto &rval) { rval->print(o); }, this->right);
}

#pragma endregion

void CST::Branch::print(std::ostream &o, unsigned indent) const {
  if (this->keyword) {
    this->keyword.value().print(o);
  }

  if (auto rval = std::get_if<RVal>(&this->body)) {
    o << ' ';
    std::visit([&o, indent](auto &rval) { rval->print(o, indent); }, *rval);
  } else {
    auto block = std::get<std::shared_ptr<Block>>(this->body);

    if (!block->is_begin_newline())
      o << ' ';

    block->print(o, indent);
  }
}

#pragma region ExplSafety

// void CST::ExplicitSafetyStatement::inspect(
//     std::ostream &o, unsigned short indent) const {
//   fmt::print(
//       o,
//       "{0}{1}\n{2}Safety: {3}\n{2}Expressions:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->safety());

//   std::visit(
//       [&o, indent](auto &&node) { node->inspect(o, indent + 1); },
//       this->value);
// }

void CST::ExplSafety::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  this->keyword.print(o);
  o << ' ';
  this->value.print(o, indent);
}

#pragma endregion

void CST::Case::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  this->keyword.print(o);
  o << ' ';
  std::visit([&o](auto &rval) { rval->print(o); }, this->cond);
  o << ' ';
  this->branch.print(o, indent);
}

void CST::Else::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  this->keyword.print(o);
  o << ' ';
  this->branch.print(o, indent);
}

#pragma region If

void CST::If::print(std::ostream &o, unsigned indent) const {
  this->self.print(o, indent);

  for (auto &elif : this->elifs) {
    o << '\n';
    elif.print(o, indent);
  }

  if (this->else_) {
    o << '\n';
    this->else_->print(o, indent);
  }
}

#pragma endregion

#pragma region While

void CST::While::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << "while ";
  std::visit([&o](auto &rval) { rval->print(o); }, this->cond);
  o << ' ';
  this->branch.print(o, indent);
}

#pragma endregion

#pragma region Return

// void CST::ReturnStatement::inspect(
//     std::ostream &out, unsigned short indent) const {
//   fmt::print(out, "{0}{1}\n", inspect_prefix(indent), this->node_name());

//   if (this->value.has_value()) {
//     out << inspect_attribute_prefix(indent) << "Value:\n";

//     std::visit(
//         [&out, indent](auto &rval) { rval->inspect(out, indent + 1); },
//         this->value.value());
//   }
// }

void CST::Return::trace(std::ostream &o) const {
  fmt::print(o, "<{0}{1}>", node_name(), value.has_value() ? " ..." : "");
}

void CST::Return::print(std::ostream &o, unsigned indent) const {
  print_tab(o, indent);
  o << "return";

  if (this->value) {
    o << ' ';
    std::visit([&o](auto &rval) { rval->print(o); }, this->value.value());
  }
}

#pragma endregion

#pragma region Block

// void CST::Block::inspect(std::ostream &out, unsigned short indent) const {
//   fmt::print(
//       out,
//       "{0}{1}\n{2}Style: {3}\n{2}Exprs:\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       style_to_string(this->style));

//   for (auto &expr : this->exprs) {
//     std::visit(
//         [&out, indent](auto &expr) { expr->inspect(out, indent + 1); },
//         expr);
//   }
// }

void CST::Block::print(std::ostream &o, unsigned indent) const {
  bool is_do = !!(std::get_if<Token::Keyword>(&this->begin));

  if (is_do) {
    o << "do";
  }

  bool is_c = this->style == Style::C;

  if (is_c)
    o << " {";

  for (auto &node : nodes) {
    std::visit(
        [this, &o, indent](auto &node) {
          if (this->is_multiline) {
            o << "\n";
            node->print(o, indent + 1);
          } else {
            o << "; ";
            node->print(o);
          }
        },
        node);
  }

  if (this->is_multiline) {
    o << "\n";
    print_tab(o, indent);
  } else if (!is_c)
    o << "; ";

  if (is_c)
    o << "}";
  else if (std::get_if<Token::Keyword>(&this->end))
    o << "end";
}

#pragma endregion

#pragma region TypeExpr

// void CST::TypeExpr::inspect(std::ostream &out, unsigned short indent) const {
//   fmt::print(
//       out,
//       "{0}{1}\n{2}Id: {3}\n",
//       inspect_prefix(indent),
//       node_name(),
//       inspect_attribute_prefix(indent),
//       this->id.Token::inspect());
// }

void CST::TypeExpr::print(std::ostream &o, unsigned indent) const {
  this->id->print(o, indent);
}

#pragma endregion

// void CST::inspect(std::ostream &o, unsigned short i) const {
//   fmt::print(o, "{0}<{1}>\n", inspect_prefix(i), node_name());

//   for (auto &node : this->_children) {
//     std::visit([&o, i](auto &&node) { node->inspect(o, i + 1); }, node);
//   }

//   o << "\n";
// }

void CST::print(std::ostream &o, unsigned indent) const {
  for (auto &child : _children) {
    std::visit([&o](auto &node) { node->print(o); }, child);
    o << '\n';
  }
}

} // namespace Fancysoft::NXC::Onyx
