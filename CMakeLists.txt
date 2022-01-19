cmake_minimum_required(VERSION 3.15)
project(fancysoft/fnx VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED)

if (IPO_SUPPORTED)
  message(STATUS "Interprocedural optimization is enabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "Interprocedural optimization is not supported")
endif()

# Project setup
#

include_directories(src/cc/include)

# Dependencies
#

find_package(fancysoft-util 0 REQUIRED)
message(STATUS "fancysoft_util_INCLUDE_DIR == ${fancysoft_util_INCLUDE_DIR}")
find_path(fancysoft_util_INCLUDE_DIR fancysoft/util/variant.hh REQUIRED)
include_directories(${fancysoft_util_INCLUDE_DIR})

find_package(fmt 7.1 REQUIRED)
message(VERBOSE "fmt_INCLUDE_DIR == ${fmt_INCLUDE_DIR}")
find_path(fmt_INCLUDE_DIR fmt/core.h REQUIRED)
include_directories(${fmt_INCLUDE_DIR})

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

find_package(LLD REQUIRED CONFIG)
include_directories(${LLD_INCLUDE_DIRS})

# Libraries
#

# NXC logger
add_library(fancysoft.nxc.logger src/cc/src/fancysoft/nxc/logger.cc)
target_link_libraries(fancysoft.nxc.logger INTERFACE fancysoft.util.null_stream)

llvm_map_components_to_libnames(LLVM_LIBS core target X86)

# The main executable
#

add_executable(fancysoft.nxc
  src/cc/src/fancysoft/nxc/c/ast.cc
  src/cc/src/fancysoft/nxc/c/block.cc
  src/cc/src/fancysoft/nxc/c/lexer.cc
  src/cc/src/fancysoft/nxc/c/parser.cc

  src/cc/src/fancysoft/nxc/onyx/ast.cc
  src/cc/src/fancysoft/nxc/onyx/file.cc
  src/cc/src/fancysoft/nxc/onyx/lexer.cc
  src/cc/src/fancysoft/nxc/onyx/parser.cc

  src/cc/src/fancysoft/nxc/cli.cc
  src/cc/src/fancysoft/nxc/file.cc
  src/cc/src/fancysoft/nxc/mlir.cc
  src/cc/src/fancysoft/nxc/placement.cc
  src/cc/src/fancysoft/nxc/program.cc

  src/cc/src/fancysoft/nxc.cc
)

target_link_libraries(fancysoft.nxc PUBLIC
  fmt

  ${LLVM_LIBS}

  lldCOFF
  lldCommon
  lldCore
  lldDriver
  lldELF
  lldMachO
  lldMachO2
  lldMinGW
  lldReaderWriter
  lldWasm

  fancysoft.util.random
  fancysoft.util.utf8

  fancysoft.nxc.logger
)

# Testing
#

find_package(doctest 2.4 REQUIRED)
message(VERBOSE "doctest_INCLUDE_DIR == ${doctest_INCLUDE_DIR}")
find_path(doctest_INCLUDE_DIR doctest/doctest.h REQUIRED)
include_directories(${doctest_INCLUDE_DIR})

enable_testing()
add_custom_target(tests)

# Onyx parser
add_executable(test.fancysoft.nxc.onyx.parser
  test/cc/fancysoft/nxc/onyx/parser.cc

  src/cc/src/fancysoft/nxc/c/ast.cc
  src/cc/src/fancysoft/nxc/c/block.cc
  src/cc/src/fancysoft/nxc/c/lexer.cc
  src/cc/src/fancysoft/nxc/c/parser.cc

  src/cc/src/fancysoft/nxc/onyx/ast.cc
  src/cc/src/fancysoft/nxc/onyx/lexer.cc
  src/cc/src/fancysoft/nxc/onyx/parser.cc
)
target_link_libraries(test.fancysoft.nxc.onyx.parser PUBLIC fmt fancysoft.nxc.logger)
add_test(NAME fancysoft/nxc/onyx/parser COMMAND test.fancysoft.nxc.onyx.parser)
add_dependencies(tests test.fancysoft.nxc.onyx.parser)
