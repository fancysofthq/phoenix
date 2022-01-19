#include <memory>
#include <optional>
#include <vector>

#include "fancysoft/nxc/c/lexer.hh"
#include "fancysoft/nxc/c/parser.hh"

namespace Fancysoft::NXC::C {

std::unique_ptr<CST> Parser::parse(bool single_expression) {
  _initialize();

  bool an_expression_parsed = false;
  auto ast = std::make_unique<CST>();

  while (!_lexer_done() && (!single_expression || !an_expression_parsed)) {
    if (_is_punct(Token::Punct::Space)) {
      _advance();
      continue;
    }

    else if (_is<Token::Id>()) {
      auto initial_type_ref = _parse_type_ref();

      if (_is_punct(Token::Punct::Space)) {
        // That's a function declaration.
        //

        _advance(); // Consume the space

        auto function_id_token = _as<Token::Id>();
        _advance(); // Consume the function id token

        _as_open_paren();
        _advance(); // Consue the opening parenthesis

        std::vector<std::shared_ptr<CST::FuncDecl::Arg>> args;

        while (_is<Token::Id>()) {
          args.push_back(_parse_func_decl_arg());

          if (_is_comma()) {
            _advance();
            continue;
          } else if (_is_close_paren()) {
            _advance();
            break;
          } else {
            throw _expected("`,` or `)`");
          }
        }

        _as_semi();

        auto node = std::make_shared<CST::FuncDecl>(
            initial_type_ref, function_id_token, args);

        _debug(node.get());
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

  auto &log = _logger->sdebug();
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

std::shared_ptr<CST::TypeRef> Parser::_parse_type_ref() {
  auto id = _as<Token::Id>();
  _advance();

  std::vector<Token::Op> pointer_tokens;
  while (_is_op("*")) {
    pointer_tokens.push_back(_as<Token::Op>());
    _advance();
  }

  auto node = std::make_shared<CST::TypeRef>(id, pointer_tokens);
  return node;
}

std::shared_ptr<CST::FuncDecl::Arg> Parser::_parse_func_decl_arg() {
  auto type_ref = _parse_type_ref();

  std::optional<Token::Id> id;
  if (_is_space()) {
    _advance();

    id = _if<Token::Id>();
    if (id)
      _advance();
  }

  auto node = std::make_shared<CST::FuncDecl::Arg>(type_ref, id);
  return node;
}

} // namespace Fancysoft::NXC::C
