#include <optional>
#include <variant>

#include "fancysoft/nxc/exception.hh"
#include "fancysoft/nxc/onyx/parser.hh"
#include "fancysoft/nxc/onyx/token.hh"
#include "fancysoft/util/map.hh"
#include "fancysoft/util/variant.hh"

namespace Fancysoft::NXC::Onyx {

std::unique_ptr<CST> Parser::parse() {
  _initialize();
  auto ast = std::make_unique<CST>();

  bool terminated = true;
  while (!_lexer_done()) {
    if (!terminated) {
      if (_is_term()) {
        terminated = true;
        continue;
      } else {
        throw _expected("terminator");
      }
    }

    if (_is_space() || _is_newline()) {
      _advance();
    } else {
      auto node = _parse_top_level_node();

      std::visit(
          [this](auto &node) {
            auto &log = _logger->sdebug();
            log << "Parsed ";
            node->trace(log);
            log << " in top level\n";
          },
          node);

      ast->add_child(node);
      terminated = false;
      _skip_space();
    }
  }

  _logger->sdebug() << "Done parsing\n";
  return ast;
}

CST::Extern Parser::_parse_extern() {
  auto keyword = _as<Token::Keyword>();

  // The Onyx lexer has just read whatever is following `extern`, which shall be
  // read by a C lexer instead. In order to achieve that, we have to "roll" the
  // Onyx lexer "back" a single token.
  _lexer->_unread();

  // Create a placement for the C block at the current Onyx lexer position.
  auto placement = Placement(_lexer->unit, _lexer->cursor());

  auto c_block = std::make_shared<C::Block>(C::Block(
      placement, _lexer->unit->source_stream(), _logger->dup("cblock")));

  // Need to skip source code lexed by the C lexer.
  auto offset = c_block->parse();
  _lexer->offset(offset);
  c_block->placement.location.end = _lexer->cursor();

  return CST::Extern(keyword, c_block);
}

std::optional<CST::Directive> Parser::_try_parse_directive() {
  if (_is(Token::Keyword::Extern))
    return std::make_shared<CST::Extern>(_parse_extern());
  else
    return std::nullopt;
}

CST::Var Parser::_parse_var(bool require_keyword) {
  std::optional<Token::Keyword> keyword;

  if ((_is(Token::Keyword::Let) || _is(Token::Keyword::Final))) {
    keyword = _consume<Token::Keyword>();
    _consume_space();
  } else if (require_keyword) {
    throw _expected("`final` or `let`");
  } else {
    // Ok, can live without a keyword.
  }

  auto id = _consume<Token::Id>();
  _skip_space();

  std::shared_ptr<CST::TypeExpr> type;
  if (_is_colon()) {
    _advance();       // Consume the colon
    _consume_space(); // Space is required after the colon
    type = std::make_shared<CST::TypeExpr>(_parse_type_expr());
  }

  std::optional<CST::RVal> rval;
  if (auto op = _if<Token::Op>()) {
    if (op->op == "=") {
      _advance(); // Consume `=`
      _skip_spaces_and_newlines();
      rval = _parse_rval();
    } else {
      throw Panic("Unexpected operator, expected assignment", op->placement);
    }
  }

  return CST::Var(keyword, id, type, rval);
}

CST::Func Parser::_parse_func() {
  // TODO: Different keywords: `def`, `decl`, `impl`, `reimpl`.
  auto keyword = _consume<Token::Keyword>();
  _consume_space();

  auto id = _as<Token::Id>();
  _advance();

  std::vector<std::shared_ptr<CST::Var>> args;
  _skip_space();
  if (_is_open_paren()) {
    _advance(); // Consume the open paren

    bool first = true;
    while (!_is_close_paren()) {
      if (first) {
        first = false;
      } else {
        _consume_comma();
        _skip_spaces_and_newlines();
      }

      args.push_back(std::make_shared<CST::Var>(_parse_var(false)));
      _skip_spaces_and_newlines();
    }

    _advance(); // Consume the closing paren
    _skip_space();
  }

  std::shared_ptr<CST::TypeExpr> return_type_expr;
  if (_is_colon()) {
    _advance(); // Consume the colon
    _skip_space();
    return_type_expr = std::make_shared<CST::TypeExpr>(_parse_type_expr());
  }

  std::shared_ptr<CST::Block> body;
  if (_is_newline() || _is_open_bracket())
    body = std::make_shared<CST::Block>(_parse_block());
  else
    throw _expected("function body");

  return CST::Func(CST::Func::Def, keyword, id, return_type_expr, args, body);
}

std::optional<CST::Decl> Parser::_try_parse_decl() {
  if (_is(Token::Keyword::Let) || _is(Token::Keyword::Final))
    return std::make_shared<CST::Var>(_parse_var(true));
  else if (_is(Token::Keyword::Def))
    return std::make_shared<CST::Func>(_parse_func());
  else
    return std::nullopt;
}

CST::Tuple Parser::_parse_tuple() {
  _logger->trace();

  _consume_open_paren();
  _skip_spaces_and_newlines();

  std::vector<CST::Element> elements;
  bool first = true;
  while (!_is_close_paren()) {
    if (first)
      first = false;
    else {
      _consume_comma();
      _skip_spaces_and_newlines();
    }

    elements.push_back(_parse_element());
    _logger->trace("Parsed element");
    _skip_spaces_and_newlines();
  }

  _advance(); // Consume the closing paren

  _logger->trace("Parsed tuple");
  return CST::Tuple(elements);
}

CST::Id::PathElement
Parser::_parse_id_path_element(CST::Id::PathElement::Access access) {
  auto raw_id = _consume<Token::Id>();

  std::map<unsigned, std::shared_ptr<CST::Id>> vargs;
  std::map<Token::Label, std::shared_ptr<CST::Id>> kwargs;

  if (_is_open_angle()) {
    _advance(); // Consume the open angle
    _skip_spaces_and_newlines();

    std::map<std::string, Token::Label> used_label_map;
    unsigned index = 0;

    while (!_is_close_angle()) {
      if (index > 0) {
        _consume_comma();
        _skip_spaces_and_newlines();
      }

      std::optional<std::variant<unsigned, Token::Label>> key;

      if (auto label = _if<Token::Label>()) {
        auto previous_label =
            Util::Map::get_if(used_label_map, label.value().value);

        if (previous_label)
          throw Panic(
              "Duplicate label",
              label->placement,
              {{"Previously used here", previous_label.value().placement}});

        kwargs[label.value()] = std::make_shared<CST::Id>(_parse_id());
      } else {
        vargs[index++] = std::make_shared<CST::Id>(_parse_id());
      }

      _skip_spaces_and_newlines();
    }

    _advance(); // Consume the close angle
  }

  return CST::Id::PathElement(access, raw_id, vargs, kwargs);
}

CST::Id Parser::_parse_id() {
  auto access = CST::Id::PathElement::Access::Undefined;

  if (_is(Token::Punct::AccessStatic)) {
    _advance();
    access = CST::Id::PathElement::Access::Static;
  } else if (_is(Token::Punct::AccessInstance)) {
    _advance();
    access = CST::Id::PathElement::Access::Instance;
  }

  if (!_is_id())
    throw _expected<Token::Id>();

  std::vector<CST::Id::PathElement> path;

  while (_is_id()) {
    path.push_back(_parse_id_path_element(access));

    if (_is(Token::Punct::AccessStatic)) {
      _advance(); // Consume the access token
      access = CST::Id::PathElement::Access::Static;
    } else if (_is(Token::Punct::AccessInstance)) {
      _advance(); // Consume the access token
      access = CST::Id::PathElement::Access::Instance;
    } else {
      break;
    }
  }

  return CST::Id(path);
}

std::optional<CST::Val> Parser::_try_parse_val() {
  _logger->trace();

  // A C string literal, e.g. `$"foo"`.
  if (auto c_str = _if<Token::CStringLiteral>()) {
    _advance(); // Consume the `$` token
    _logger->trace("Parsed CString");
    return std::make_shared<CST::CStringLiteral>(c_str.value());
  }

  // An Onyx identifier, e.g. `foo`.
  else if (_is_id()) {
    _logger->trace("Parsed Id");
    return std::make_shared<CST::Id>(_parse_id());
  }

  // A C identifier, e.g. `$char`.
  else if (auto cid = _if<Token::CId>()) {
    _logger->trace("Parsed CId");
    return std::make_shared<CST::CId>(cid.value());
  }

  // A tuple.
  else if (_is_open_paren()) {
    return std::make_shared<CST::Tuple>(_parse_tuple());
  }

  // An int literal.
  else if (_is<Token::IntLiteral>()) {
    _logger->trace("Parsed IntLiteral");
    return std::make_shared<CST::IntLiteral>(_consume<Token::IntLiteral>());
  }

  else {
    return std::nullopt;
  }
}

CST::UnOp Parser::_parse_unop() {
  _logger->trace();
  auto _operator = _consume<Token::Op>();
  auto operand = _parse_rval();
  _logger->trace("Parsed CST::UnOp");
  return CST::UnOp(_operator, operand);
}

CST::BinOp Parser::_parse_binop(CST::RVal lval) {
  _logger->trace();

  std::optional<std::variant<Token::Op, Token::Punct>> op;
  if (_is_op())
    op = _consume<Token::Op>();
  else if (_is(Token::Punct::OpenAngle) || _is(Token::Punct::CloseAngle))
    op = _consume<Token::Punct>();

  _skip_spaces_and_newlines();
  auto rval = _parse_rval();

  _logger->trace("Parsed CST::BinOp");
  return CST::BinOp(lval, op.value(), rval);
}

std::optional<CST::RVal> Parser::_try_parse_rval() {
  _logger->trace();
  CST::RVal rval;

  if (_is_op()) {
    rval = std::make_shared<CST::UnOp>(_parse_unop());
  } else if (auto val = _try_parse_val()) {
    if (_is_open_paren()) {
      auto args = _parse_tuple();
      _logger->trace("Returning CST::Call as CST::RVal");
      rval = std::make_shared<CST::Call>(val.value(), args);
    } else {
      rval = Util::Variant::upcast<CST::RVal>(val.value());
    }
  } else {
    return std::nullopt;
  }

  _skip_space();

  if (_is_op()) {
    auto binop = std::make_shared<CST::BinOp>(_parse_binop(rval));
    return CST::RVal(binop);
  } else {
    return rval;
  }
}

CST::RVal Parser::_parse_rval() {
  if (auto rval = _try_parse_rval())
    return rval.value();
  else
    throw _expected("rvalue");
}

std::optional<CST::ExplSafety> Parser::_try_parse_expl_safety() {
  // ```
  // unsafe! <rval>
  // unsafe! (do[;]|;) <body>; end
  // unsafe! [do][;] { <body> }
  // ```
  //
  // ```ebnf
  // (* Spaces are omitted for brevity. *)
  // safety = safety_kw, ["do"], [term], "{", {body}, "}";
  // safety = safety_kw, ("do", [term]) | term, {body}, term, "end";
  // safety = safety_kw, rval;
  // ```
  //

  if (!(_is(Token::Keyword::UnsafeBang) || _is(Token::Keyword::FragileBang) ||
        _is(Token::Keyword::ThreadsafeBang)))
    return std::nullopt;

  auto safety_keyword = _consume<Token::Keyword>();
  _skip_space();

  std::optional<CST::Branch> branch;

  if (_is(Token::Keyword::Do) || _is_term() || _is_open_bracket()) {
    auto block = std::make_shared<CST::Block>(_parse_block());
    branch.emplace(CST::Branch(std::nullopt, block));
  } else {
    branch.emplace(CST::Branch(std::nullopt, _parse_rval()));
  }

  return CST::ExplSafety(safety_keyword, branch.value());
}

CST::Case Parser::_parse_case() {
  // ```
  // case <rval> [then] [do] { <body> }
  // case <rval> [then] (do|;) <body>; end
  // case <rval> then <rval>
  // ```
  //
  // ```ebnf
  // (* Spaces are omitted for brevity. *)
  // case = "case", rval, ["then"], ["do"], "{", {body}, "}";
  // case = "case", rval, ["then"], ("do" | term), {body}, term, "end";
  // case = "case", rval, "then", rval;
  // ```
  //

  // Consume `if`, `elif` or `case`.
  auto keyword = _consume<Token::Keyword>();
  _skip_space();

  auto cond = _parse_rval();
  _skip_space();

  auto then = _if(Token::Keyword::Then);
  if (then) {
    _advance();
    _skip_space();
  }

  std::optional<CST::Branch> branch;
  if (_is(Token::Keyword::Do) || _is_term() || _is_open_bracket()) {
    auto block = _parse_block(
        std::nullopt, {Token::Keyword::Elif, Token::Keyword::Else});

    branch.emplace(CST::Branch(then, std::make_shared<CST::Block>(block)));
  } else if (then) {
    branch.emplace(CST::Branch(then, _parse_rval()));
  } else {
    throw _expected("`then <rvalue>` or block");
  }

  return CST::Case(keyword, cond, branch.value());
}

CST::Else Parser::_parse_else() {
  // ```
  // else [then] <rval>
  // else [then] (do|;) <body>; end
  // else [then] [do] { <body> }
  // ```
  //
  // ```ebnf
  // (* Spaces are omitted for brevity. *)
  // else = "else", ["then"], ["do"], "{", {body}, "}";
  // else = "else", ["then"], ("do" | term), {body}, term, "end";
  // else = "else", ["then"], rval;
  // ```
  //

  auto else_kw = _consume<Token::Keyword>();
  _skip_space();

  auto then = _if(Token::Keyword::Then);
  if (then) {
    _advance();
    _skip_space();
  }

  std::optional<CST::Branch> branch;
  if (_is(Token::Keyword::Do) || _is_term() || _is_open_bracket()) {
    auto block = _parse_block(
        std::nullopt, {Token::Keyword::Elif, Token::Keyword::Else});

    branch.emplace(CST::Branch(then, std::make_shared<CST::Block>(block)));
  } else {
    branch.emplace(CST::Branch(then, _parse_rval()));
  }

  return CST::Else(else_kw, branch.value());
}

std::optional<CST::If> Parser::_try_parse_if() {
  if (!_is(Token::Keyword::If))
    return std::nullopt;

  auto _if = _parse_case();

  std::vector<CST::Case> elifs;
  while (_is(Token::Keyword::Elif)) {
    elifs.push_back(_parse_case());
  }

  std::optional<CST::Else> _else;
  if (_is(Token::Keyword::Else)) {
    _else.emplace(_parse_else());
  }

  return CST::If(_if, elifs, _else);
}

std::optional<CST::While> Parser::_try_parse_while() {
  // NOTE: Terminator is required after `do` to treat it as a block.
  //
  // ```
  // while <cond> [do][;] { <body> }
  // while <cond> [do]; <body>; end
  // while <cond> do <expr>
  // ```
  //
  // ```ebnf
  // while = "while", rval, ["do"], [term], "{", {block_body}, "}";
  // while = "while", rval, ["do"], term, {block_body}, term, "end";
  // while = "while", rval, "do", expr;
  // ```
  //

  if (!_is(Token::Keyword::While))
    return std::nullopt;

  auto keyword = _consume<Token::Keyword>();
  _skip_space();

  auto cond = _parse_rval();
  _skip_space();

  auto do_kw = _if(Token::Keyword::Do);
  if (do_kw) {
    _advance();
    _skip_space();
  }

  std::optional<CST::Branch> branch;
  if (_is_term() || _is_open_bracket()) {
    // `do` would be a part of the block, e..g `while true do { stuff() }`.
    auto block = std::make_shared<CST::Block>(_parse_block(do_kw));

    branch.emplace(CST::Branch(std::nullopt, block));
  } else if (do_kw) {
    // `do` would be a part of the branch, e.g. `while true do stuff()`.
    branch.emplace(CST::Branch(do_kw, _parse_rval()));
  } else {
    throw _expected("`do <rvalue>` or block");
  }

  return CST::While(keyword, cond, branch.value());
}

std::optional<CST::BranchStatement> Parser::_try_parse_branch_statement() {
  if (auto _if = _try_parse_if())
    return CST::BranchStatement(std::make_shared<CST::If>(_if.value()));
  else if (auto _while = _try_parse_while())
    return CST::BranchStatement(std::make_shared<CST::While>(_while.value()));
  else
    return std::nullopt;
}

std::optional<CST::Return> Parser::_try_parse_return() {
  _logger->trace("");

  if (!_is(Token::Keyword::Return))
    return std::nullopt;

  auto keyword = _consume_return();
  _skip_space();

  if (_is_term() || _is_close_bracket() || _is_close_paren()) {
    _logger->trace("Parsed CST::Return w/o value");
    return CST::Return(keyword);
  } else {
    auto rval = _parse_rval();
    _logger->trace("Parsed CST::Return w/ value");
    return CST::Return(keyword, rval);
  }
}

std::optional<CST::ControlStatement> Parser::_try_parse_control_statement() {
  if (auto _return = _try_parse_return())
    return CST::ControlStatement(
        std::make_shared<CST::Return>(_return.value()));
  else
    return std::nullopt;
}

std::optional<CST::Statement> Parser::_try_parse_statement() {
  if (auto branch_stmnt = _try_parse_branch_statement())
    return Util::Variant::upcast<CST::Statement>(branch_stmnt.value());
  else if (auto control_stmnt = _try_parse_control_statement())
    return Util::Variant::upcast<CST::Statement>(control_stmnt.value());
  else
    return std::nullopt;
}

CST::Block Parser::_parse_block(
    std::optional<Token::Keyword> explicit_do,
    std::set<Token::Keyword::Kind> extra_terminators) {
  CST::Block::Style style;
  std::optional<bool> multiline;
  std::optional<std::variant<Token::Keyword, Token::Punct>> begin;

  if (!explicit_do) {
    if ((explicit_do = _if(Token::Keyword::Do))) {
      _advance();
      _skip_space();
    }
  }

  // `do\n<expr>`
  if (_is_newline()) {
    style = CST::Block::Style::Ruby;
    multiline = true;

    if (!begin)
      begin = _consume<Token::Punct>();

    _skip_spaces_and_newlines();
  }

  // `do; <expr>`
  else if (_is_semi()) {
    style = CST::Block::Style::Ruby;

    if (!begin)
      begin = _consume<Token::Punct>();

    _skip_space();

    if (_is_newline()) {
      multiline = true;
      _skip_spaces_and_newlines();
    }
  }

  // `do { <expr>`
  else if (_is_open_bracket()) {
    style = CST::Block::Style::C;

    if (!begin)
      begin = _consume<Token::Punct>();

    _skip_space();

    if (_is_newline()) {
      multiline = true;
      _skip_spaces_and_newlines();
    }
  }

  // `do <expr>`
  else if (explicit_do) {
    style = CST::Block::Style::Ruby;
    begin = explicit_do.value();
  }

  std::vector<CST::Block::Node> nodes;

  // We need to handle a newline terminator in a special way, because this would
  // be considered a multi-line block:
  //
  // ```
  // do foo;
  //   bar
  // end
  // ```
  //
  // Also, a closing bracket acts a terminator,
  // e.g. `{ expr() }`, but `do expr(); end`.
  //
  bool terminated = true;
  std::optional<Token::Punct> latest_terminator;
  //
  // For a C-style block, we expect a closing bracket. For a Ruby-style block,
  // we expect either an `end` keyword, or a keyword from the terminators list.
  //
  auto ruby_terminators = extra_terminators;
  ruby_terminators.insert(Token::Keyword::End);
  //
  // Handled edge-cases:
  //
  //   * [x] `do; end`
  //   * [x] `do\nend`
  //   * [x] `do { }`
  //   * [x] `do {\n}`
  //   * [x] `do { ; }`
  //
  while (!(style == CST::Block::Style::C && _is_close_bracket()) &&
         !(style == CST::Block::Style::Ruby && _is(ruby_terminators))) {
    if (!terminated) {
      if (_is_term()) {
        if (_is_newline())
          multiline = true;

        terminated = true;
        latest_terminator = _consume_term();
      } else {
        throw _expected("terminator");
      }
    }

    // A freestanding semicolon is legal, e.g. `do { ; }`.
    if (_is_semi()) {
      _advance();
      _logger->debug("Parsed freestanding semi");
      _skip_spaces_and_single_newline();
      continue;
    }

    // A freestanding empty line.
    if (_is_newline()) {
      auto node = std::make_shared<CST::EmptyLine>(_consume<Token::Punct>());
      _logger->debug("Parsed CST::EmptyLine");
      nodes.push_back(CST::Block::Node(node));
      _skip_spaces_and_newlines();
      continue;
    }

    // A block may contain any declaration.
    else if (auto decl = _try_parse_decl()) {
      std::visit([this](auto &node) { _debug(node.get()); }, decl.value());
      nodes.push_back(Util::Variant::upcast<CST::Block::Node>(decl.value()));
    }

    // A block may contain any statement.
    else if (auto stmnt = _try_parse_statement()) {
      std::visit([this](auto &node) { _debug(node.get()); }, stmnt.value());
      nodes.push_back(Util::Variant::upcast<CST::Block::Node>(stmnt.value()));
    }

    // A generic (sub-)block may begin with `do`.
    else if (auto block = _try_parse_block()) {
      auto ptr = std::make_shared<CST::Block>(block.value());
      _debug(ptr.get());
      nodes.push_back(CST::Block::Node(ptr));
    }

    // A block may contain an rval.
    else if (auto rval = _try_parse_rval()) {
      std::visit([this](auto &node) { _debug(node.get()); }, rval.value());
      nodes.push_back(Util::Variant::upcast<CST::Block::Node>(rval.value()));
    }

    else {
      throw _expected("declaration, rvalue, statement or block");
    }

    _skip_space();

    if (_is_newline()) {
      multiline = true;
      terminated = true; // A newline is a terminator
      latest_terminator = _consume_term();
      _skip_space();
    } else if (_is_semi()) {
      terminated = true; // A semicolon is a terminator
      latest_terminator = _consume_term();
      _skip_spaces_and_single_newline();
    } else {
      terminated = false;
      _skip_spaces_and_single_newline();
    }
  }

  if (!terminated) {
    switch (style) {
    case CST::Block::Style::C:
      break; // Closing bracket acts as a terminator
    case CST::Block::Style::Ruby:
      throw _expected("terminator");
    }
  }

  std::optional<std::variant<Token::Keyword, Token::Punct>> end;

  switch (style) {
  case CST::Block::Style::C: {
    end = _consume<Token::Punct>(); // The closing bracket is the end
    break;
  }
  case CST::Block::Style::Ruby: {
    if (_is(Token::Keyword::End))
      end = _consume<Token::Keyword>(); // The `end` keyword is the end
    else if (_is(extra_terminators))
      end = latest_terminator.value(); // The latest terminator is the end
    else
      assert(false && "BUG");

    break;
  }
  }

  return CST::Block(
      begin.value(), style, multiline.value(), nodes, end.value());
}

std::optional<CST::Block> Parser::_try_parse_block() {
  if (_is_open_bracket())
    return _parse_block();
  else if (_is(Token::Keyword::Do))
    return _parse_block();
  else
    return std::nullopt;
}

CST::Element Parser::_parse_element() {
  if (auto expl_safety = _try_parse_expl_safety())
    return CST::Element(std::make_shared<CST::ExplSafety>(expl_safety.value()));
  else if (auto branch_stmnt = _try_parse_branch_statement())
    return Util::Variant::upcast<CST::Element>(branch_stmnt.value());
  else if (auto block = _try_parse_block())
    return CST::Element(std::make_shared<CST::Block>(block.value()));
  else
    try {
      auto rval = _parse_rval();
      return Util::Variant::upcast<CST::Element>(rval);
    } catch (_ExpectedRVal) {
      throw _expected(
          "rvalue, explicit safety statement, branch statement or block");
    }
}

CST::TopLevelNode Parser::_parse_top_level_node() {
  if (auto directive = _try_parse_directive())
    return Util::Variant::upcast<CST::TopLevelNode>(directive.value());

  else if (auto decl = _try_parse_decl())
    return Util::Variant::upcast<CST::TopLevelNode>(decl.value());

  else if (auto rval = _try_parse_rval())
    return Util::Variant::upcast<CST::TopLevelNode>(rval.value());

  else
    throw _expected("directive, declaration or expression");
}

CST::TypeExpr Parser::_parse_type_expr() {
  if (_is_id()) {
    return CST::TypeExpr(std::make_shared<CST::Id>(_parse_id()));
  } else {
    throw _expected("type expression");
  }
}

void Parser::_skip_space() {
  while (!_lexer_done() && _is_space())
    _advance();
}

void Parser::_skip_spaces_and_newlines() {
  while (!_lexer_done() && (_is_space() || _is_newline()))
    _advance();
}

void Parser::_skip_spaces_and_single_newline() {
  bool parsed_newline = false;

  while (!_lexer_done()) {
    if (_is_space())
      _advance();
    else if (_is_newline()) {
      if (!parsed_newline) {
        _advance();
        parsed_newline = true;
      } else
        break;
    } else
      break;
  }
}

bool Parser::_is(Token::Punct::Kind kind) {
  if (auto punct = _if<Token::Punct>())
    return punct->kind == kind;
  else
    return false;
}

bool Parser::_is(Token::Keyword::Kind kind) {
  if (auto keyword = _if<Token::Keyword>())
    return keyword->kind == kind;
  else
    return false;
}

bool Parser::_is(std::set<Token::Keyword::Kind> kinds) {
  if (auto keyword = _if<Token::Keyword>())
    return kinds.contains(keyword->kind);
  else
    return false;
}

std::optional<Token::Keyword> Parser::_if(Token::Keyword::Kind kind) {
  if (auto keyword = _if<Token::Keyword>()) {
    if (keyword->kind == kind)
      return keyword;
  }

  return std::nullopt;
}

bool Parser::_is_op(std::string compared_op) {
  if (auto op = _if<Token::Op>()) {
    return op->op == compared_op;
  } else
    return false;
}

Token::Punct Parser::_consume(Token::Punct::Kind kind) {
  if (_is(kind)) {
    return _consume<Token::Punct>();
  } else {
    throw _expected(Token::Punct::kind_to_safe_str(kind));
  }
}

Token::Keyword Parser::_consume(Token::Keyword::Kind kind) {
  if (_is(kind)) {
    return _consume<Token::Keyword>();
  } else {
    throw _expected(Token::Keyword::kind_to_str(kind));
  }
}

} // namespace Fancysoft::NXC::Onyx
