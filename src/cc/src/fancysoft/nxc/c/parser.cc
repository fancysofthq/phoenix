#include <memory>
#include <optional>
#include <vector>

#include "fancysoft/nxc/c/lexer.hh"
#include "fancysoft/nxc/c/parser.hh"

namespace Fancysoft::NXC::C {

void Parser::parse(std::shared_ptr<CST::Root> root, bool single_expression) {
  _initialize();
  bool an_expression_parsed = false;

  while (!_lexer_done() && (!single_expression || !an_expression_parsed)) {
    if (_is<Token::Punct::Space>()) {
      _advance();
      continue;
    }

    else if (_is<Token::Id>()) {
      auto initial_type_node_ptr = _parse_type();

      if (_is<Token::Punct::Space>()) {
        // That's a function declaration.
        //

        _advance(); // Consume the space
        auto function_id_token = _as<Token::Id>();

        _expect_next<Token::Punct::OpenParen>();
        _advance();

        std::vector<std::shared_ptr<CST::Node::Proto::ArgDecl>> args;

        while (_is<Token::Id>()) {
          args.push_back(_parse_arg_decl());

          if (_is<Token::Punct::Comma>()) {
            _advance();
            continue;
          } else if (_is<Token::Punct::CloseParen>()) {
            _advance();
            break;
          } else {
            throw _unexpected();
          }
        }

        _as<Token::Punct::Semi>();

        auto node =
            CST::Node::Proto(initial_type_node_ptr, function_id_token, args);
        auto node_ptr = std::make_shared<CST::RootChildNode>(node);

        fmt::print(
            Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

        root->children.push_back(node_ptr);

        if (!single_expression) {
          _advance();
        }

        an_expression_parsed = true;
        continue;
      } else {
        throw _unexpected();
      }
    } else {
      throw _unexpected();
    }
  }

  auto &log = Util::logger.debug(_debug_name());
  log << "Done parsing due to ";

  if (_lexer_done()) {
    log << "lexer depletion";
  } else if (an_expression_parsed) {
    log << "single expression parsed";
  } else {
    throw "Unreacheable";
  }

  log << std::endl;
}

std::shared_ptr<CST::Node::Type> Parser::_parse_type() {
  auto id = _as<Token::Id>();
  _advance();

  unsigned short pointer_depth = 0;
  while (_is<Token::Op::Asterisk>()) {
    pointer_depth += 1;
    _advance();
  }

  auto node = CST::Node::Type(id, pointer_depth);

  fmt::print(
      Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

  return std::make_shared<CST::Node::Type>(node);
}

std::shared_ptr<CST::Node::Proto::ArgDecl> Parser::_parse_arg_decl() {
  auto type = _parse_type();

  std::optional<Token::Id> id;
  if (_is<Token::Punct::Space>()) {
    _advance();

    id = _if<Token::Id>();
    if (id)
      _advance();
  }

  auto node = CST::Node::Proto::ArgDecl(type, id);

  fmt::print(
      Util::logger.debug(_debug_name()), "Parsed {}\n", node.node_name());

  return std::make_shared<CST::Node::Proto::ArgDecl>(node);
}

} // namespace Fancysoft::NXC::C
