#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "./ast.hh"

namespace Fancysoft {
namespace Phoenix {
namespace Onyx {

/// An Onyx Middle Level Intermediate Representation module containing function and
/// type specializations. A specialization resides in an MLIR module linked to a source
/// file containing the implementation.
///
/// MLIR is expected to be "fat".
///
///   For example, `Int32::Int32(42)` as a call is preferrable over some literal node to
///   reduce MLIR complexity; upon lowering, it's to be inlined anyway, e.g. to `i32 42`.
///
///   Primitive types are structs in MLIR, e.g. `struct Int32 { this : Int32 }`. In LLIR,
///   it'd be simplified to just `i32`.
///
struct MLIR {
  struct Function;

  /// An MLIR struct specialization with defined bitsize.
  struct Struct {
    struct Field {
      /// `std::nullopt` means recursive (parent) type.
      std::optional<std::weak_ptr<Struct>> type;

      std::weak_ptr<AST::VarDef> ast_def;

      bool is_static() const;
      std::string id() const;
    };

    /// The matching AST struct definition.
    /// TODO: Record, i.e. an anonymous struct.
    std::optional<std::weak_ptr<AST::StructProtoSpec>> ast_def;

    std::unordered_map<std::string, Field> instance_fields;
    std::unordered_map<std::string, Field> static_fields;
    std::vector<std::shared_ptr<Function>> instance_methods;
    std::vector<std::shared_ptr<Function>> static_methods;
  };

  struct Ref;
  struct Call;
  struct Assignment;

  using Expr = std::
      variant<std::shared_ptr<Ref>, std::shared_ptr<Call>, std::shared_ptr<Assignment>>;

  struct RVal {};

  struct Call {
    /// The function specialization being called.
    std::shared_ptr<Function> callee;

    /// Arguments for this call. Note that an object access is converted to UFCS.
    std::vector<Expr> args;
  };

  struct Assignment {
    /// `a` in `a = b`.
    std::shared_ptr<Ref> assignee;

    /// `b` in `a = b`.
    std::shared_ptr<Expr> value;
  };

  /// An MLIR function specialization contains executable code.
  struct Function {
    struct Arg {
      std::string id;
      std::vector<std::shared_ptr<Struct>> type;
      std::optional<Expr> default_value;
    };

    /// The matching AST function proto-specialization.
    /// Multiple protospecs may reference the same specialization, for example:
    ///
    /// ```nx
    /// impl foo<T ~ Real>() { }
    ///
    /// foo<Int32>()   # Protospec #1, Spec #1
    /// foo<Float64>() # Protospec #2, Spec #2
    /// foo<Int32>()   # Protospec #3, Spec #1
    /// ```
    std::weak_ptr<AST::ProtoFuncSpec> ast_proto;

    std::vector<Arg> args;
    std::vector<Expr> body;
  };
};

} // namespace Onyx
} // namespace Phoenix
} // namespace Fancysoft
