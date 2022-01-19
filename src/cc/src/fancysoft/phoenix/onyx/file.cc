#include <memory>

#include "fancysoft/phoenix/onyx/ast.hh"
#include "fancysoft/phoenix/onyx/file.hh"
#include "fancysoft/phoenix/util/logger.hh"

namespace Fancysoft::Phoenix::Onyx {

void File::compile(std::shared_ptr<Util::Logger> logger) {
  if (!parsed())
    parse();

  if (!_mlir)
    _mlir = std::make_unique<MLIR>(MLIR());

  if (!_c_ast)
    _c_ast = std::make_unique<C::AST>(C::AST());

  if (!_c_mlir)
    _c_mlir = std::make_unique<C::MLIR>(C::MLIR());

  _ast = std::make_shared<AST>(
      AST(_program,
          path,
          _mlir.get(),
          _c_ast.get(),
          _c_mlir.get(),
          logger->fork("ast")));
}

} // namespace Fancysoft::Phoenix::Onyx
