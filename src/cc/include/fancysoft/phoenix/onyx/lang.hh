#pragma once

#include "fancysoft/phoenix/onyx/lang.hh"
#include <optional>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Fancysoft {
namespace Phoenix {
namespace Onyx {

/// The Onyx language features.
namespace Lang {

enum class Safety { Unsafe, Fragile, Threadsafe };

static const std::string safety_string(Safety safety, bool bang = false) {
  switch (safety) {
  case Safety::Unsafe:
    return bang ? "unsafe!" : "unsafe";
  case Safety::Fragile:
    return bang ? "fragile!" : "fragile";
  case Safety::Threadsafe:
    return bang ? "threadsafe!" : "threadsafe";
  }
}

enum class PointerStorage { Undefined, Local, Instance, Static };

#pragma region Entity

enum class EntityCategory {
  Namespace,
  Variable,
  TemplateArgument,
  Function,
  Type,
  Expression,
  Specialization,
  IDLiteral,
};

inline const char *entity_category_string(EntityCategory category) {
  switch (category) {
  case EntityCategory::Namespace:
    return "namespace";
  case EntityCategory::Variable:
    return "variable";
  case EntityCategory::TemplateArgument:
    return "template argument";
  case EntityCategory::Function:
    return "function";
  case EntityCategory::Type:
    return "type";
  case EntityCategory::Expression:
    return "expression";
  case EntityCategory::Specialization:
    return "reference";
  }
}

enum class TypeCategory {
  Trait,
  Struct,
  // Class,
  // Unit,
  // Enum,
  // Annotation,
};

inline const char *type_category_string(TypeCategory category) {
  switch (category) {
  case TypeCategory::Trait:
    return "trait";
  case TypeCategory::Struct:
    return "struct";
    // case TypeCategory::Class:
    //   return "class";
    // case TypeCategory::Unit:
    //   return "unit";
    // case TypeCategory::Enum:
    //   return "enum";
    // case TypeCategory::Annotation:
    //   return "annotation";
  }
}

enum class ExpressionCategory {
  UnOp,
  BinOp,
  // Call
};

inline const char *type_category_string(ExpressionCategory category) {
  switch (category) {
  case ExpressionCategory::UnOp:
    return "unary operation";
  case ExpressionCategory::BinOp:
    return "binary operation";
    // case TypeCategory::Class:
    //   return "class";
    // case TypeCategory::Unit:
    //   return "unit";
    // case TypeCategory::Enum:
    //   return "enum";
    // case TypeCategory::Annotation:
    //   return "annotation";
  }
}

#pragma endregion

#pragma region Ion

enum class IonKind {
  Declaration,
  Implementation,
  Definition,
  Extension,
};

inline const char *ion_kind_to_string(IonKind kind) {
  switch (kind) {
  case IonKind::Declaration:
    return "declaration";
  case IonKind::Implementation:
    return "implementation";
  case IonKind::Definition:
    return "definition";
  case IonKind::Extension:
    return "extension";
  }
}

enum class FunctionIonKind { Declaration, Implementation, Definition };

inline const char *function_ion_kind_to_string(FunctionIonKind kind) {
  switch (kind) {
  case FunctionIonKind::Declaration:
    return ion_kind_to_string(IonKind::Declaration);
  case FunctionIonKind::Implementation:
    return ion_kind_to_string(IonKind::Implementation);
  case FunctionIonKind::Definition:
    return ion_kind_to_string(IonKind::Definition);
  }
}

enum class TypeIonKind { Definition, Extension };

inline const char *type_ion_kind_to_string(TypeIonKind kind) {
  switch (kind) {
  case TypeIonKind::Definition:
    return ion_kind_to_string(IonKind::Definition);
  case TypeIonKind::Extension:
    return ion_kind_to_string(IonKind::Extension);
  }
}

/// A well-known unary operator.
enum class WellKnownUnOp {
  LogicNot,    ///< `!x`.
  BitwiseNot,  ///< `~x`.
  Dereference, ///< `*x`.
  Addressof,   ///< `&x`.
};

/// Parse a well-known unary operator, return `std::nullopt` on failure.
inline std::optional<WellKnownUnOp> parse_well_known_unop(const char *string) {
  if (!strcmp(string, "!"))
    return WellKnownUnOp::LogicNot;

  else if (!strcmp(string, "~"))
    return WellKnownUnOp::BitwiseNot;

  else if (!strcmp(string, "*"))
    return WellKnownUnOp::Dereference;

  else if (!strcmp(string, "&"))
    return WellKnownUnOp::Addressof;

  else
    return std::nullopt;
}

enum class TypeUnOp {
  Not,        ///< `!T`.
  Virtualize, ///< `~T`.
  Splat,      ///< `*T`.
};

inline std::optional<TypeUnOp> well_known_to_type_unop(WellKnownUnOp well_known) {
  switch (well_known) {
  case (WellKnownUnOp::LogicNot):
    return TypeUnOp::Not;
  case (WellKnownUnOp::BitwiseNot):
    return TypeUnOp::Virtualize;
  case (WellKnownUnOp::Dereference):
    return TypeUnOp::Splat;
  default:
    return std::nullopt;
  }
}

/// A well-known binary operator.
enum class WellKnownBinOp {
  LogicAnd,   ///< `x && y`.
  LogicOr,    ///< `x || y`.
  BitwiseAnd, ///< `x & y`.
  BitwiseOr,  ///< `x | y`.

  LessThan,         ///< `x < y`.
  GreaterThan,      ///< `x > y`.
  LessOrEqualTo,    ///< `x <= y`.
  GreaterOrEqualTo, ///< `x >= y`.
  Spaceship,        ///< `x <=> y`.

  RealTypeComparison, ///< `x : y`.
  Virtualization,     ///< `x~y`.
  Assignment,         ///< `x = y`.

  // TIP: "Equal" and "equivalent" are equivalent, but they're not equal.
  Equivalence,    ///< `x == y`.
  NonEquivalence, ///< `x != y`.
  Equal,          ///< `x === y`.
  NonEqual,       ///< `x !== y`.
};

/// Parse a well-known binary operator, return `std::nullopt` on failure.
inline std::optional<WellKnownBinOp> parse_well_known_binop(const char *string) {
  if (!strcmp(string, "&&"))
    return WellKnownBinOp::LogicAnd;
  else if (!strcmp(string, "||"))
    return WellKnownBinOp::LogicOr;
  else if (!strcmp(string, "&"))
    return WellKnownBinOp::BitwiseAnd;
  else if (!strcmp(string, "|"))
    return WellKnownBinOp::BitwiseOr;

  else if (!strcmp(string, "<"))
    return WellKnownBinOp::LessThan;
  else if (!strcmp(string, ">"))
    return WellKnownBinOp::GreaterThan;
  else if (!strcmp(string, "<="))
    return WellKnownBinOp::LessOrEqualTo;
  else if (!strcmp(string, ">="))
    return WellKnownBinOp::GreaterOrEqualTo;
  else if (!strcmp(string, "<=>"))
    return WellKnownBinOp::Spaceship;

  else if (!strcmp(string, ":"))
    return WellKnownBinOp::RealTypeComparison;
  else if (!strcmp(string, "~"))
    return WellKnownBinOp::Virtualization;
  else if (!strcmp(string, "="))
    return WellKnownBinOp::Assignment;

  else if (!strcmp(string, "=="))
    return WellKnownBinOp::Equivalence;
  else if (!strcmp(string, "!="))
    return WellKnownBinOp::NonEquivalence;
  else if (!strcmp(string, "==="))
    return WellKnownBinOp::Equal;
  else if (!strcmp(string, "!=="))
    return WellKnownBinOp::NonEqual;

  else
    return std::nullopt;
}

enum class TypeBinOp {
  And,                ///< `T && U`.
  Or,                 ///< `T || U`.
  RealRestriction,    ///< `T : U`.
  VirtualRestriction, ///< `T~U`.
};

inline std::optional<TypeBinOp> well_known_to_type_binop(WellKnownBinOp well_known) {
  switch (well_known) {
  case (WellKnownBinOp::LogicAnd):
    return TypeBinOp::And;
  case (WellKnownBinOp::LogicOr):
    return TypeBinOp::Or;
  case (WellKnownBinOp::RealTypeComparison):
    return TypeBinOp::RealRestriction;
  case (WellKnownBinOp::Virtualization):
    return TypeBinOp::VirtualRestriction;
  default:
    return std::nullopt;
  }
}

#pragma endregion

#pragma region Literal

/// A be-literal type may be used as a virtual type restriction, e.g. `<Enable ~ \Bool>`.
enum class BeLiteralType { Bool, Int, UInt, Float, String, Char };

inline const char *beliteral_type_string(BeLiteralType type) {
  switch (type) {
  case BeLiteralType::Bool:
    return "\\Bool";
  case BeLiteralType::Int:
    return "\\Int";
  case BeLiteralType::UInt:
    return "\\UInt";
  case BeLiteralType::Float:
    return "\\Float";
  case BeLiteralType::String:
    return "\\String";
  case BeLiteralType::Char:
    return "\\Char";
  }
}

/// Return `BeLiteralType` if `id` is a may-be-literal type, e.g. `"Bool"`.
inline std::optional<BeLiteralType> parse_beliteral_type(std::string id) {
  if (!id.compare("Bool"))
    return BeLiteralType::Bool;
  if (!id.compare("Int"))
    return BeLiteralType::Int;
  if (!id.compare("UInt"))
    return BeLiteralType::UInt;
  if (!id.compare("Float"))
    return BeLiteralType::Float;
  if (!id.compare("String"))
    return BeLiteralType::String;
  if (!id.compare("Char"))
    return BeLiteralType::Char;
  else
    return std::nullopt;
}

/// An well-known identifier literal, e.g. `void` (or `Void` if can be used as a type).
enum class IDLiteral {
  Void,
  Discard,
  Nil,
  True,
  False,
  Self,
  This,
};

inline const char *id_literal_string(IDLiteral value, bool uppercase = false) {
  switch (value) {
  case IDLiteral::Void:
    return uppercase ? "Void" : "void";
  case IDLiteral::Discard:
    return uppercase ? "Discard" : "discard";
  case IDLiteral::Nil:
    return uppercase ? "Nil" : "nil";
  case IDLiteral::True:
    return "true";
  case IDLiteral::False:
    return "false";
  case IDLiteral::Self:
    return uppercase ? "Self" : "self";
  case IDLiteral::This:
    return "this";
  }
}

/// A literal which resolves to some special scope.
enum class SpecialScopeLiteral {
  Void,    ///< `void`, the void.
  Discard, ///< `discard`, which wraps void.
  Self,    ///< `self`, the static scope accessor.
  This,    ///< `this`, the instance scope accessor.
  // TODO: A local scope accessor, e.g. `local.foo` or `thou.foo`
};

inline IDLiteral special_scope_to_id_literal(SpecialScopeLiteral value) {
  switch (value) {
  case SpecialScopeLiteral::Void:
    return IDLiteral::Void;
  case SpecialScopeLiteral::Discard:
    return IDLiteral::Discard;
  case SpecialScopeLiteral::Self:
    return IDLiteral::Self;
  case SpecialScopeLiteral::This:
    return IDLiteral::This;
  }
}

#pragma endregion

/// A well-known built-in function.
/// TODO: Move to MLIR?
enum class BuiltinFunction {
  IntAdd,
  IntLte,
};

enum class AccessScope {
  Static,   ///< `::`.
  Instance, ///< `.`.
  UFCS,     ///< Universal Function Call Syntax, `:`.
};

} // namespace Lang
} // namespace Onyx
} // namespace Phoenix
} // namespace Fancysoft
