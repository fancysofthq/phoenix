#pragma once

#include <map>
#include <optional>

namespace Fancysoft {
namespace Util {
namespace Map {

/// Get a value at *key*, or return `std::nullopt`.
template <typename K, typename V>
std::optional<V> get_if(const std::map<K, V> &map, K key) {
  if (map.find(key) != map.end())
    return map.at(key);
  else
    return std::nullopt;
}

} // namespace Map
} // namespace Util
} // namespace Fancysoft
