#include <memory>
#include <optional>
#include <vector>

#include "fancysoft/nxc/c/block.hh"
#include "fancysoft/nxc/onyx/lexer.hh"
#include "fancysoft/nxc/onyx/parser.hh"

namespace Fancysoft::NXC::Onyx {

std::unique_ptr<AST> Parser::parse() {
  _initialize();
  auto ast = std::make_unique<AST>();

  while (!_lexer_done()) {
    if (_is_punct({Token::Punct::HSpace, Token::Punct::Newline})) {
      _advance();
      continue;
    }

    // An `extern` statement.
    else if (auto token = _if_keyword(Token::Keyword::Extern)) {
      _lexer->_unread(); // HACK: Unread whatever followed `extern`

      auto placement = Placement(_lexer->unit, _lexer->cursor());
      auto c_block = std::make_shared<C::Block>(
          C::Block(placement, _lexer->unit->source_stream()));

      // Need to offset the lexer, because a C block is a part
      // of the source file being lexed.
      //
      auto offset = c_block->parse();
      _lexer->offset(offset);
      c_block->placement.location.end = _lexer->cursor();

      auto node =
          std::make_shared<AST::ExternDirective>(token.value(), c_block);

      _debug_parsed(node->node_name());
      ast->add_child(AST::TopLevelNode(node));

      _advance();
      continue;
    }

    // A variable definition.
    else if (
        auto keyword =
            _if_keyword({Token::Keyword::Let, Token::Keyword::Final})) {
      _advance();
      _as_punct(Token::Punct::HSpace);

      auto id = _next_as<Token::Id>();

      _advance();
      _skip_space();

      if (auto op = _if<Token::Op>()) {
        if (op->op == "=") {
          _advance(); // Consume `=`
          _skip_space();

          auto rval = _parse_rval();

          auto node = std::make_shared<AST::VarDecl>(
              keyword.value(), id, nullptr, rval);

          _debug_parsed(node->node_name());
          ast->add_child(node);

          continue;
        } else {
          throw Panic("Unexpected operator", op->placement);
        }
      } else {
        auto node = std::make_shared<AST::VarDecl>(keyword.value(), id);
        _debug_parsed(node->node_name());
        ast->add_child(node);
        continue;
      }
    }

    // An explicit safety region statement beginning with an explicit
    // safety keyword, e.g. `unsafe!`.
    else if (
        auto keyword = _if_keyword(
            {Token::Keyword::UnsafeBang, Token::Keyword::FragileBang})) {
      _advance();
      _as_punct(Token::Punct::HSpace);

      auto rval = _parse_rval();
      auto node =
          std::make_shared<AST::ExplicitSafetyStatement>(keyword.value(), rval);

      _debug_parsed(node->node_name());
      ast->add_child(node);

      _advance();
      continue;
    }

    else {
      throw _unexpected();
    }
  }

  Util::logger.debug(_debug_name()) << "Done parsing\n";

  return ast;
}

AST::RVal Parser::_parse_rval() {
  while (!_lexer_done()) {
    // Spaces can be skipped.
    if (_is_space()) {
      _advance();
      continue;
    }

    // // A string literal, e.g. `"foo"`.
    // else if (auto literal = _if<Token::StringLiteral>()) {
    //   _advance(); // Consume the literal token
    //   auto node = std::make_shared<AST::StringLiteral>(literal.value());
    //   _debug_parsed(node->node_name());
    //   return node;
    // }

    // A C string literal, e.g. `$"foo"`.
    else if (auto literal = _if<Token::CStringLiteral>()) {
      _advance(); // Consume the literal token
      auto node = std::make_shared<AST::CStringLiteral>(literal.value());
      _debug_parsed(node->node_name());
      return node;
    }

    // An Onyx identifier.
    else if (auto id = _if<Token::Id>()) {
      _advance(); // Consume the identifier
      auto node = std::make_shared<AST::Id>(id.value());
      _debug_parsed(node->node_name());
      return node;
    }

    // Otherwise, parse an expression.
    else {
      auto expr = _parse_expr();
      return Util::Variant::upcast(expr);
    }
  }

  throw _unexpected_eof();
}

AST::Expr Parser::_parse_expr() {
  // An expression beginning with a C identifier.
  if (auto cid = _if<Token::CId>()) {
    _advance(); // Consume the C identifier

    if (_is_open_paren()) {
      _advance(); // Consume the open paren

      // This is a C function call.
      //

      std::vector<AST::RVal> args;

      // Handled cases:
      //
      // * [x] Empty parentheses
      // * [x] Single argument
      // * [x] Multiple comma-separated arguments
      //
      // TODO: Handle spaces.
      //

      bool first = true;
      while (true) {
        if (first) {
          first = false;
          _skip_space_newline();

          if (_is_close_paren())
            break; // Empty parentheses, i.e. zero arity

          args.push_back(_parse_rval());
          continue;
        } else {
          if (_is_comma()) {
            _advance(); // Consume the comma
            _skip_space_newline();

            args.push_back(_parse_rval());
            continue;
          } else if (_is_close_paren()) {
            _advance(); // Consume the closing parenthesis
            break;      // Done parsing arguments
          } else {
            throw _unexpected("comma or closing parenthesis");
          }
        }
      }

      auto node = std::make_shared<AST::CCall>(cid.value(), args);
      _debug_parsed(node->node_name());

      return node;
    } else {
      throw _unexpected("call");
    }
  }

  // Unary operation.
  else if (auto _operator = _if<Token::Op>()) {
    _advance(); // Consume the operator token
    auto operand = _parse_rval();
    auto node = std::make_shared<AST::UnOp>(_operator.value(), operand);
    _debug_parsed(node->node_name());
    return node;
  }

  else {
    throw _unexpected("expression");
  }
}

void Parser::_skip_space() {
  while (!_lexer_done() && _is_space())
    _advance();
}

void Parser::_skip_space_newline() {
  while (!_lexer_done() && (_is_space() || _is_newline()))
    _advance();
}

bool Parser::_is_punct(Token::Punct::Kind kind) {
  if (auto punct = _if<Token::Punct>())
    return punct->kind == kind;
  else
    return false;
}

bool Parser::_is_punct(std::set<Token::Punct::Kind> kinds) {
  if (auto punct = _if<Token::Punct>())
    return kinds.contains(punct->kind);
  else
    return false;
}

Token::Punct Parser::_as_punct(Token::Punct::Kind kind) {
  if (auto punct = _if<Token::Punct>()) {
    if (punct->kind == kind)
      return punct.value();
  }

  throw _unexpected(Token::Punct::kind_to_expected(kind));
}

bool Parser::_is_keyword(Token::Keyword::Kind kind) {
  if (auto keyword = _if<Token::Keyword>())
    return keyword->kind == kind;
  else
    return false;
}

bool Parser::_is_keyword(std::set<Token::Keyword::Kind> kinds) {
  if (auto keyword = _if<Token::Keyword>())
    return kinds.contains(keyword->kind);
  else
    return false;
}

std::optional<Token::Keyword> Parser::_if_keyword(Token::Keyword::Kind kind) {
  if (auto keyword = _if<Token::Keyword>()) {
    if (keyword->kind == kind)
      return keyword;
  }

  return std::nullopt;
}

std::optional<Token::Keyword>
Parser::_if_keyword(std::set<Token::Keyword::Kind> kinds) {
  if (auto keyword = _if<Token::Keyword>()) {
    if (kinds.contains(keyword->kind))
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

} // namespace Fancysoft::NXC::Onyx
