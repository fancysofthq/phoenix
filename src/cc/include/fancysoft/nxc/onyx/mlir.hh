#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <fancysoft/util/variant.hh>

#include "../c/mlir.hh"
#include "./lang.hh"
#include "./mlir/builtin.hh"

namespace Fancysoft {
namespace NXC {
namespace Onyx {
namespace MLIR {

struct IntLiteral {
  const std::int64_t value;
};

struct StringLiteral {
  /// The null byte is NOT implicitly appended to the value upon lowering.
  std::string value;
};

using Literal = std::variant<IntLiteral, StringLiteral>;

using Type =
    Util::Variant::Flatten<std::variant<Builtin::Type::Any, C::MLIR::Type>>;

struct Call;
struct Cast;
struct VarRef;

using RVal = std::variant<
    std::unique_ptr<Call>,
    std::unique_ptr<Cast>,
    std::unique_ptr<Literal>,
    std::unique_ptr<VarRef>>;

struct If;
struct Return;

using FlowStatement =
    std::variant<std::unique_ptr<If>, std::unique_ptr<Return>>;

struct Assignment;
struct VarDecl;
struct Function;

struct Block {
  using Expr = Util::Variant::Flatten<std::variant<
      std::shared_ptr<VarDecl>,
      std::unique_ptr<Assignment>,
      std::unique_ptr<Cast>,
      std::unique_ptr<Call>,
      FlowStatement>>;

  std::vector<Expr> exprs;
};

struct Case {
  RVal cond;
  Block branch;
};

struct If {
  Case main_branch;
  std::vector<Case> elif_branches;
  std::optional<Case> else_branch;
};

struct Return {
  std::optional<RVal> value;
};

struct VarDecl {
  Type type;
  std::string name;
  std::optional<RVal> value;
};

struct VarRef {
  std::shared_ptr<VarDecl> decl;
};

struct Assignment {
  VarRef lval;
  RVal rval;
};

struct Call {
  std::variant<
      Builtin::Function,
      std::shared_ptr<C::MLIR::Function>,
      std::shared_ptr<Function>>
      callee;

  std::vector<RVal> args;
};

struct Cast {
  RVal source;
  Type target_type;
};

struct Function {
  Lang::Id id;
  std::vector<std::shared_ptr<VarDecl>> args;
  Block body;
};

} // namespace MLIR
} // namespace Onyx
} // namespace NXC
} // namespace Fancysoft
