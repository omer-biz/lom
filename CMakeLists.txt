cmake_minimum_required(VERSION 3.16)
project(lom)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

