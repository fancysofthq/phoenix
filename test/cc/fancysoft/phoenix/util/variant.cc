#include <type_traits>
#include <variant>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "fancysoft/util/variant.hh"

using namespace Fancysoft::Util;

TEST_CASE("`Fancysoft::Util::Variant::Flatten`") {
  class Klass {};

  using Nested = std::
      variant<int, double, std::variant<std::string, int>, std::variant<Klass>>;

  using Flattened = std::variant<int, double, std::string, int, Klass>;

  static_assert(
      std::is_same<Variant::Flatten<Nested>, Flattened>{},
      "`Fancysoft::Util::Variant::Flatten` failed the test");
}

TEST_CASE("`Fancysoft::Util::Variant::index`") {
  std::variant<int, float> var = 42;
  CHECK(Variant::index<std::variant<int, float>, int>() == 0);
  CHECK(Variant::index<std::variant<int, float>, float>() == 1);
}

TEST_CASE("`Fancysoft::Util::Variant::upcast`") {
  std::variant<int, double> var = 42;

  auto cast = Variant::upcast<std::variant<int, double, char *>>(var);
  CHECK(std::get<int>(cast) == 42);

  CHECK_THROWS_AS(({ std::get<double>(cast) == 42; }), std::bad_variant_access);

  WHEN("shared ptr") {
    struct Foo {
      const std::string bar;
      Foo(std::string bar) : bar(bar) {}
    };

    std::variant<std::shared_ptr<Foo>> var = std::make_shared<Foo>("baz");
    auto cast = Variant::upcast<
        std::variant<std::shared_ptr<Foo>, std::shared_ptr<double>>>(var);
    CHECK(!std::get<std::shared_ptr<Foo>>(cast)->bar.compare("baz"));
  }
}

TEST_CASE("`Fancysoft::Util::Variant::option`") {
  std::variant<int, char *> var = 42;
  CHECK_THROWS_AS(Variant::option<char *>(var), std::bad_variant_access);

  auto opt = *Variant::option<int>(var);
  CHECK(opt == 42);
}
