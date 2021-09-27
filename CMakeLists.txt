cmake_minimum_required(VERSION 3.15)
project(fnxc VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "Compiler is CLang")

  if(MINGW)
    message(STATUS "Compiler OS is MinGW")

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
      message(STATUS "Compiler frontend is MSVC")
      add_compile_options("/clang:-fcoroutines-ts")

    elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
      message(STATUS "Compiler frontend is GNU")
      add_compile_options("-stdlib=libc++" "-fcoroutines-ts")
      add_link_options("-stdlib=libc++")

    else()
      message(FATAL_ERROR "Unsupported compiler frontend")

    endif()

  elseif(WIN32)
    message(STATUS "Compiler OS is WIN32")

    # On Windows, libc++ is not needed?
    #

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
      message(STATUS "Compiler frontend is MSVC")
      add_compile_options("/clang:-fcoroutines-ts")

    elseif(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
      message(STATUS "Compiler frontend is GNU")
      add_compile_options("-fcoroutines-ts")

    else()
      message(FATAL_ERROR "Unsupported compiler frontend")

    endif()

  elseif(UNIX)
    message(STATUS "Compiler OS is UNIX")

    # On Unix, libc++ is needed.
    #

    add_compile_options("-stdlib=libc++" "-fcoroutines-ts")
    add_link_options("-stdlib=libc++")

  else()
    message(FATAL_ERROR "Unsupported compiler OS")

  endif()

# MacOS
elseif(APPLE)
  message(STATUS "Compiler OS is MacOS")
  add_compile_options("-fcoroutines-ts")

# GNU CC
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Compiler is GNU")
  add_compile_options("-fcoroutines")

else ()
  message(FATAL_ERROR "Unsupported compiler")

endif ()

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

add_library(fancysoft.util.logger src/cc/src/fancysoft/util/logger.cc)
add_library(fancysoft.util.null_stream src/cc/src/fancysoft/util/null_stream.cc)
add_library(fancysoft.util.utf8 src/cc/src/fancysoft/util/utf8.cc)
target_link_libraries(fancysoft.util.logger INTERFACE fancysoft.util.null_stream)

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
  src/cc/src/fancysoft/nxc/mlir.cc
  src/cc/src/fancysoft/nxc/placement.cc
  src/cc/src/fancysoft/nxc/program.cc

  src/cc/src/fancysoft/nxc.cc
)

target_link_libraries(fancysoft.nxc PUBLIC
  fmt
  fancysoft.util.logger
  fancysoft.util.null_stream
  fancysoft.util.utf8
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
)

# Testing
#

find_package(doctest 2.4 REQUIRED)
message(VERBOSE "doctest_INCLUDE_DIR == ${doctest_INCLUDE_DIR}")
find_path(doctest_INCLUDE_DIR doctest/doctest.h REQUIRED)
include_directories(${doctest_INCLUDE_DIR})

enable_testing()
add_custom_target(tests)

add_executable(test.fancysoft.util.coro test/cc/fancysoft/util/coro.cc)
add_test(NAME fancysoft/util/coro COMMAND test.fancysoft.util.coro)
add_dependencies(tests test.fancysoft.util.coro)

add_executable(test.fancysoft.util.flatten_variant test/cc/fancysoft/util/flatten_variant.cc)
add_test(NAME fancysoft/util/flatten_variant COMMAND test.fancysoft.util.flatten_variant)
add_dependencies(tests test.fancysoft.util.flatten_variant)

add_executable(test.fancysoft.util.pool test/cc/fancysoft/util/pool.cc)
add_test(NAME fancysoft/util/pool COMMAND test.fancysoft.util.pool)
add_dependencies(tests test.fancysoft.util.pool)

add_executable(test.fancysoft.util.utf8 test/cc/fancysoft/util/utf8.cc)
target_link_libraries(test.fancysoft.util.utf8 fancysoft.util.utf8)
add_test(NAME fancysoft/util/utf8 COMMAND test.fancysoft.util.utf8)
add_dependencies(tests test.fancysoft.util.utf8)
