cmake_minimum_required(VERSION 3.20)

project(MiniLanguage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(mini_enable_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive- /utf-8)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

if(MSVC)
  set(MINI_CONFIGURED_MSVC true)
else()
  set(MINI_CONFIGURED_MSVC false)
endif()

configure_file(
  src/build_config.hpp.in
  "${CMAKE_CURRENT_BINARY_DIR}/mini_build_config.hpp"
  @ONLY
)

add_library(mini_compiler STATIC
  src/ast.cpp
  src/cli.cpp
  src/codegen.cpp
  src/diagnostics.cpp
  src/driver.cpp
  src/host_compiler.cpp
  src/lexer.cpp
  src/lower.cpp
  src/number_format.cpp
  src/parser.cpp
  src/resolver.cpp
  src/source.cpp
  src/token.cpp
)

target_include_directories(mini_compiler
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_compile_features(mini_compiler PUBLIC cxx_std_17)
mini_enable_warnings(mini_compiler)

add_executable(minic src/main.cpp)
target_link_libraries(minic PRIVATE mini_compiler)
target_compile_features(minic PRIVATE cxx_std_17)
mini_enable_warnings(minic)
