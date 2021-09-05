#pragma once

#include <tuple>
#include <variant>

namespace Fancysoft {
namespace Util {
namespace Variant {

namespace FlattenImpl {

// A flattened variant implementation by Vittorio Romeo [1], licensed
// under CC BY-SA 3.0. Minor changes were made to adapt the code.
// This file is exemptive from the wrapping FNX license, and is
// licensed under CC BY-SA 3.0 as well.
//
// [1]: https://stackoverflow.com/a/39273356/3645337
//

// Type of the concatenation of all 'Ts...' tuples.
template <typename... Ts>
using cat = decltype(std::tuple_cat(std::declval<Ts>()...));

template <typename TResult, typename... Ts> struct flatten_variant;

// Base case: no more types to process.
template <typename TResult> struct flatten_variant<TResult> {
  using type = TResult;
};

// Case: T is not a variant.
// Return concatenation of previously processed types,
// T, and the flattened remaining types.
template <typename TResult, typename T, typename... TOther>
struct flatten_variant<TResult, T, TOther...> {
  using type =
      cat<TResult,
          std::tuple<T>,
          typename flatten_variant<TResult, TOther...>::type>;
};

// Case: T is a variant.
// Return concatenation of previously processed types,
// the types inside the variant, and the flattened remaining types.
// The types inside the variant are recursively flattened in a new
// flatten_variant instantiation.
template <typename TResult, typename... Ts, typename... TOther>
struct flatten_variant<TResult, std::variant<Ts...>, TOther...> {
  using type =
      cat<TResult,
          typename flatten_variant<std::tuple<>, Ts...>::type,
          typename flatten_variant<TResult, TOther...>::type>;
};

template <typename T> struct to_variant;

template <typename... Ts> struct to_variant<std::tuple<Ts...>> {
  using type = std::variant<Ts...>;
};
} // namespace FlattenImpl

/// Flatten a variant.
///
/// @example test/cc/fnxc/util/flattened_variant.cc
///
/// @tparam T The flattened variant.
template <typename T>
using flatten_t = typename FlattenImpl::to_variant<
    typename FlattenImpl::flatten_variant<std::tuple<>, T>::type>::type;

namespace UpcastImpl {

template <class... Args> struct _proxy {
  std::variant<Args...> v;

  template <class... ToArgs> operator std::variant<ToArgs...>() const {
    return std::visit(
        [](auto &&arg) -> std::variant<ToArgs...> { return arg; }, v);
  }
};

} // namespace UpcastImpl

template <class... Args>
auto upcast(const std::variant<Args...> &v) -> UpcastImpl::_proxy<Args...> {
  return {v};
}

/// Get an option index value.
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

/// Get a typed option reference (safe).
template <typename TargetT, typename VariantT>
TargetT *option(VariantT &variant) {
  TargetT *pointer;

  std::visit(
      [&pointer](auto &&option) {
        using OptionT = decltype(option);

        if constexpr (std::is_base_of_v<TargetT, std::decay_t<OptionT>>) {
          pointer = std::addressof(option);
        } else {
          throw std::bad_variant_access{};
        }
      },
      std::forward<VariantT>(variant));

  return pointer;
}

template <class> inline constexpr bool always_false_v = false;

template <typename... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // namespace Variant
} // namespace Util
} // namespace Fancysoft
