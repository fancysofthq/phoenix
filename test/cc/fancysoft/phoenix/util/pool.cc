#include <chrono>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "../../../src/fancysoft/util/pool.cc"

using namespace Fancysoft::Util;

struct Dummy {
  int value;
};

auto pool = Pool<Dummy>([] { return Dummy(); }, 3);

TEST_SUITE("pool") {
  TEST_CASE("basics") {
    auto dummy = pool.acquire();
    dummy->value = 42;

    pool.release(move(dummy));
    dummy = pool.acquire();

    CHECK(dummy->value == 42);
    pool.release(move(dummy));
  }

  SCENARIO("multiple checkins") {
    auto dummy1 = pool.acquire();
    CHECK(dummy1->value == 42);
    dummy1->value = 1;

    auto dummy2 = pool.acquire();
    dummy2->value = 2;

    pool.release(move(dummy2));
    pool.release(move(dummy1));

    auto dummy3 = pool.acquire();
    CHECK_MESSAGE(dummy3->value == 2, "uses FIFO queue");
    pool.release(move(dummy3));
  }

  SCENARIO("with timeout") {
    auto dummy1 = pool.acquire();
    CHECK(dummy1->value == 1); // dummy1 from previous test

    auto dummy2 = pool.acquire();
    CHECK(dummy2->value == 2); // dummy2 from previous test

    auto dummy3 = pool.acquire();
    CHECK(dummy3->value == 0); // New object

    auto dummy4 = pool.acquire(std::chrono::milliseconds(10));
    CHECK_MESSAGE(dummy4 == nullptr, "should return NULL upon timeout");
  }
}
