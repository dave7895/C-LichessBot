cmake_policy(SET CMP0048 NEW)
cmake_minimum_required( VERSION 3.18 )
project( LichessBot VERSION 0.1.0 LANGUAGES CXX )
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG 67e12da316754ff3c6c91b50aafb2658438f3c1e) # the commit hash for 1.7.0
FetchContent_MakeAvailable(cpr)
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v0.9.6
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(simdjson)
include_directories( /home/dave/libchesskz04px/src/ )
add_library(libchess SHARED IMPORTED)
set_target_properties(libchess PROPERTIES
IMPORTED_LOCATION "/home/dave/libchesskz04px/build/shared/libchess.so"
INTERFACE_INCLUDE_DIRECTORIES "/home/dave/libchesskz04px/src")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -march=native" )
file( GLOB SRCS *.c *.cpp *.cc *.h *.hpp )
add_executable( ${PROJECT_NAME} ${SRCS} )
target_link_libraries( ${PROJECT_NAME} cpr::cpr simdjson curl libchess)
