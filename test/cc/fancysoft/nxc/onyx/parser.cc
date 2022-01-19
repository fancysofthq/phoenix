#include <sstream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "fancysoft/nxc/onyx/parser.hh"

#include "./scratch.cc"

using namespace Fancysoft::NXC::Onyx;

TEST_CASE("Simple var") {
  auto scratch = std::make_shared<Scratch>();
  scratch->stream << "let x = 42\n";
  scratch->parse();

  std::stringstream ss;
  scratch->ast()->print(ss);

  CHECK_EQ(ss.str(), "let x = 42\n");
}

TEST_CASE("Simple function") {
  auto scratch = std::make_shared<Scratch>();
  scratch->stream << "def sum(a :  Int32, b : Int32) : Int32\n"
                     "  return a +  b ; end";
  scratch->parse();

  std::stringstream ss;
  scratch->ast()->print(ss);

  CHECK_EQ(
      ss.str(),
      "def sum(a : Int32, b : Int32) : Int32\n"
      "	return a + b\n"
      "end\n");
}

TEST_CASE("Fib function") {
  auto scratch = std::make_shared<Scratch>();
  scratch->stream << "def  fib(n : Int32) {\n"
                     "  if n <=  1 then\n"
                     "    return n\n"
                     "  end\n"
                     "  \n"
                     "  return fib(n- 1)+fib(n -2) }";
  scratch->parse();

  std::stringstream ss;
  scratch->ast()->print(ss);

  CHECK_EQ(
      ss.str(),
      "def fib(n : Int32) {\n"
      "	if n <= 1 then\n"
      "		return n\n"
      "	end\n"
      "	\n"
      "	return fib(n - 1) + fib(n - 2)\n"
      "}\n");
}
