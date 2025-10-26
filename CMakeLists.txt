cmake_minimum_required(VERSION 3.16)
project(lom)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} parser)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

