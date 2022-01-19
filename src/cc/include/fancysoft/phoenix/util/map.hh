#pragma once

#include <map>
#include <optional>
#include <unordered_map>

namespace Fancysoft {
namespace Phoenix {
namespace Util {
namespace Map {

/// Get a copy of the value at *key*, or return `std::nullopt`.
template <typename K, typename V, template <typename, typename...> class M>
std::optional<V> get_if(const M<K, V> &map, K key) {
  if (contains(map, key))
    return map.at(key);
  else
    return std::nullopt;
}

template <typename K, typename V, template <typename, typename...> class M>
bool contains(const M<K, V> &map, K key) {
  return map.find(key) != map.end();
}

} // namespace Map
} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft
