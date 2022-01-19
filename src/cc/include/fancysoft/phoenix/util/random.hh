#pragma once

#include <random>

namespace Fancysoft {
namespace Phoenix {
namespace Util {
namespace Random {

/// Generate a random string of given *length* from *charset*.
std::string string(const std::string charset, size_t length);

/// Returns a random alphanumeric string of given *length*.
std::string alphanum_string(size_t length);

} // namespace Random
} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft
