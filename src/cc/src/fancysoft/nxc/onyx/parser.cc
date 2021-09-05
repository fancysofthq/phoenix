#include <memory>
#include <optional>
#include <vector>

#include "fancysoft/nxc/c/block.hh"
#include "fancysoft/nxc/onyx/lexer.hh"
#include "fancysoft/nxc/onyx/parser.hh"

namespace Fancysoft::NXC::Onyx {

void Parser::parse(std::shared_ptr<CST::Root> root) {
  _initialize();

  while (!_lexer_done()) {
    if (_is<Token::Punct::Space, Token::Punct::EOL>()) {
      _advance();
      continue;
    }

    // An `extern` statement.
    else if (auto token = _if<Token::Keyword::Extern>()) {
      _lexer->_unread(); // HACK: Unread whatever followed `extern`

      auto placement = Placement(_lexer->unit, _lexer->cursor());
      auto c_block = std::make_shared<C::Block>(
          C::Block(placement, _lexer->unit->source_stream()));

      // Need to offset the lexer, because a C block is a part
      // of the source file being lexed.
      //
      auto offset = c_block->parse(c_block);
      _lexer->offset(offset);
      c_block->placement.location.end = _lexer->cursor();

      auto node = CST::Node::Extern(token.value(), c_block);
      fmt::print(
          Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

      auto node_ptr = std::make_shared<CST::RootChildNode>(node);
      root.get()->children.push_back(node_ptr);

      _advance();
      continue;
    }

    // A variable declaration or definition.
    else if (auto keyword = _if<Token::Keyword::Let, Token::Keyword::Final>()) {
      _expect_next<Token::Punct::Space>();

      auto id = _expect_next<Token::Id>();
      _advance();

      std::shared_ptr<CST::Node::Expression> value_expr;

      // Space is optional around the `=` symbol, e.g. `foo=42`.
      //

      if (_is<Token::Punct::Space>()) {
        _advance();
      }

      if (_is<Token::BuiltInOp::Assign>()) {
        _advance();
        value_expr = _parse_expression(false);
      } else {
        throw _expected("assignment");
      }

      auto node = CST::Node::VarDecl(keyword.value(), id, value_expr);
      fmt::print(
          Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

      auto node_ptr = std::make_shared<CST::RootChildNode>(node);
      root.get()->children.push_back(node_ptr);

      continue;
    }

    // An explicit safety region statement beginning with an explicit
    // safety keyword, e.g. `unsafe!`.
    else if (auto keyword = _if<Token::Keyword::UnsafeBang>()) {
      _expect_next<Token::Punct::Space>();

      auto expr_node = _parse_expression(false);
      std::vector<std::shared_ptr<CST::Node::Expression>> expressions = {
          expr_node};

      auto node = CST::Node::ExplicitSafety(
          keyword.value(), CST::Node::ExplicitSafety::Inline, expressions);

      fmt::print(
          Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

      auto node_ptr = std::make_shared<CST::RootChildNode>(node);
      root.get()->children.push_back(node_ptr);

      _advance();
      continue;
    }

    else {
      throw _unexpected();
    }
  }

  Util::logger.debug(_debug_name()) << "Done parsing\n";
}

std::shared_ptr<CST::Node::Expression>
Parser::_parse_expression(bool allow_empty) {
  while (!_lexer_done()) {
    if (_is<Token::Punct::Space>()) {
      _advance();
      continue;
    }

    // A string literal, e.g. `"foo"`.
    else if (auto literal = _if<Token::StringLiteral>()) {
      _advance(); // Consume the literal token
      auto node = CST::Node::StringLiteral(literal.value());

      fmt::print(
          Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

      return std::make_shared<CST::Node::Expression>(node);
    }

    // An expression beginning with a C identifier.
    else if (auto cid = _if<Token::CId>()) {
      _advance(); // Consume the C identifier

      if (_is<Token::Punct::OpenParen>()) {
        _advance();

        // This is a C function call.
        //

        std::vector<std::shared_ptr<CST::Node::Expression>> args;

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
            auto arg = _parse_expression(true);

            if (arg) {
              args.push_back(arg);
              continue;
            } else {
              break; // Empty parentheses, i.e. zero arity
            }
          } else {
            if (_is<Token::Punct::Comma>()) {
              _advance(); // Consume the comma

              auto arg = _parse_expression(false);
              args.push_back(arg);
              continue;
            } else if (_is<Token::Punct::CloseParen>()) {
              _advance(); // Consume the paren
              break;      // Done parsing arguments
            } else {
              throw _expected("`,`, `)`");
            }
          }
        }

        auto node = CST::Node::CCall(cid.value(), args);
        fmt::print(
            Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

        return std::make_shared<CST::Node::Expression>(node);
      } else {
        throw _expected("call");
      }
    }

    else if (auto _operator = _if<Token::BuiltInOp::AddressOf, Token::Op>()) {
      _advance(); // Consume the operator token
      auto operand = _parse_expression(false);
      auto node = CST::Node::UnOp(_operator.value(), operand);
      fmt::print(
          Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());
      return std::make_shared<CST::Node::Expression>(node);
    }

    else if (auto id = _if<Token::Id>()) {
      _advance();
      auto node = CST::Node::Id(id.value());
      fmt::print(
          Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());
      return std::make_shared<CST::Node::Expression>(node);
    }

    else {
      if (allow_empty)
        return nullptr;
      else
        throw _expected("expression");
    }
  }

  throw _unexpected_eof();
}

} // namespace Fancysoft::NXC::Onyx
