#pragma once

#include <optional>
#include <tuple>
#include <utility>
#include <variant>

namespace Fancysoft {
namespace Phoenix {
namespace Util {
namespace Variant {

namespace FlattenImpl {

// A flattened variant implementation by Vittorio Romeo [1], licensed
// under CC BY-SA 3.0. Minor changes were made to adapt the code.
// This file is exemptive from the wrapping Phoenix license, and is
// licensed under CC BY-SA 3.0 as well.
//
// [1]: https://stackoverflow.com/a/39273356/3645337
//

template <typename... Ts>
using cat = decltype(std::tuple_cat(std::declval<Ts>()...));

template <typename TResult, typename... Ts> struct flatten_variant;

template <typename TResult> struct flatten_variant<TResult> {
  using Type = TResult;
};

template <typename TResult, typename T, typename... TOther>
struct flatten_variant<TResult, T, TOther...> {
  using Type =
      cat<TResult,
          std::tuple<T>,
          typename flatten_variant<TResult, TOther...>::Type>;
};

template <typename TResult, typename... Ts, typename... TOther>
struct flatten_variant<TResult, std::variant<Ts...>, TOther...> {
  using Type =
      cat<TResult,
          typename flatten_variant<std::tuple<>, Ts...>::Type,
          typename flatten_variant<TResult, TOther...>::Type>;
};

template <typename T> struct to_variant;

template <typename... Ts> struct to_variant<std::tuple<Ts...>> {
  using Type = std::variant<Ts...>;
};

} // namespace FlattenImpl

/// Flatten a variant.
///
/// @example test/cc/fancysoft/phoenix/util/flattened_variant.cc
///
/// @tparam T The flattened variant.
///
/// NOTE: Be extremely attentive to not include the same option type twice.
template <typename T>
using Flatten = typename FlattenImpl::to_variant<
    typename FlattenImpl::flatten_variant<std::tuple<>, T>::Type>::Type;

/// Get an option index value.
///
/// ```
/// std::variant<int, float> var = 42;
/// assert(Util::Variant::index<std::variant<int, float>, int>() == 0)
/// ```
template <typename VariantT, typename ElementT, std::size_t idx = 0>
constexpr std::size_t index() {
  if constexpr (idx == std::variant_size_v<VariantT>) {
    return idx;
  } else if constexpr (std::is_same_v<
                           std::variant_alternative_t<idx, VariantT>,
                           ElementT>) {
    return idx;
  } else {
    return index<VariantT, ElementT, idx + 1>();
  }
}

// /// Copy-cast `SourceT` if it contains a value from `TargetT`.
// /// Otherwise throws `std::bad_variant_access`.
// ///
// /// ```
// /// std::variant<int, float, char*> var = 42;
// /// Util::Variant::cast<std::variant<int, float>>(var);
// /// ```
// template <typename TargetT, typename SourceT> TargetT cast(SourceT &from) {
//   std::optional<TargetT> result;

//   std::visit(
//       [&result](auto &option) {
//         using T = decltype(option);

//         if constexpr (std::is_convertible_v<std::decay_t<T>, TargetT>) {
//           result = TargetT(option);
//         } else {
//           throw std::bad_variant_access{};
//         }
//       },
//       from);

//   return result.value();
// }

/// Check if `T` is contained in the variant type `VariantT`.
///
/// ```
/// static_assert(IsOneOf<int, std::variant<int, double>>::value, "BUG");
/// ```
template <class T, class VariantT> struct IsOneOf;

template <class T, class... Ts>
struct IsOneOf<T, std::variant<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

static_assert(IsOneOf<int, std::variant<int, double>>::value, "BUG");

/// A helper constexpr which always asserts to `false`.
///
/// ```
/// static_assert(Util::Variant::False<T>, "Non-exhaustive visitor!");
/// ```
template <typename> inline constexpr bool False = false;

/// Upcast a *SourceT* variant instance to *TargetT*.
///
/// ```
/// assert(Util::Variant::upcast<std::variant<int, float,
/// std::string>>(std::variant<float, int>()))
/// ```
template <typename TargetT, typename SourceT> TargetT upcast(SourceT from) {
  return std::visit(
      [](auto &option) {
        using T = decltype(option);

        if constexpr (IsOneOf<typename std::decay<T>::type, TargetT>::value) {
          return TargetT(option);
        } else {
          static_assert(False<T>, "Is not a part of TargetT");
        }
      },
      from);
}

/// Get a typed option reference or throw `std::bad_variant_access`.
///
/// ```
/// std::variant<int, char*> var = 42;
/// int opt = Util::Variant::option<int>(var);
/// assert(opt == 42);
/// ```
template <typename OptionT, typename VariantT>
OptionT *option(VariantT &variant) {
  OptionT *pointer;

  std::visit(
      [&pointer](auto &option) {
        using T = decltype(option);

        if constexpr (std::is_same_v<OptionT, std::decay_t<T>>) {
          pointer = std::addressof(option);
        } else {
          throw std::bad_variant_access{};
        }
      },
      variant);

  return pointer;
}

} // namespace Variant
} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft
