#include <memory>
#include <optional>
#include <vector>

#include "fancysoft/nxc/c/lexer.hh"
#include "fancysoft/nxc/c/parser.hh"

namespace Fancysoft::NXC::C {

std::unique_ptr<AST> Parser::parse(bool single_expression) {
  _initialize();

  bool an_expression_parsed = false;
  auto ast = std::make_unique<AST>();

  while (!_lexer_done() && (!single_expression || !an_expression_parsed)) {
    if (_is_punct(Token::Punct::HSpace)) {
      _advance();
      continue;
    }

    else if (_is<Token::Id>()) {
      auto initial_type_ref = _parse_type_ref();

      if (_is_punct(Token::Punct::HSpace)) {
        // That's a function declaration.
        //

        _advance(); // Consume the space

        auto function_id_token = _as<Token::Id>();
        _advance(); // Consume the function id token

        _as_open_paren();
        _advance(); // Consue the opening parenthesis

        std::vector<std::shared_ptr<AST::FuncDecl::ArgDecl>> args;

        while (_is<Token::Id>()) {
          args.push_back(_parse_arg_decl());

          if (_is_comma()) {
            _advance();
            continue;
          } else if (_is_close_paren()) {
            _advance();
            break;
          } else {
            throw _unexpected("comma or closing parenthesis");
          }
        }

        _as_semi();

        auto node = std::make_shared<AST::FuncDecl>(
            initial_type_ref, function_id_token, args);

        _debug_parsed(node->node_name());
        ast->add_child(node);

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

  return ast;
}

std::shared_ptr<AST::TypeRef> Parser::_parse_type_ref() {
  auto id = _as<Token::Id>();
  _advance();

  std::vector<Token::Op> pointer_tokens;
  while (_is_op("*")) {
    pointer_tokens.push_back(_as<Token::Op>());
    _advance();
  }

  auto node = std::make_shared<AST::TypeRef>(id, pointer_tokens);
  _debug_parsed(node->node_name());

  return node;
}

std::shared_ptr<AST::FuncDecl::ArgDecl> Parser::_parse_arg_decl() {
  auto type_ref = _parse_type_ref();

  std::optional<Token::Id> id;
  if (_is_space()) {
    _advance();

    id = _if<Token::Id>();
    if (id)
      _advance();
  }

  auto node = std::make_shared<AST::FuncDecl::ArgDecl>(type_ref, id);
  _debug_parsed(node->node_name());

  return node;
}

} // namespace Fancysoft::NXC::C
