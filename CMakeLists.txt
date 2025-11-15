cmake_minimum_required(VERSION 3.16)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Create compile_commands.json" ON)

project(lom
  LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(tests)

# build liblom.a
add_library(lom STATIC src/parser.c src/lom.c)
target_include_directories(lom PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_include_directories(lom PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(Lua REQUIRED)
target_link_libraries(lom PRIVATE ${LUA_LIBRARIES})
target_include_directories(lom PRIVATE ${LUA_INCLUDE_DIR})


