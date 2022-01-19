#include <memory>
#include <sstream>

#include <fancysoft/nxc/onyx/parser.hh>
#include <fancysoft/nxc/unit.hh>

namespace Fancysoft::NXC::Onyx {

/// A scratch unit for testing purposes.
struct Scratch : Unit, std::enable_shared_from_this<Scratch> {
  std::stringstream stream;

  std::istream &source_stream() override { return stream; }

  Position parse() override {
    auto logger = std::make_shared<Logger>(Logger::Verbosity::Trace, std::cerr);

    logger->enable_thread_id_output = false;
    logger->enable_time_output = false;

    auto lexer = std::make_shared<Lexer>(shared_from_this(), ({
                                           auto log = logger->dup("lexer");
                                           log->verbosity =
                                               Logger::Verbosity::Debug;
                                           log;
                                         }));

    Parser parser(lexer, logger->dup("parser"));
    _ast = move(parser.parse());
    return lexer->cursor();
  }

  AST *ast() const { return _ast.get(); }

private:
  std::unique_ptr<AST> _ast;
};

} // namespace Fancysoft::NXC::Onyx
