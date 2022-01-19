#include "fancysoft/phoenix/util/random.hh"

namespace Fancysoft::Phoenix::Util {

std::string Random::string(const std::string charset, size_t length) {
  auto random_char_lambda = [&charset]() -> char {
    const size_t max_index = (charset.size() - 1);
    return charset[rand() % max_index];
  };

  std::string string(length, 0);
  std::generate_n(string.begin(), length, random_char_lambda);

  return string;
}

std::string Random::alphanum_string(size_t length) {
  return string(
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz",
      length);
}

} // namespace Fancysoft::Phoenix::Util
