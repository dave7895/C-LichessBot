cmake_policy(SET CMP0048 NEW)
cmake_minimum_required( VERSION 3.18 )
project( LichessBot VERSION 0.1.0 LANGUAGES CXX )
include(FetchContent)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 67e12da316754ff3c6c91b50aafb2658438f3c1e) # the commit hash for 1.7.0
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v0.9.6
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(simdjson)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.1)
FetchContent_MakeAvailable(Catch2)

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/libchess/src/" )
add_library(libchess SHARED IMPORTED)
set_target_properties(libchess PROPERTIES
IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libchess/build/shared/libchess.so"
INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/libchess/src/")

set(CMAKE_CXX_STANDARD 20)
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -march=native -O3" )

add_executable(
  ${PROJECT_NAME}
  src/lichessBot.cpp
  src/lichessBot.h
  src/playGames.cpp
  src/playGames.h
  src/engine.cpp
  src/engine.h
  src/apiInteraction.cpp
  src/apiInteraction.h
)
target_link_libraries( ${PROJECT_NAME} cpr::cpr simdjson curl libchess)

file( GLOB TESTSRC tests/* )
add_executable(
  ${PROJECT_NAME}-tests
  tests/main.cpp
  tests/mateinone.cpp
  tests/bestcaptures.cpp
  tests/evaluations.cpp
  tests/alphabeta.cpp
  src/engine.cpp
  src/engine.h
)
target_link_libraries( ${PROJECT_NAME}-tests libchess Catch2::Catch2 cpr::cpr simdjson curl)
