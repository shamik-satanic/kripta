cmake_minimum_required(VERSION 3.14)
project(crypto_math_service CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

add_library(math_service STATIC src/math_service.cpp)
target_include_directories(math_service PUBLIC include)
target_compile_options(math_service PRIVATE -Wall -Wextra -O2)

add_executable(demo src/main.cpp)
target_link_libraries(demo math_service m)

add_executable(unit_tests tests/unit_tests.cpp)
target_link_libraries(unit_tests math_service m)