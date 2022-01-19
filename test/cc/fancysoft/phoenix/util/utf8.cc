#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "fancysoft/util/utf8.hh"

using namespace Fancysoft::Util;

TEST_CASE("UTF8::code_point_byte_size") {
  CHECK(UTF8::code_point_byte_size('\0') == 1);
  CHECK(UTF8::code_point_byte_size('A') == 1);
  CHECK(UTF8::code_point_byte_size(246) == 2);    // ö
  CHECK(UTF8::code_point_byte_size(1046) == 2);   // Ж
  CHECK(UTF8::code_point_byte_size(8364) == 3);   // €
  CHECK(UTF8::code_point_byte_size(119070) == 4); // 𝄞
}

TEST_CASE("UTF8::size_from_leading_byte") {
  CHECK(UTF8::size_from_leading_byte('\0') == 1);
  CHECK(UTF8::size_from_leading_byte('A') == 1);
  CHECK(UTF8::size_from_leading_byte(195u) == 2); // ö = c3 b6
  CHECK(UTF8::size_from_leading_byte(208u) == 2); // Ж = d0 96
  CHECK(UTF8::size_from_leading_byte(226u) == 3); // € = e2 82 ac
  CHECK(UTF8::size_from_leading_byte(240u) == 4); // 𝄞 = f0 9d 84 9e
}

TEST_CASE("UTF8::to_code_units") {
  CHECK(std::string((char *)UTF8::to_code_units(0)) == "\0");
  CHECK(std::string((char *)UTF8::to_code_units(65)) == "A");
  CHECK(std::string((char *)UTF8::to_code_units(246)) == "ö");
  CHECK(std::string((char *)UTF8::to_code_units(1046)) == "Ж");
  CHECK(std::string((char *)UTF8::to_code_units(8364)) == "€");
  CHECK(std::string((char *)UTF8::to_code_units(119070)) == "𝄞");
}

TEST_CASE("UTF8::to_code_point") {
  CHECK(UTF8::to_code_point(u8"\0") == 0);
  CHECK(UTF8::to_code_point(u8"A") == 65);
  CHECK(UTF8::to_code_point(u8"ö") == 246);
  CHECK(UTF8::to_code_point(u8"Ж") == 1046);
  CHECK(UTF8::to_code_point(u8"€") == 8364);
  CHECK(UTF8::to_code_point(u8"𝄞") == 119070);
}
