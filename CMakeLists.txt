cmake_minimum_required(VERSION 3.16)
project(mini-pd VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(minipd_warnings tgt)
  if(MSVC)
    target_compile_options(${tgt} PRIVATE /W4)
  else()
    target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

add_library(minipd
  src/io/parser.cpp
  src/io/bookshelf.cpp
  src/metrics/hpwl.cpp
  src/viz/svg.cpp
  src/viz/qor.cpp
  src/place/random.cpp
  src/place/quadratic.cpp
  src/legalize/snap.cpp
  src/legalize/abacus.cpp
  src/route/gcell.cpp
)
target_include_directories(minipd PUBLIC src)
minipd_warnings(minipd)

add_executable(mini-pd src/flow.cpp)
target_link_libraries(mini-pd PRIVATE minipd)
minipd_warnings(mini-pd)

enable_testing()
add_executable(test_hpwl tests/test_hpwl.cpp)
target_link_libraries(test_hpwl PRIVATE minipd)
minipd_warnings(test_hpwl)
add_test(NAME hpwl COMMAND test_hpwl)

add_executable(test_overflow tests/test_overflow.cpp)
target_link_libraries(test_overflow PRIVATE minipd)
minipd_warnings(test_overflow)
add_test(NAME overflow COMMAND test_overflow)

add_executable(test_mst tests/test_mst.cpp)
target_link_libraries(test_mst PRIVATE minipd)
minipd_warnings(test_mst)
add_test(NAME mst COMMAND test_mst)
