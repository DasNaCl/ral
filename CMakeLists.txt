cmake_minimum_required(VERSION 3.5)

project(ral LANGUAGES CXX VERSION 0.0.1)

set(ral_files src/token.cpp
  src/ast.cpp
  src/stream_lookup.cpp
  src/source_range.cpp
  src/symbol.cpp
  src/parser.cpp
  src/type.cpp
  src/interpret.cpp
  )

# Dependencies
add_subdirectory(lib/robin-map)
find_package(fmt REQUIRED)
find_package(Threads REQUIRED)

option(USE_WERROR_DBG "Treat warnings as errors when compiling ral-lang." ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb -Wpedantic -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-implicit-fallthrough -Wno-switch")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

if(USE_WERROR_DBG)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
endif()

# main program
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(ral src/main.cpp "${ral_files}")
target_link_libraries(ral PRIVATE fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT} tsl::robin_map)
set_property(TARGET ral PROPERTY CXX_STANDARD 17)

